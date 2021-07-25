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

static int populate_device_tree( struct seq_file * m );

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

struct pio_driver {
    uint32_t irq;
    uint64_t irqCount_;
    void __iomem * regs;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
    struct resource * mem_resource;
};

static struct semaphore __sem;
static wait_queue_head_t __trig_queue;
static int __trig_queue_condition = 0;

// --> 2017-APR-24
struct region {
    struct resource * resource_;
    void * iomem_;
};

struct region_allocated {
    size_t size;
    struct region regions[ 16 ];
};

struct gpio_register {
    const char * name;
    const char * gpio_name;
    int gpio; // base
    uint32_t count;
    uint32_t regs[ 3 ];
    uint32_t start;
    volatile uint32_t * addr;
};

struct irq_allocated {
    size_t size;
    int irqNumber[ 128 ];
};

struct gpio_dev_id {
    int gpioNumber;
    int irqNumber;
    struct gpio_register * parent;
};

struct gpio_allocated {
    size_t size;
    int gpioNumber[ 32 ];
};

struct trig_data {
    struct pio_trigger_data * cache_;
    uint32_t cache_size_;
    uint32_t rp_;
    uint32_t wp_;
    uint32_t tailp_;
};

static int __debug_level__ = 0;

inline static void irq_mask( struct gpio_register * pio, int mask )
{
    if ( pio->addr )
        pio->addr[ 2 ] = mask;
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    if ( __debug_level__ )
        printk(KERN_INFO "" MODNAME " handle irq %d\n", irq );

    return IRQ_HANDLED;
}

static int
pio_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "proc read\n" );
    populate_device_tree( m );

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
    if ( __debug_level__ )
        printk(KERN_INFO "" MODNAME " open dgmod char device\n");

    return 0;
}

static int pio_cdev_release(struct inode *inode, struct file *file)
{
    if ( __debug_level__ )
        printk(KERN_INFO "" MODNAME " release dgmod char device\n");

    return 0;
}

static long pio_cdev_ioctl( struct file* file, unsigned int code, unsigned long args)
{
    return 0;
}

static ssize_t pio_cdev_read(struct file *file, char __user *data, size_t size, loff_t *f_pos )
{
    size_t count = 0;

    if ( down_interruptible( &__sem ) ) {
        printk(KERN_INFO "down_interruptible for read faild\n" );
        return -ERESTARTSYS;
    }

    wait_event_interruptible( __trig_queue, __trig_queue_condition != 0 );
    __trig_queue_condition = 0;

    *f_pos += count;

    up( &__sem );

    return count;
}

static ssize_t pio_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    if ( __debug_level__ )
        printk(KERN_INFO "pio_cdev_write size=%d\n", size );

//    if ( *f_pos >= sizeof( protocol_sequence ) ) {
        printk(KERN_INFO "pio_cdev_write size overrun size=%d, offset=%lld\n", size, *f_pos );
//        return 0;
//    }

    /* if ( copy_from_user( (char *)(&protocol_sequence) + *f_pos, data, count ) ) */
    /*     return -EFAULT; */

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

    // make_nod /dev/dgmod0
    if ( !device_create( __pio_class, NULL, pio_dev_t, NULL, "dgmod%d", MINOR( pio_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return pio_dtor( -1 );
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &pio_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    sema_init( &__sem, 1 );

    init_waitqueue_head( &__trig_queue );

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
pio_module_probe( struct platform_device * pdev )
{
    dev_info( &pdev->dev, "pio_module proved [%s]", pdev->name );

    int irq = 0;
    if ( platform_get_drvdata( pdev ) == 0 ) {
        struct pio_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct pio_driver ), GFP_KERNEL );
        if ( ! drv )
            return -ENOMEM;

        dev_info( &pdev->dev, "pio_module [%s] did not have pio_driver data; crate it.", pdev->name );

        __pdev = pdev;
        platform_set_drvdata( pdev, drv );
    }
    struct pio_driver * drv = platform_get_drvdata( pdev );

    for ( int i = 0; i < pdev->num_resources; ++i ) {
        struct resource * res = platform_get_resource( pdev, IORESOURCE_MEM, i );
        if ( res ) {
            void __iomem * regs = devm_ioremap_resource( &pdev->dev, res );
            if ( regs ) {
                drv->regs = regs;
                drv->mem_resource = res;
            }
            dev_info( &pdev->dev, "dgmod probe resource[%d]: %x -- %x, map to %p\n"
                      , i, res->start, res->end, regs );
        }
        sema_init( &drv->sem, 1 );
    }

    if ( ( irq = platform_get_irq( pdev, 0 ) ) > 0 ) {
        dev_info( &pdev->dev, "platform_get_irq: %d", irq );
        if ( devm_request_irq( &pdev->dev, irq, handle_interrupt, 0, MODNAME, pdev ) == 0 ) {
            drv->irq = irq;
        } else {
            dev_err( &pdev->dev, "Failed to register IRQ.\n" );
            return -ENODEV;
        }
    }
    return 0;
}

static int
pio_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __pio_module_id [] = {
    { .compatible = "dummy,dummy" }
    , { .compatible = "simple-pio" }
    /* , { .compatible = "altr,juart-1.0" } */
    /* , { .compatible = "altr,sysid-1.0" } */
    /* , { .compatible = "altr,pio-1.0" } */
    /* , { .compatible = "xaltr,pio-1.0" } */
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


static int populate_device_tree( struct seq_file * m )
{
    struct device_node * node;
    struct device_node * child;

    // trial to find hps_led0
    if ( ( node = of_find_node_by_path( "/soc/base_fpga_region" ) ) ) {

        for_each_child_of_node( node, child ) {
            seq_printf( m, "child->name = %s, addr_cells = %d, size_cells = %d\n"
                        , child->name
                        , of_n_addr_cells( child )
                        , of_n_size_cells( child ) );
            struct property * prop = child->properties;
            do {
                const char * value[ 32 ] = { 0 };
                u32 ivalue[ 32 ] = { 0 };
                seq_printf( m, "\tname = %s\t", prop->name );

                if ( of_property_read_string( child, prop->name, value ) == 0 ) {
                    for ( int i = 0; i < countof(value) && value[ i ]; ++i )
                        seq_printf( m, "'%s' ", value[i] );

                    if ( strlen( value[0] ) == 0 ) {
                        int sz = prop->length / 4 < countof(ivalue) ? prop->length / 4 : countof(ivalue);
                        if ( of_property_read_u32_array( child, prop->name, ivalue, sz ) == 0 ) {
                            for ( int i = 0; i < prop->length / 4; ++i )
                                seq_printf( m, "0x%x ", ivalue[ i ] );
                        }
                    }
                }
                seq_printf( m, "\n" );
            } while (( prop = prop->next ));

            struct device_node * c1;
            for_each_child_of_node( child, c1 ) {
                seq_printf( m, "\tchild->name = %s\n", c1->name );
            }
        }
        return 0;
    }
    return -1;
}
