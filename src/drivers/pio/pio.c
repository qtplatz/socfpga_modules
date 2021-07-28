/**************************************************************************
** Copyright (C) 2016-2021 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#include "pio.h"
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h> // create_proc_entry
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/ktime.h>

static char *devname = MODNAME;
MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "Device Driver for the Delay Pulse Generator" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, "pio param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __platform_driver;
static struct platform_device *__pdev;
static dev_t pio_dev_t = 0;
static struct cdev * __pio_cdev;
static struct class * __pio_class;
static struct semaphore __sem;
static wait_queue_head_t __queue;
static int __queue_condition;

struct pio_driver {
    struct semaphore sem;
	struct gpio_desc * inject_in;
    struct gpio_desc * inject_out;
	struct gpio_desc * led;
    u32 gpio_inject_out; // legacy gpio number
    u32 gpio_inject_in;  // legacy gpio number
    u32 gpio_led;        // legacy gpio number
    u32 injin_irq;
    u64 tp;              // time point for last injection event
    u64 irqCount;
};

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    if ( dev_id == __pdev ) {
        struct pio_driver * drv = platform_get_drvdata( __pdev );
        if ( drv->injin_irq == irq ) {

            drv->tp  = ktime_get_real_ns(); // UTC
            drv->irqCount++;

            ++__queue_condition;
            wake_up_interruptible( &__queue );

            dev_info(&__pdev->dev, "handle_interrupt inj.in irq %d", irq );
        }
    }
    return IRQ_HANDLED;
}

static int
pio_proc_read( struct seq_file * m, void * v )
{
    struct pio_driver * drv = platform_get_drvdata( __pdev );

    if ( drv ) {
        seq_printf( m, "injin irq: %d\n",  drv->injin_irq );
        // seq_printf( m, "dipsw irq: %d\n",  drv->dipsw_irq );

        int led = gpio_get_value( drv->gpio_led );
        int inj_o = gpio_get_value( drv->gpio_inject_out );

        gpio_set_value( drv->gpio_inject_out, inj_o ? 0 : 1 );
        gpio_set_value( drv->gpio_led, led ? 0 : 1 );

        int inj_i = 0;
        for ( int i = 0; i < 2; ++i )
            inj_i |= ( gpio_get_value( drv->gpio_inject_in + i ) ? 1 : 0 ) << i;

        seq_printf( m
                    , "led = %d, inect_out = %d, inj.in = 0x%x\n"
                    , led, inj_o, inj_i );
    }

    return 0;
}

static ssize_t
pio_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    return size;
}

static int
pio_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, pio_proc_read, NULL );
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)  // not sure, which version is relevant though
static const struct proc_ops pio_proc_file_fops = {
    .proc_open    = pio_proc_open,
    .proc_read    = seq_read,
    .proc_write   = pio_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations pio_proc_file_fops = {
    .owner   = THIS_MODULE,
    .open    = pio_proc_open,
    .read    = seq_read,
    .write   = pio_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

static int pio_cdev_open(struct inode *inode, struct file *file)
{
    dev_info(&__pdev->dev, "pio_cdev_open inode=%d", MINOR( inode->i_rdev ) );
    return 0;
}

static int pio_cdev_release(struct inode *inode, struct file *file)
{
    dev_info(&__pdev->dev, "pio_cdev_release inode=%d", MINOR( inode->i_rdev ) );
    return 0;
}

static long pio_cdev_ioctl( struct file* file, unsigned int code, unsigned long args)
{
    return 0;
}

static ssize_t pio_cdev_read(struct file *file, char __user *data, size_t size, loff_t *f_pos )
{
    ssize_t count = 0;

    struct pio_driver * drv = platform_get_drvdata( __pdev );
    if ( !drv )
        return -EFAULT;

    if ( down_interruptible( &__sem ) ) {
        dev_err(&__pdev->dev, "pio_cdev_read failed for down_interruptible\n" );
        return -ERESTARTSYS;
    }
    // dev_info(&__pdev->dev, "pio_cdev_read size=%d, pos=%lld, tp=%lld\n", size, *f_pos, drv->tp );

    __queue_condition = 0;
    wait_event_interruptible( __queue, __queue_condition != 0 );

    __queue_condition = 0;

    u64 rep[ 2 ] = { drv->tp, drv->irqCount };
    count = ( size >= sizeof( rep ) ) ? sizeof( rep ) : ( size >= sizeof( u64 ) ) ? sizeof( u64 ) : size;

    if ( copy_to_user( data, (const void * )(&drv->tp), count ) ) {
        up( &__sem );
        return -EFAULT;
    }

    *f_pos += count;

    up( &__sem );
    return count;
}

static ssize_t pio_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    *f_pos += size;
    return size;
}

static struct file_operations pio_cdev_fops = {
    .owner   = THIS_MODULE,
    .open    = pio_cdev_open,
    .release = pio_cdev_release,
    .unlocked_ioctl = pio_cdev_ioctl,
    .read    = pio_cdev_read,
    .write   = pio_cdev_write,
};

static int pio_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0644 );
    return 0;
}

static int pio_dtor( int errno )
{
    if ( __pio_class ) {
        class_destroy( __pio_class );
        printk( KERN_INFO "" MODNAME " class_destry done\n" );
    }

    if ( __pio_cdev ) {
        cdev_del( __pio_cdev );
        printk( KERN_INFO "" MODNAME " cdev_del done\n" );
    }

    if ( pio_dev_t ) {
        unregister_chrdev_region( pio_dev_t, 1 );
        printk( KERN_INFO "" MODNAME " unregister_chrdev_region done\n" );
    }

    return errno;
}

static int __init
pio_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&pio_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __pio_cdev = cdev_alloc();
    if ( !__pio_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __pio_cdev, &pio_cdev_fops );
    if ( cdev_add( __pio_cdev, pio_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return pio_dtor( -1 );
    }

    __pio_class = class_create( THIS_MODULE, MODNAME );
    if ( !__pio_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return pio_dtor( -1 );
    }
    __pio_class->dev_uevent = pio_dev_uevent;

    // make_nod /dev/pio0
    if ( !device_create( __pio_class, NULL, pio_dev_t, NULL, MODNAME "%d", MINOR( pio_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return pio_dtor( -1 );
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &pio_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    sema_init( &__sem, 1 );
    init_waitqueue_head( &__queue );
    __queue_condition = 0;

    return 0;
}

static void
pio_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __pio_class, pio_dev_t ); // device_creeate

    class_destroy( __pio_class ); // class_create

    cdev_del( __pio_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( pio_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n<-----------\n\n", MOD_VERSION );
}

static int
pio_perror( struct platform_device * pdev, const char * prefix, void * p )
{
    if ( IS_ERR( p ) ) {
        if ( PTR_ERR( p ) != -EPROBE_DEFER )
            dev_info(&pdev->dev, "%s error code: %ld", prefix, PTR_ERR(p) );
        return PTR_ERR(p);
    }
    return 0;
}

static int
pio_module_probe( struct platform_device * pdev )
{
    dev_info( &pdev->dev, "pio_module probed [%s]", pdev->name );

    if ( platform_get_drvdata( pdev ) == 0 ) {
        struct pio_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct pio_driver ), GFP_KERNEL );
        if ( ! drv )
            return -ENOMEM;
        __pdev = pdev;
        platform_set_drvdata( pdev, drv );
    }
    struct pio_driver * drv = platform_get_drvdata( pdev );
    struct device_node * node = pdev->dev.of_node;
    int rcode = 0;

    // LED
    drv->led = devm_gpiod_get_optional( &pdev->dev, "led", GPIOD_OUT_LOW );
    if ( pio_perror( pdev, "led", drv->led ) == -EPROBE_DEFER ) {
        if ( of_property_read_u32_index( node, "led-gpios", 1, &drv->gpio_led ) == 0 ) {
            dev_info(&__pdev->dev, "led-gpios switch to legacy api: numberr %d", drv->gpio_led );
            if ( ( rcode = devm_gpio_request( &pdev->dev, 2016, "led" ) ) == 0 ) {
                gpio_direction_output( drv->gpio_led, 0 );
            }
        } else {
            dev_err( &pdev->dev, "legacy gpio led request failed: %d", rcode );
        }
    }

    // INJ.IN
    drv->inject_in = devm_gpiod_get_optional( &pdev->dev, "inject_in", GPIOD_IN );
    if ( pio_perror( pdev, "inject_in", drv->inject_in ) == -EPROBE_DEFER ) {
        if ( of_property_read_u32_index( node, "inject_in-gpios", 1, &drv->gpio_inject_in ) == 0 ) {
            dev_info(&__pdev->dev, "inject_in-gpios switch to legacy api: numberr %d", drv->gpio_inject_in );
            if ( ( rcode = devm_gpio_request( &pdev->dev, drv->gpio_inject_in, "inject_in" ) ) == 0 ) {
                gpio_direction_input( drv->gpio_inject_in );
            }
        } else {
            dev_err( &pdev->dev, "legacy gpio inject_in request failed: %d", rcode );
        }
    }

    // INJ.OUT
    drv->inject_out = devm_gpiod_get_optional( &pdev->dev, "inject_out", GPIOD_OUT_HIGH );
    if ( pio_perror( pdev, "inject_out", drv->inject_out ) == -EPROBE_DEFER ) {
        if ( of_property_read_u32_index( node, "inject_out-gpios", 1, &drv->gpio_inject_out ) == 0 ) {
            dev_info(&__pdev->dev, "inject_out-gpios switch to legacy api: numberr %d", drv->gpio_inject_out );
            if ( ( rcode = devm_gpio_request( &pdev->dev, drv->gpio_inject_out, "inject_out" ) ) == 0 ) {
                gpio_direction_output( drv->gpio_inject_out, 1 );
            }
        } else {
            dev_err( &pdev->dev, "legacy gpio inject_out request failed: %d", rcode );
        }
    }

    // IRQ
    if ( gpio_is_valid( drv->gpio_inject_in ) ) {
        if ( (drv->injin_irq = gpio_to_irq( drv->gpio_inject_in ) ) > 0 ) {
            dev_info(&pdev->dev, "\tinj.in gpio irq is %d", drv->injin_irq );
            if ( (rcode = devm_request_irq( &pdev->dev
                                            , drv->injin_irq
                                            , handle_interrupt
                                            , IRQF_SHARED | IRQF_TRIGGER_FALLING
                                            , "inj.in"
                                            , pdev )) != 0 ) {
                dev_err( &pdev->dev, "inj.in irq request failed: %d", rcode );
            }
        }
    }
    return 0;
}

static int
pio_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        dev_info( &pdev->dev, "IRQ %d about to be freed\n", irqNumber );
        free_irq( irqNumber, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __pio_module_id [] = {
    { .compatible = "altr,cds-pio" }
    , { .compatible = "altr,pio-20.1" }
    , { .compatible = "altr,pio-1.0" }
    , {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __pio_module_id )
        ,
    }
    , .probe = pio_module_probe
    , .remove = pio_module_remove
};

module_init( pio_module_init );
module_exit( pio_module_exit );
