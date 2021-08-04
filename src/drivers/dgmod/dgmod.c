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

#include "dgmod.h"
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
MODULE_PARM_DESC(devname, "dgmod param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __platform_driver;
static struct platform_device *__pdev;
static dev_t dgmod_dev_t = 0;
static struct cdev * __dgmod_cdev;
static struct class * __dgmod_class;
static struct semaphore __sem;
static wait_queue_head_t __queue;
static int __queue_condition;

struct dgmod_driver {
	struct gpio_desc * dg_pio; // t0 trigger
    u32 dg_pio_number;        // legacy gpio number
    u32 irq;
    u32 irqCount;
    void __iomem * regs;
    struct resource * mem_resource;
};

struct dgmod_cdev_private {
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

typedef struct slave_data64 {
    uint64_t user_dataout;    // 0x00
    uint64_t user_datain;     // 0x08
    uint64_t irqmask;         // 0x10
    uint64_t edge_capture;    // 0x18
    uint64_t resv[12];        //
} slave_data64;

inline static volatile slave_data64 * __slave_data64( void __iomem * regs, u32 idx )
{
    return ((volatile slave_data64 *)(regs)) + idx;
}


static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    if ( dev_id == __pdev ) {
        struct dgmod_driver * drv = platform_get_drvdata( __pdev );
        if ( drv->irq == irq ) {
#if 0
            int wp   = __trig_data.wp_++ % __trig_data.cache_size_;
            u64 tp   = __data_2( dg2_timepoint )->user_dataout;

            __trig_data.cache_[ wp ].kTimeStamp_     = (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec; // ktime_to_ns( ktime_get() );
            __trig_data.cache_[ wp ].fTimeStamp_     = tp * 10; // 100MHz precision
            __trig_data.cache_[ wp ].trigCount_      = __data_1( dg1_trigCount )->user_dataout;
            __trig_data.cache_[ wp ].protocolIndex_  = __dgfsm.next_protocol;
            __trig_data.cache_[ wp ].trigErrorCount_ = ++__trigCounter - __trig_data.cache_[ wp ].trigCount_;

            __trig_data.tailp_ = wp;
            dgfsm_handle_irq( &__protocol_sequence, __debug_level__ > 8 ); // start trigger
            drv->tp  = ktime_get_real_ns(); // UTC
#endif
            drv->irqCount++;
            // ++__queue_condition;
            // wake_up_interruptible( &__queue );
            // dev_info(&__pdev->dev, "handle_interrupt inj.in irq %d", irq );
        }
    }
    return IRQ_HANDLED;
}

static int
dgmod_proc_read( struct seq_file * m, void * v )
{
    struct dgmod_driver * drv = platform_get_drvdata( __pdev );

    if ( drv ) {
        seq_printf( m, "proc_read irqCount=%d\n", drv->irqCount );
        for ( int i = 0; i < 16; ++i ) {
            volatile struct slave_data64 * p = __slave_data64( drv->regs, i );
            seq_printf( m, "[%2d] 0x%08x:%08x\t", i, (u32)(p->user_dataout >> 32), (u32)(p->user_dataout & 0xffffffff) );
            seq_printf( m, "%08x:%08x\n", (u32)(p->user_datain >> 32), (u32)(p->user_datain & 0xffffffff) );
        }
    }

    return 0;
}

static void
dgmod_dg_init( struct dgmod_driver * drv )
{
    if ( drv ) {
        __slave_data64( drv->regs, 15 )->user_datain = 1LL << 32; // set page 1 (set point)
        int has_value = 0;
        for ( int i = 1; i < 9; ++i ) {
            if ( __slave_data64( drv->regs,  i )->user_dataout != 0LL ) {
                has_value = 1;
                break;
            }
        }
        if ( ! has_value ) {
            __slave_data64( drv->regs,  0 )->user_datain =    0LL << 32 | 100000LL;   // flags = 0, interval = 1 ms
            __slave_data64( drv->regs,  1 )->user_datain =  100LL << 32 | (0x100LL);  // p0  = 0 ns, 1 us width
            __slave_data64( drv->regs,  2 )->user_datain =  200LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  3 )->user_datain =  300LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  4 )->user_datain =  400LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  5 )->user_datain =  500LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  6 )->user_datain =  600LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  7 )->user_datain =  700LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  8 )->user_datain =  800LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  9 )->user_datain =  900LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 10 )->user_datain = 1000LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 11 )->user_datain = 1100LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 12 )->user_datain = 1200LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 13 )->user_datain = 1300LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 14 )->user_datain = 1400LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
        }
        // set page 0 (actual monitor), and commit value
        __slave_data64( drv->regs, 15 )->user_datain = 0x0LL << 32 | 0x01;
    }
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

    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( strncmp( readbuf, "write", 5 ) == 0 ) {
            __slave_data64( drv->regs,  0 )->user_datain =    0LL << 32 | 100000LL;     // flags = 0, interval = 1 ms
            __slave_data64( drv->regs,  1 )->user_datain =  100LL << 32 | (0x100LL);    // p0  = 0 ns, 1 us width
            __slave_data64( drv->regs,  2 )->user_datain =  200LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  3 )->user_datain =  300LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  4 )->user_datain =  400LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  5 )->user_datain =  500LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  6 )->user_datain =  600LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  7 )->user_datain =  700LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  8 )->user_datain =  800LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs,  9 )->user_datain =  900LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 10 )->user_datain = 1000LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 11 )->user_datain = 1100LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 12 )->user_datain = 1200LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 13 )->user_datain = 1300LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
            __slave_data64( drv->regs, 14 )->user_datain = 1400LL << 32 | (0x100LL);  // p1  = 1 us, 1 us width
        }
        else if ( strncmp( readbuf, "commit", 6 ) == 0 ) {
            __slave_data64( drv->regs, 15 )->user_datain = 0x01;
        }
        else if ( strncmp( readbuf, "p1", 2 ) == 0 ) {
            __slave_data64( drv->regs, 15 )->user_datain = 1LL << 32;
        }
        else if ( strncmp( readbuf, "p0", 2 ) == 0 ) {
            __slave_data64( drv->regs, 15 )->user_datain = 0LL << 32;
        }
    }
    return size;
}

static int
dgmod_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, dgmod_proc_read, NULL );
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)  // not sure, which version is relevant though
static const struct proc_ops dgmod_proc_file_fops = {
    .proc_open    = dgmod_proc_open,
    .proc_read    = seq_read,
    .proc_write   = dgmod_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations dgmod_proc_file_fops = {
    .owner   = THIS_MODULE,
    .open    = dgmod_proc_open,
    .read    = seq_read,
    .write   = dgmod_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

static int dgmod_cdev_open(struct inode *inode, struct file *file)
{
    // dev_info(&__pdev->dev, "dgmod_cdev_open inode=%d", MINOR( inode->i_rdev ) );
    struct dgmod_cdev_private *
        private_data = devm_kzalloc( &__pdev->dev
                                     , sizeof( struct dgmod_cdev_private )
                                     , GFP_KERNEL );
    if ( private_data ) {
        private_data->node =  MINOR( inode->i_rdev );
        private_data->size = 16 * sizeof(u64);
        private_data->mmap = 0;
        file->private_data = private_data;
    }

    return 0;
}

static int dgmod_cdev_release(struct inode *inode, struct file *file)
{
    if ( file->private_data ) {
        // todo -- maybe refcount for mmap ?
        // dev_info(&__pdev->dev, "dgmod_cdev_release inode=%d", MINOR( inode->i_rdev ) );
        devm_kfree( &__pdev->dev, file->private_data );
    }
    return 0;
}

static long dgmod_cdev_ioctl( struct file* file, unsigned int code, unsigned long args)
{
    return 0;
}

static ssize_t dgmod_cdev_read(struct file *file, char __user *data, size_t size, loff_t *f_pos )
{
    ssize_t count = 0;
    dev_info( &__pdev->dev, "dgmod_cdev_read: fpos=%llx, size=%ud\n", *f_pos, size );

    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( down_interruptible( &__sem ) ) {
            dev_info(&__pdev->dev, "%s: down_interruptible for read faild\n", __func__ );
            return -ERESTARTSYS;
        }

        struct dgmod_cdev_private * private_data = file->private_data;

        // 64 bit align
        *f_pos &= ~07;

        if ( *f_pos < private_data->size ) {
            size_t dsize = private_data->size - *f_pos;
            for ( size_t i = *f_pos / sizeof(u64); i < dsize / sizeof(u64); ++i ) {
                u64 d = __slave_data64( drv->regs, i )->user_dataout;
                if ( copy_to_user( data, (const char *)&d, sizeof(u64) ) ) {
                    up( &__sem );
                    return -EFAULT;
                }
                data += sizeof(u64);
            }
            *f_pos += dsize;
            count = dsize;
        }
        up( &__sem );
    }
    return count;
}

static ssize_t dgmod_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    ssize_t count = 0;
    struct dgmod_driver * drv = platform_get_drvdata( __pdev );
    if ( drv ) {
        if ( down_interruptible( &__sem ) ) {
            dev_info(&__pdev->dev, "%s: down_interruptible for read faild\n", __func__ );
            return -ERESTARTSYS;
        }
        // align (round up)
        *f_pos = ( *f_pos + sizeof(u64) - 1 ) & ~07;
        struct dgmod_cdev_private * private_data = file->private_data;
        while ( (*f_pos < private_data->size ) && size >= sizeof(u64) ) {
            u64 d;
            if ( copy_from_user( &d, data, sizeof(u64) ) ) {
                up( &__sem );
                return -EFAULT;
            }
            *f_pos += sizeof( u64 );
            count += sizeof( u64 );
            size -= sizeof( u64 );
        }
        up( &__sem );
    }
    return count;
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
    if ( size > drv->mem_resource->end - drv->mem_resource->start + 1 )
        return -EINVAL;

    dev_info( &__pdev->dev, "vma %lx, %lx, %u, pgoff=%lx\n", vma->vm_start, vma->vm_end, size, vma->vm_pgoff );
    dev_info( &__pdev->dev, "resource %x, %x, %x\n"
              , drv->mem_resource->start, drv->mem_resource->end
              , drv->mem_resource->end - drv->mem_resource->start + 1 );
    struct page * page = virt_to_page( (u32)(drv->regs) + (vma->vm_pgoff << PAGE_SHIFT));
    u32 pfn = page_to_pfn( page );
    dev_info( &__pdev->dev, "pfn: %x\n", pfn );
    if ( ( ret = remap_pfn_range( vma
                                  , vma->vm_start
                                  , pfn
                                  , size
                                  , vma->vm_page_prot ) ) ) {
        dev_err(&__pdev->dev, "%s: remap of shared memory failed, %d\n", __func__, ret);
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

static int dgmod_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int dgmod_dtor( int errno )
{
    if ( __dgmod_class ) {
        class_destroy( __dgmod_class );
        printk( KERN_INFO "" MODNAME " class_destry done\n" );
    }

    if ( __dgmod_cdev ) {
        cdev_del( __dgmod_cdev );
        printk( KERN_INFO "" MODNAME " cdev_del done\n" );
    }

    if ( dgmod_dev_t ) {
        unregister_chrdev_region( dgmod_dev_t, 1 );
        printk( KERN_INFO "" MODNAME " unregister_chrdev_region done\n" );
    }

    return errno;
}


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
        return dgmod_dtor( -1 );
    }

    __dgmod_class = class_create( THIS_MODULE, MODNAME );
    if ( !__dgmod_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return dgmod_dtor( -1 );
    }
    __dgmod_class->dev_uevent = dgmod_dev_uevent;

    // make_nod /dev/pio0
    if ( !device_create( __dgmod_class, NULL, dgmod_dev_t, NULL, MODNAME "%d", MINOR( dgmod_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return dgmod_dtor( -1 );
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &dgmod_proc_file_fops );

    platform_driver_register( &__platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    sema_init( &__sem, 1 );
    init_waitqueue_head( &__queue );
    __queue_condition = 0;

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
    printk( KERN_INFO "" MODNAME " driver %s unloaded\n<-----------\n\n", MOD_VERSION );
}

static int
dgmod_perror( struct platform_device * pdev, const char * prefix, void * p )
{
    if ( IS_ERR( p ) ) {
        if ( PTR_ERR( p ) != -EPROBE_DEFER )
            dev_info(&pdev->dev, "%s error code: %ld", prefix, PTR_ERR(p) );
        return PTR_ERR(p);
    }
    return 0;
}

static int
dgmod_module_probe( struct platform_device * pdev )
{
    dev_info( &pdev->dev, "dgmod_module probed [%s], num_resources=%d\n"
              , pdev->name
              , pdev->num_resources );

    if ( platform_get_drvdata( pdev ) == 0 ) {
        struct dgmod_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct dgmod_driver ), GFP_KERNEL );
        if ( ! drv )
            return -ENOMEM;
        __pdev = pdev;
        platform_set_drvdata( pdev, drv );
    }
    struct dgmod_driver * drv = platform_get_drvdata( pdev );

    for ( int i = 0; i < pdev->num_resources; ++i ) {
        struct resource * res = platform_get_resource( pdev, IORESOURCE_MEM, i );
        if ( res ) {
            drv->mem_resource = res;
            drv->regs = devm_platform_ioremap_resource(pdev, i);
            dev_info( &pdev->dev, "res [%d] 0x%08x, 0x%x = %p\n", i, res->start, res->end - res->start + 1, drv->regs );
            dgmod_dg_init( drv );
        }
    }

    //////
    drv->dg_pio = devm_gpiod_get_optional( &pdev->dev, "dg", GPIOD_IN );
    if ( drv->dg_pio && dgmod_perror( pdev, "dg", drv->dg_pio ) == -EPROBE_DEFER ) {
        u32 rcode;
        struct device_node * node = pdev->dev.of_node;
        if ( of_property_read_u32_index( node, "dg-gpios", 1, &drv->dg_pio_number ) == 0 ) {
            dev_info(&__pdev->dev, "dg_pio-gpios switch to legacy api: numberr %d", drv->dg_pio_number );
            if ( ( rcode = devm_gpio_request( &pdev->dev, drv->dg_pio_number, "dg" ) ) == 0 ) {
                gpio_direction_input( drv->dg_pio_number );
            }
        } else {
            dev_err( &pdev->dev, "legacy gpio dg_pio request failed: %d", rcode );
        }
    } else {
        dev_err( &pdev->dev, "dg gpio get failed: %ld", PTR_ERR( drv->dg_pio ) );
    }

    // IRQ
    if ( drv->dg_pio && gpio_is_valid( drv->dg_pio_number ) ) {
        if ( (drv->irq = gpio_to_irq( drv->dg_pio_number ) ) > 0 ) {
            dev_info(&pdev->dev, "\tdg gpio irq is %d", drv->irq );
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
dgmod_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        dev_info( &pdev->dev, "IRQ %d about to be freed\n", irqNumber );
        free_irq( irqNumber, 0 );
    }
    __pdev = 0;
    return 0;
}

static const struct of_device_id __dgmod_module_id [] = {
    { .compatible = "altr,avalon-mm-slave-2.0" }
    , {}
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
