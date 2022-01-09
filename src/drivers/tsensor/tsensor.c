/**************************************************************************
** Copyright (C) 2022 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include "tsensor.h"
#include <linux/version.h>
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
#include <linux/mm.h>
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
MODULE_PARM_DESC(devname, "tsensor param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __platform_driver;
static struct platform_device *__pdev;
static dev_t tsensor_dev_t = 0;
static struct cdev * __tsensor_cdev;
static struct class * __tsensor_class;

struct tsensor_driver {
	struct gpio_desc * tsensor_sync; // tsensor_sync; // t0 trigger
    u32 legacy_gpio_number;        // legacy gpio number
    u32 irq;
    u32 irqCount;
    void __iomem * regs;
    struct resource * mem_resource;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
};

struct tsensor_cdev_private {
    u32 node;
    u32 size;
    void * mmap;
};


typedef struct slave_data {
    uint32_t user_dataout;    // 0x00
    uint32_t user_datain;     // 0x04
    uint32_t irqmask;         // 0x08
    uint32_t edge_capture;    // 0x0c
    uint32_t resv[12];        //
} slave_data;

inline static volatile slave_data * __slave_data( void __iomem * regs, u32 idx )
{
    return ((volatile slave_data *)(regs)) + idx;
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    if ( dev_id == __pdev ) {
        struct tsensor_driver * drv = platform_get_drvdata( __pdev );
        if ( drv->irq == irq ) {
            drv->irqCount++;
            drv->queue_condition++;
            wake_up_interruptible( &drv->queue );
        }
    }
    return IRQ_HANDLED;
}

static int
tsensor_proc_read( struct seq_file * m, void * v )
{
    struct tsensor_driver * drv = platform_get_drvdata( __pdev );

    if ( drv ) {
        seq_printf( m, "proc_read irqCount=%d\n", drv->irqCount );

        // current page can read from [0]
        u32 data[ 16 ];
        for ( size_t i = 0; i < 16; ++i ) {
            volatile struct slave_data * p = __slave_data( drv->regs, i );
            data[ i ] = p->user_dataout;
        }

        seq_printf( m, "[id] <data>\n" );
        for ( size_t i = 0; i < 16; ++i ) {
            seq_printf( m, "[%2d] ", i );
            seq_printf( m, "%08x\t", data[ i ] );
            if ( i == 1 ) {
                seq_printf( m, "%08x\t%10d (degC)", data[ i ] >> 3, (( data[ i ] >> 3 ) & 0xfff) * 1024 / 0x1000 );
            }
            seq_printf( m, "\n" );
        }
    }

    return 0;
}

static void
tsensor_dg_init( struct tsensor_driver * drv )
{
}

static ssize_t
tsensor_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    static char readbuf[256];

    if ( size >= sizeof( readbuf ) )
        size = sizeof( readbuf ) - 1;

    if ( copy_from_user( readbuf, user, size ) )
        return -EFAULT;

    struct tsensor_driver * drv = platform_get_drvdata( __pdev );

    if ( strncmp( readbuf, "off", 3 ) == 0 ) {
        __slave_data( drv->regs, 2 )->user_datain = __slave_data( drv->regs, 2 )->user_dataout | 0x02;
        dev_info(&__pdev->dev, "tsensor_proc_write command off 0x%x", __slave_data( drv->regs, 2 )->user_dataout );
    } else if ( strncmp( readbuf, "on", 2 ) == 0 ) {
        __slave_data( drv->regs, 2 )->user_datain = __slave_data( drv->regs, 2 )->user_dataout & ~0x02;
        dev_info(&__pdev->dev, "tsensor_proc_write command on 0x%x", __slave_data( drv->regs, 2 )->user_dataout );
    }

    readbuf[ size ] = '\0';

    return size;
}

static int
tsensor_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, tsensor_proc_read, NULL );
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)  // not sure, which version is relevant though
static const struct proc_ops tsensor_proc_file_fops = {
    .proc_open    = tsensor_proc_open,
    .proc_read    = seq_read,
    .proc_write   = tsensor_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations tsensor_proc_file_fops = {
    .owner   = THIS_MODULE,
    .open    = tsensor_proc_open,
    .read    = seq_read,
    .write   = tsensor_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

static int tsensor_cdev_open(struct inode *inode, struct file *file)
{
    // dev_info(&__pdev->dev, "tsensor_cdev_open inode=%d", MINOR( inode->i_rdev ) );
    struct tsensor_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct tsensor_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 4 * sizeof(u32); // 2 dwords (set, act, sw)
        private_data->mmap = 0;
        file->private_data = private_data;
    }

    return 0;
}

static int tsensor_cdev_release(struct inode *inode, struct file *file)
{
    if ( file->private_data ) {
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long tsensor_cdev_ioctl( struct file* file, unsigned int code, unsigned long args)
{
    return 0;
}

static ssize_t tsensor_cdev_read(struct file *file, char __user *data, size_t size, loff_t *f_pos )
{
    ssize_t count = 0;

    struct tsensor_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( down_interruptible( &drv->sem ) ) {
            dev_info(&__pdev->dev, "%s: down_interruptible for read faild\n", __func__ );
            return -ERESTARTSYS;
        }

        struct tsensor_cdev_private * private_data = file->private_data;
        *f_pos &= ~03;        // 32 bit align

        wait_event_interruptible( drv->queue, drv->queue_condition != 0 );
        drv->queue_condition = 0;

        while ( ( *f_pos < private_data->size ) && (count + sizeof(u32)) <= size ) {

            size_t idx = *f_pos / sizeof(u32);
            u32 d = ( idx == 1 ) ? (__slave_data( drv->regs, idx )->user_dataout >> 3) & 0xfff
                : __slave_data( drv->regs, idx )->user_dataout
                ;

            if ( copy_to_user( data, (const char *)&d, sizeof(u32) ) ) {
                up( &drv->sem );
                return -EFAULT;
            }
            count += sizeof( u32 );
            data += sizeof( u32 );
            *f_pos += sizeof( u32 );
        }
        up( &drv->sem );
    }
    return count;
}

static ssize_t tsensor_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    ssize_t count = 0;
    struct tsensor_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( down_interruptible( &drv->sem ) ) {
            dev_info(&__pdev->dev, "%s: down_interruptible for read faild\n", __func__ );
            return -ERESTARTSYS;
        }
        // align (round up)
        *f_pos = ( *f_pos + sizeof(u32) - 1 ) & ~03;
        struct tsensor_cdev_private * private_data = file->private_data;

        while ( (*f_pos < private_data->size ) && size >= sizeof(u32) ) {
            u32 d;
            if ( copy_from_user( &d, data, sizeof(u32) ) ) {
                up( &drv->sem );
                return -EFAULT;
            }
            size_t idx = *f_pos / (16 * sizeof( u32 ) );
            __slave_data( drv->regs, idx % 16 )->user_datain = d;

            *f_pos += sizeof( u32 );
            count += sizeof( u32 );
            size -= sizeof( u32 );
        }
        up( &drv->sem );
    }
    return count;
}


loff_t
tsensor_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    // struct tsensor_cdev_reader * reader = file->private_data;
    loff_t pos = 0;
    switch( orig ) {
    case 0: // SEEK_SET
        pos = offset;
        break;
    case 1: // SEEK_CUR
        pos = file->f_pos + offset;
        break;
    case 2: // SEEK_END
        pos = ((struct tsensor_cdev_private *)file->private_data)->size + offset;
        break;
    default:
        return -EINVAL;
    }
    // dev_info(&__pdev->dev, "seek offs=%llx, orig=%d, f_pos = %llx -> %llx; ", offset, orig, file->f_pos, pos );
    if ( pos < 0 )
        return -EINVAL;
    file->f_pos = pos;
    return pos;
}


static struct file_operations tsensor_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = tsensor_cdev_llseek
    , .open    = tsensor_cdev_open
    , .release = tsensor_cdev_release
    , .unlocked_ioctl = tsensor_cdev_ioctl
    , .read    = tsensor_cdev_read
    , .write   = tsensor_cdev_write
    , .mmap    = 0
};

static int tsensor_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int tsensor_dtor( int errno )
{
    if ( __tsensor_class ) {
        class_destroy( __tsensor_class );
        printk( KERN_INFO "" MODNAME " class_destry done\n" );
    }

    if ( __tsensor_cdev ) {
        cdev_del( __tsensor_cdev );
        printk( KERN_INFO "" MODNAME " cdev_del done\n" );
    }

    if ( tsensor_dev_t ) {
        unregister_chrdev_region( tsensor_dev_t, 1 );
        printk( KERN_INFO "" MODNAME " unregister_chrdev_region done\n" );
    }

    return errno;
}


static int __init
tsensor_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&tsensor_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __tsensor_cdev = cdev_alloc();
    if ( !__tsensor_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __tsensor_cdev, &tsensor_cdev_fops );
    if ( cdev_add( __tsensor_cdev, tsensor_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return tsensor_dtor( -1 );
    }

    __tsensor_class = class_create( THIS_MODULE, MODNAME );
    if ( !__tsensor_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return tsensor_dtor( -1 );
    }
    __tsensor_class->dev_uevent = tsensor_dev_uevent;

    // make_nod /dev/pio0
    if ( !device_create( __tsensor_class, NULL, tsensor_dev_t, NULL, MODNAME "%d", MINOR( tsensor_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return tsensor_dtor( -1 );
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &tsensor_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
tsensor_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __tsensor_class, tsensor_dev_t ); // device_creeate

    class_destroy( __tsensor_class ); // class_create

    cdev_del( __tsensor_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( tsensor_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n<-----------\n\n", MOD_VERSION );
}

static int
tsensor_perror( struct platform_device * pdev, const char * prefix, void * p )
{
    if ( IS_ERR( p ) ) {
        if ( PTR_ERR( p ) != -EPROBE_DEFER )
            dev_info(&pdev->dev, "%s error code: %ld", prefix, PTR_ERR(p) );
        return PTR_ERR(p);
    }
    return 0;
}

static int
tsensor_module_probe( struct platform_device * pdev )
{
    dev_info( &pdev->dev, "tsensor_module probed [%s], num_resources=%d\n"
              , pdev->name
              , pdev->num_resources );

    if ( platform_get_drvdata( pdev ) == 0 ) {
        struct tsensor_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct tsensor_driver ), GFP_KERNEL );
        if ( ! drv )
            return -ENOMEM;
        __pdev = pdev;
        platform_set_drvdata( pdev, drv );
    }
    struct tsensor_driver * drv = platform_get_drvdata( pdev );

    init_waitqueue_head( &drv->queue );
    drv->queue_condition = 0;
    sema_init( &drv->sem, 1 );

    for ( int i = 0; i < pdev->num_resources; ++i ) {
        struct resource * res = platform_get_resource( pdev, IORESOURCE_MEM, i );
        if ( res ) {
            drv->mem_resource = res;
            drv->regs = devm_platform_ioremap_resource(pdev, i);
            dev_info( &pdev->dev, "res [%d] 0x%08x, 0x%x = %p\n", i, res->start, res->end - res->start + 1, drv->regs );
            tsensor_dg_init( drv );
        }
    }

    //////
    drv->tsensor_sync = devm_gpiod_get_optional( &pdev->dev, "tsensor_sync", GPIOD_IN );
    if ( drv->tsensor_sync && tsensor_perror( pdev, "tsensor_sync", drv->tsensor_sync ) == -EPROBE_DEFER ) {
        u32 rcode;
        struct device_node * node = pdev->dev.of_node;
        if ( of_property_read_u32_index( node, "tsensor_sync-gpios", 1, &drv->legacy_gpio_number ) == 0 ) {
            dev_info(&__pdev->dev, "tsensor_sync-gpios switch to legacy api: numberr %d", drv->legacy_gpio_number );
            if ( ( rcode = devm_gpio_request( &pdev->dev, drv->legacy_gpio_number, "tsensor_sync" ) ) == 0 ) {
                gpio_direction_input( drv->legacy_gpio_number );
            }
        } else {
            dev_err( &pdev->dev, "legacy gpio tsensor_sync request failed: %d", rcode );
        }
    } else {
        dev_err( &pdev->dev, "tsensor_sync gpio get failed: %ld", PTR_ERR( drv->tsensor_sync ) );
    }

    // IRQ
    if ( drv->tsensor_sync && gpio_is_valid( drv->legacy_gpio_number ) ) {
        if ( (drv->irq = gpio_to_irq( drv->legacy_gpio_number ) ) > 0 ) {
            dev_info(&pdev->dev, "\ttsensor_sync gpio irq is %d", drv->irq );
            u32 rcode;
            if ( (rcode = devm_request_irq( &pdev->dev
                                            , drv->irq
                                            , handle_interrupt
                                            , IRQF_SHARED | IRQF_TRIGGER_RISING // FALLING
                                            , "dg"
                                            , pdev )) != 0 ) {
                dev_err( &pdev->dev, "dg irq request failed: %d", rcode );
            }
        }
    }

    return 0;
}

static int
tsensor_module_remove( struct platform_device * pdev )
{
    struct tsensor_driver * drv = platform_get_drvdata( pdev );
    if ( drv && drv->irq ) {
        dev_info( &pdev->dev, "IRQ %d about to be freed\n", drv->irq );
        // devm_free_irq( &pdev->dev, drv->irq, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __tsensor_module_id [] = {
    { .compatible = "tsensor_data" }
    , {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __tsensor_module_id )
        ,
    }
    , .probe = tsensor_module_probe
    , .remove = tsensor_module_remove
};

module_init( tsensor_module_init );
module_exit( tsensor_module_exit );
