/**************************************************************************
** Copyright (C) 2021 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include "dgmod.h"
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
#include <linux/mm.h>
#include <linux/proc_fs.h> // create_proc_entry
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/ktime.h>

static char *devname = MODNAME;

MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "Driver for the Delay Pulse Generator" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, "dgmod param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static int __debug_level__ = 0;
static struct platform_driver __platform_driver;
static dev_t dgmod_dev_t = 0;
static struct cdev * __dgmod_cdev;
static struct class * __dgmod_class;
static struct platform_device *__pdev;

struct dgmod_driver {
    uint32_t irq;
    uint64_t irqCount_;
    void __iomem * regs;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
    struct resource * mem_resource;
};

struct dgmod_cdev_private {
    u32 node;
    u32 size;
};

static int
dgmod_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int
dgmod_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "debug level = %d\n", __debug_level__ );
    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        seq_printf( m
                    , "dgmod mem resource: %x -- %x, map to %p\n"
                    , drv->mem_resource->start, drv->mem_resource->end, drv->regs );
        const u32 * p = ( const u32 * )drv->regs;
        u32 addr = drv->mem_resource->start;
        for ( u32 i = 0; i < 16; ++i ) {
            seq_printf( m, "%08x: %08x %08x\t%08x %08x\n", addr, p[0], p[1], p[2], p[3] );
            addr += 4 * 4;
            p += 4;
            if ( i == 7 )
                seq_printf( m, "\n" );
        }
    }
    return 0;
}

static ssize_t
dgmod_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    static char readbuf[256];

    if ( size >= sizeof( readbuf ) )
        size = sizeof( readbuf ) - 1;

    if ( copy_from_user( readbuf, user, size ) )
        return -EFAULT;

    readbuf[ size ] = '\0';

    return size;
}

static int
dgmod_proc_open( struct inode * inode, struct file * file )
{
    printk( KERN_INFO "" MODNAME " proc_open private_data=%x\n", (u32)file->private_data );
    return single_open( file, dgmod_proc_read, NULL );
}

static const struct proc_ops dgmod_proc_file_fops = {
    .proc_open    = dgmod_proc_open,
    .proc_read    = seq_read,
    .proc_write   = dgmod_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/////////////////////////////////
/////////////////////////////////

static int dgmod_cdev_open(struct inode *inode, struct file *file)
{
    struct dgmod_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct dgmod_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 64 * 4;
        file->private_data = private_data;
    }

    return 0;

}

static int
dgmod_cdev_release( struct inode *inode, struct file *file )
{
    if ( file->private_data ) {
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long dgmod_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t dgmod_cdev_read( struct file * file, char __user* data, size_t size, loff_t* f_pos )
{
    dev_info( &__pdev->dev, "dgmod_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );

    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( down_interruptible( &drv->sem ) ) {
            dev_info(&__pdev->dev, "%s: down_interruptible for read faild\n", __func__ );
            return -ERESTARTSYS;
        }
        struct dgmod_cdev_private * private_data = file->private_data;

        //size_t dsize = drv->mem_resource->end - (drv->mem_resource->start + *f_pos);
        size_t dsize = private_data->size - *f_pos;
        if ( dsize > size )
            dsize = size;
        if ( copy_to_user( data, (const char *)drv->regs + (*f_pos), dsize ) ) {
            up( &drv->sem );
            return -EFAULT;
        }
        *f_pos += dsize;
        up( &drv->sem );
        return dsize;
    }
    return 0;
}

static ssize_t
dgmod_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    dev_info( &__pdev->dev, "dgmod_cdev_write: fpos=%llx, size=%ud\n", *f_pos, size );
    return size;
}

static ssize_t
dgmod_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
    int ret = (-1);

    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv == 0  ) {
        printk( KERN_CRIT "%s: Couldn't allocate shared memory for user space\n", __func__ );
        return -1; // Error
    }

    // Map the allocated memory into the calling processes address space.
    u32 size = vma->vm_end - vma->vm_start;

    ret = remap_pfn_range( vma
                           , vma->vm_start
                           , drv->mem_resource->start
                           , size
                           , vma->vm_page_prot );
    if (ret < 0) {
        printk(KERN_CRIT "%s: remap of shared memory failed, %d\n", __func__, ret);
        return ret;
    }

    return ret;
}

loff_t
dgmod_cdev_llseek( struct file * file, loff_t offset, int orig )
{
    // struct dgmod_cdev_reader * reader = file->private_data;
    loff_t pos = 0;
    switch( orig ) {
    case 0: // SEEK_SET
        pos = offset;
        break;
    case 1: // SEEK_CUR
        pos = pos + offset;
        break;
    case 2: // SEEK_END
        pos = pos + offset;
        break;
    default:
        return -EINVAL;
    }
    if ( pos < 0 )
        return -EINVAL;
    file->f_pos = pos;
    return pos;
}

static struct file_operations dgmod_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = dgmod_cdev_llseek
    , .open    = dgmod_cdev_open
    , .release = dgmod_cdev_release
    , .unlocked_ioctl = dgmod_cdev_ioctl
    , .read    = dgmod_cdev_read
    , .write   = dgmod_cdev_write
    , .mmap    = dgmod_cdev_mmap
};

static int __init
dgmod_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&dgmod_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __dgmod_cdev = cdev_alloc();
    if ( !__dgmod_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __dgmod_cdev, &dgmod_cdev_fops );
    if ( cdev_add( __dgmod_cdev, dgmod_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return -ENOMEM;
    }

    __dgmod_class = class_create( THIS_MODULE, MODNAME );
    if ( !__dgmod_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return -ENOMEM;
    }
    __dgmod_class->dev_uevent = dgmod_dev_uevent;

    // make_nod /dev/adc_fifo
    if ( !device_create( __dgmod_class, NULL, dgmod_dev_t, NULL, MODNAME "%d", MINOR( dgmod_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return -ENOMEM;
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &dgmod_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
dgmod_module_exit( void )
{
    platform_driver_unregister( &__platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __dgmod_class, dgmod_dev_t ); // device_creeate

    class_destroy( __dgmod_class ); // class_create

    cdev_del( __dgmod_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( dgmod_dev_t, 1 ); // alloc_chrdev_region
    //
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    struct dgmod_driver * drv = dev_id ? platform_get_drvdata( dev_id ) : 0;
    (void)drv;
    dev_info( &__pdev->dev, "dgmod handle_interrupt\n" );
    return IRQ_HANDLED;
}

static int
dgmod_module_probe( struct platform_device * pdev )
{
    int irq = 0;
    struct dgmod_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct dgmod_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    __pdev = pdev;

    dev_info( &pdev->dev, "dgmod_module proved [%s]", pdev->name );

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
    platform_set_drvdata( pdev, drv );

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
dgmod_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __dgmod_module_id [] = {
    { .compatible = "xlnx,delaypulse-generator-1.0" },
    {}
};

static struct platform_driver __platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __dgmod_module_id )
        ,
    }
    , .probe = dgmod_module_probe
    , .remove = dgmod_module_remove
};

module_init( dgmod_module_init );
module_exit( dgmod_module_exit );
