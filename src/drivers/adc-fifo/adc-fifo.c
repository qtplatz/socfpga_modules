/**************************************************************************
** Copyright (C) 2017-2021 Toshinobu Hondo, Ph.D.
** Copyright (C) 2017-2021 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include "adc-fifo.h"
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
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
#include <linux/time.h>

static char *devname = MODNAME;

MODULE_AUTHOR( "Toshinobu Hondo" );
MODULE_DESCRIPTION( "DMA Driver for ADC" );
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(devname, MODNAME " param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __adc_fifo_platform_driver;

struct platform_device * __pdevice = 0;

enum dma {
    dmalen = 16*sizeof(u32)  // 512bits
    , dma_bufcount = 32
    , dma_bufsize = dmalen * dma_bufcount
};

struct adc_fifo_driver {
    uint32_t irq;
    const char * label;
    uint64_t irqCount;
    void __iomem * reg_csr;
    void __iomem * reg_descriptor;
    void __iomem * reg_response;   // this is once time access only allowed (destructive read)
    uint32_t phys_source_address;
    uint32_t phys_destination_address;
    wait_queue_head_t queue;
    int queue_condition;
    struct semaphore sem;
    u32 wp;         // next effective dma phys addr
    u32 wlapc;         // write count, steady
    u32 readp;      // fpga read phys addr
    u32 rlapc;
    dma_addr_t dma_handle;
    u32 dma_alloc_size;
    void * dma_vaddr;
    struct dma_pool * dma_pool;
};

struct adc_fifo_cdev_reader {
    struct adc_fifo_driver * drv;
    u32 rp;
    u32 rlapc;
    u32 node;
};

enum { dg_data_irq_mask = 2 };

static int __debug_level__ = 0;

void
adc_fifo_transfer( struct platform_device * pdev
                       , u32 phys_source_address
                       , u32 phys_destination_address
                       , u32 octets
                       , int packet_enabled )
{
    struct adc_fifo_driver * drv = platform_get_drvdata( pdev );
    if ( drv ) {
        // u32 * csr = drv->reg_csr;
        drv->phys_source_address = phys_source_address;
        drv->phys_destination_address = phys_destination_address;

        u32 * descriptor = drv->reg_descriptor;
        descriptor[ 0 ] = phys_source_address;
        descriptor[ 1 ] = phys_destination_address;
        descriptor[ 2 ] = octets;
        u32 Go = 0x80004000 ; // b31 = Go, b14 = IRQ
        if ( packet_enabled )
            Go |= 0x1000; // 'end on eop'
        descriptor[ 3 ] = Go;
    }
}

void
adc_fifo_transfer_r( u32 phys_source_address
                         , u32 phys_destination_address
                         , u32 octets
                         , int packet_enabled )
{
    if ( __pdevice ) {
        adc_fifo_transfer( __pdevice
                           , phys_source_address
                           , phys_destination_address
                           , octets
                           , packet_enabled );
    }
}

void
adc_fifo_transfer_w( u32 phys_source_address
                         , u32 phys_destination_address
                         , u32 octets
                         , int packet_enabled )
{
}

static void
adc_fifo_enable_irq( uint32_t * csr )
{
    csr[1] |= 0x10; // control[bit 4] set
}

static void
adc_fifo_disable_irq( uint32_t * csr )
{
    csr[1] &= ~0x10; // control[bit 4] clear
}

static inline void
adc_fifo_clear_irq( uint32_t * csr )
{
    csr[0] = 0x200;
}

static inline u32
adc_fifo_nextp( struct adc_fifo_driver * drv, u32 cp, u32 * clap )
{
    u32 npos = ( ( cp - drv->dma_handle ) / dmalen ) + 1;

    if ( npos & ~(dma_bufcount - 1) )
        (*clap)++;

    return drv->dma_handle + dmalen * (npos % dma_bufcount);
}

static inline u32
adc_fifo_read_count( struct adc_fifo_driver * drv, u32 cp, u32 clap )
{
    return (( cp - drv->dma_handle ) / dmalen) + ( clap * dma_bufcount );
}

static void
adc_fifo_fetch( struct adc_fifo_driver * drv )
{
    //const int dmalen = 8 * sizeof(u32);  // 12bit nacc, 52bit fpga-tp(50MHz), 8*24bit adc data
    const int packet_enable = 0;

    if ( drv ) {
        adc_fifo_transfer_r( 0x00000000, drv->wp, dmalen /* 64o */, packet_enable );
        drv->wp = adc_fifo_nextp( drv, drv->wp, &drv->wlapc );
    }
}


static irqreturn_t
handle_interrupt( int irq, void *dev_id )
{
    // dev_info( &__pdevice->dev, "handle_interrupt(%d)\n", irq );
    struct adc_fifo_driver * drv = dev_id ? platform_get_drvdata( dev_id ) : 0;
    if ( drv && drv->reg_csr ) {

        adc_fifo_clear_irq( drv->reg_csr );
        u32 resp_length = 0, resp_status = 0;
        volatile const u32 * resp = drv->reg_response;  // should not read twice
        if ( resp ) {
            resp_length = resp[0];
            resp_status = resp[1];
        }

        if ( drv->phys_source_address == 0 && drv->phys_destination_address != 0 ) { // stream -> memory
            drv->readp = drv->phys_destination_address;
            drv->rlapc = drv->wlapc; // at this moment, rp == wp until return from interrupt

            u64 * p64 = (u64 *)((char *)(drv->dma_vaddr) + ( drv->readp - drv->dma_handle ));
            p64[ 7 ] = cpu_to_be64( ktime_get_clocktai_ns() ); // override kernel time
        }

        drv->queue_condition++;
        wake_up_interruptible( &drv->queue );

        adc_fifo_fetch( drv );  // start next dma cycle
    }
    return IRQ_HANDLED;
}

static int
adc_fifo_proc_read( struct seq_file * m, void * v )
{
    seq_printf( m, "adc-fifo debug level = %d\n", __debug_level__ );

    struct adc_fifo_driver * drv = platform_get_drvdata( __pdevice );
    if ( drv ) {
        volatile u32 * csr = drv->reg_csr;
        if ( csr ) {

            uint32_t status = csr[ 0 ];
            uint32_t control = csr[ 1 ];
            uint32_t reg2 = csr[ 2 ];
            uint32_t reg3 = csr[ 3 ];

            seq_printf( m, "%s CSR:%04x %04x %08x, %08x "
                        , drv->label, status, control, reg2, reg3 );

            if ( status & 0x01 )
                seq_printf( m, "Busy," );
            if ( status & 0x02 )
                seq_printf( m, "Desc-empty,");
            if ( status & 0x04 )
                seq_printf( m, "Desc-full,");
            if ( status & 0x08 )
                seq_printf( m, "Resp-empty,");
            if ( status & 0x10 )
                seq_printf( m, "Resp-full,");
            if ( status & 0x20 )
                seq_printf( m, "Stopped,");
            if ( status & 0x40 )
                seq_printf( m, "Resetting,");
            if ( status & 0x80 )
                seq_printf( m, "Stopped on Error,");
            if ( status & 0x100 )
                seq_printf( m, "Stopped on Early Termination,");
            if ( status & 0x200 )
                seq_printf( m, "IRQ,");

            seq_printf( m, "\t");
            if ( control & 0x01 )
                seq_printf( m, "Stop Dispatcher,");
            if ( control & 0x02 )
                seq_printf( m, "Reset Dispatcher,");
            if ( control & 0x04 )
                seq_printf( m, "Stop on Error,");
            if ( control & 0x08 )
                seq_printf( m, "Stop on Early Termination,");
            if ( control & 0x10 )
                seq_printf( m, "Global Interrupt Enable Mask,");
            if ( control & 0x20 )
                seq_printf( m, "Stop Descriptors,");

            seq_printf( m, "\n");
            volatile u32 * desc = drv->reg_descriptor;
            if ( desc ) {
                seq_printf( m, "\tdescriptor: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n"
                            , desc[0], desc[1], desc[2], desc[3] );
            }

            // most recent data
            const u64 * p64 = (const u64 *)((const char * )(drv->dma_vaddr) + ( drv->readp - drv->dma_handle ));
            const u16 * p16 = (const u16 *)(p64);
            const u64 adc_counter = be64_to_cpu( *p64 ) & ~0xfff0000000000000;
            const u16 n_samples = (be16_to_cpu( *p16 ) >> 4) + 1;

            const u8 * dp = (const u8 *)(p64 + 1);
            u32 data[ 8 ];
            for ( int i = 0; i < countof(data); ++i )
                data[ i ] = dp[ i*3 + 0 ] << 16 | dp[ i*3 + 1 ] << 8 | dp[  i*3 + 2 ];

            const u64 * meta_data64 = p64 + 4;
            u64 clock_counter = be64_to_cpu( meta_data64[0] );
            u64 latch_tp = be64_to_cpu( meta_data64[1] );
            u32 flags = be32_to_cpu( (u32)(meta_data64[2] >> 32 ));
            // u32 cmd_latch = be32_to_cpu( (u32)(meta_data64[2] & 0xffffffff ));

            seq_printf( m, "N:%d, data#%lld\ttp:%llx\tflag_tp:%llx\tflags:%x"
                        , n_samples, adc_counter, clock_counter, latch_tp, flags );
            for ( int i = 0; i < 8; ++i )
                seq_printf( m, "\t[%d]%d", i, data[ i ] );

            seq_printf( m, "\n" );
        }
    }
    return 0;
}

static ssize_t
adc_fifo_proc_write( struct file * filep, const char * user, size_t size, loff_t * f_off )
{
    static char readbuf[256];

    if ( size >= sizeof( readbuf ) )
        size = sizeof( readbuf ) - 1;

    if ( copy_from_user( readbuf, user, size ) )
        return -EFAULT;

    readbuf[ size ] = '\0';

    if ( strncmp( readbuf, "recv", 4 ) == 0 && __pdevice ) {

        struct adc_fifo_driver * driver = platform_get_drvdata( __pdevice );
        adc_fifo_fetch( driver );

    } else if ( strncmp( readbuf, "debug", 5 ) == 0 ) {

        __debug_level__++;

    } else if ( strncmp( readbuf, "succinct", 8 ) == 0 ) {

        __debug_level__ = 0;

    }
    return size;
}

static int
adc_fifo_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, adc_fifo_proc_read, NULL );
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)  // not sure, which version is relevant though
static const struct proc_ops proc_file_fops = {
    .proc_open    = adc_fifo_proc_open,
    .proc_read    = seq_read,
    .proc_write   = adc_fifo_proc_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations proc_file_fops = {
    .owner   = THIS_MODULE,
    .open    = adc_fifo_proc_open,
    .read    = seq_read,
    .write   = adc_fifo_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif

static int adc_fifo_cdev_open(struct inode *inode, struct file *file)
{
    unsigned int node = MINOR( inode->i_rdev );

    if ( __pdevice ) {

        struct adc_fifo_cdev_reader * reader = devm_kzalloc( &__pdevice->dev
                                                             , sizeof( struct adc_fifo_cdev_reader ), GFP_KERNEL );
        reader->drv       = platform_get_drvdata( __pdevice );
        reader->node      = node;
        reader->rp        = 0; //reader->driver->readp;  // <- most recent recv data pointer (phys)
        reader->rlapc     = 0;

        file->private_data = reader;

    } else {
        // dev_err( &__pdevice->dev, "failed dma alloc_coherent %p\n", drv->dma_vaddr );
    }

    return 0;
}

static int
adc_fifo_cdev_release( struct inode *inode, struct file *file )
{
    unsigned int node = MINOR( inode->i_rdev );

    struct adc_fifo_cdev_reader * reader = file->private_data;

    if ( reader && __debug_level__ )
        printk( KERN_INFO "cdev_release " MODNAME " node %d, %p, %x\n", node, file->private_data, reader->rp );

    if ( file->private_data && __pdevice )
        devm_kfree( &__pdevice->dev, file->private_data );

    return 0;
}

static long adc_fifo_cdev_ioctl( struct file * file, unsigned int code, unsigned long args )
{
    return 0;
}

static ssize_t adc_fifo_cdev_read( struct file * file, char __user *data, size_t size, loff_t *f_pos )
{
    ssize_t count = 0;
    struct adc_fifo_cdev_reader * reader = file->private_data;
    if ( reader ) {
        struct adc_fifo_driver * drv = reader->drv;
        if ( drv ) {
            if ( down_interruptible( &drv->sem ) ) {
                dev_err( &__pdevice->dev,  "%s: down_interruptible for read faild\n", __func__ );
                return -ERESTARTSYS;
            }
            if ( reader->rp && size >= dmalen ) {
                u32 dev_count = adc_fifo_read_count(drv, drv->readp, drv->rlapc );
                u32 reader_count = adc_fifo_read_count(drv, reader->rp, reader->rlapc );
                if ( dev_count > reader_count ) {
                    do {
                        dev_dbg( &__pdevice->dev,  "rp (%x,%x) != readp (%x,%x)\ttotal: %x, %x\n"
                                 , (reader->rp - drv->dma_handle)/dmalen, reader->rlapc
                                 , (drv->readp - drv->dma_handle)/dmalen, drv->rlapc
                                 , reader_count, dev_count );
                        const void * fp = (const void *)((const char * )(drv->dma_vaddr) + ( reader->rp - drv->dma_handle ));
                        if ( copy_to_user( data, fp, dmalen ) ) {
                            up( &drv->sem );
                            return -EFAULT;
                        }
                        size -= dmalen;
                        count += dmalen;
                        *f_pos += dmalen;
                        data += dmalen;
                        reader->rp = adc_fifo_nextp( drv, reader->rp, &reader->rlapc );
                    } while ( size >= dmalen && dev_count > adc_fifo_read_count( drv, reader->rp, reader->rlapc ) );
                    if ( count ) {
                        up( &drv->sem );
                        dev_info( &__pdevice->dev,  "-- cdev_read %d octets\n", count );
                        return count;
                    }
                }
            }

            wait_event_interruptible( drv->queue, drv->queue_condition != 0 );
            drv->queue_condition = 0;

            if ( size >= dmalen )
                size = dmalen;

            if ( reader->rp == 0 ) {
                reader->rp = drv->readp;
                reader->rlapc = drv->rlapc;
            }

            const void * fp = (const void *)((const char * )(drv->dma_vaddr) + ( reader->rp - drv->dma_handle ));

            if ( copy_to_user( data, fp, size ) ) {
                up( &drv->sem );
                return -EFAULT;
            }

            reader->rp = adc_fifo_nextp( drv, reader->rp, &reader->rlapc );
            count += size;
            data  += size;

            *f_pos += count;
            up( &drv->sem );
        }
    }

    return count;
}

static ssize_t
adc_fifo_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    ssize_t processed = 0;
    return processed;
}

static ssize_t
adc_fifo_cdev_mmap( struct file * file, struct vm_area_struct * vma )
{
#if 0
    const unsigned long length = vma->vm_end - vma->vm_start;
    int ret = (-1);

    struct adc_fifo_driver * drv = platform_get_drvdata( __pdevices[ dma_r ] );
    if ( drv == 0 || drv->dma_handle == 0 ) {
        printk(KERN_CRIT "%s: Couldn't allocate shared memory for user space\n", __func__);
        return -1; // Error
    }

    // Map the allocated memory into the calling processes address space.
    ret = remap_pfn_range( vma
                           , vma->vm_start
                           , ingress_address >> PAGE_SHIFT
                           , length
                           , vma->vm_page_prot );
    if (ret < 0) {
        printk(KERN_CRIT "%s: remap of shared memory failed, %d\n", __func__, ret);
        return(ret);
    }
#endif
    return -1; // Error
}

loff_t
adc_fifo_cdev_llseek( struct file * file, loff_t offset, int orig )
{
	struct adc_fifo_cdev_reader * reader = file->private_data;

	if ( reader ) {
        // orig == 2 && offset == 0
		reader->rp = 0;
    }

    return file->f_pos;
}

static struct file_operations adc_fifo_cdev_fops = {
    .owner   = THIS_MODULE
    , .llseek  = adc_fifo_cdev_llseek
    , .open    = adc_fifo_cdev_open
    , .release = adc_fifo_cdev_release
    , .unlocked_ioctl = adc_fifo_cdev_ioctl
    , .read    = adc_fifo_cdev_read
    , .write   = adc_fifo_cdev_write
    , .mmap    = adc_fifo_cdev_mmap
};

static dev_t adc_fifo_dev_t = 0;
static struct cdev * __adc_fifo_cdev;
static struct class * __adc_fifo_class;

static int msgdma_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0666 );
    return 0;
}

static int msgdma_dtor( int errno )
{
    if ( __adc_fifo_class ) {
        class_destroy( __adc_fifo_class );
        printk( KERN_INFO "" MODNAME " class_destry done\n" );
    }

    if ( __adc_fifo_cdev ) {
        cdev_del( __adc_fifo_cdev );
        printk( KERN_INFO "" MODNAME " cdev_del done\n" );
    }

    if ( adc_fifo_dev_t ) {
        unregister_chrdev_region( adc_fifo_dev_t, 1 );
        printk( KERN_INFO "" MODNAME " unregister_chrdev_region done\n" );
    }

    return errno;
}

static int __init
adc_fifo_module_init( void )
{
    printk( KERN_INFO "" MODNAME " driver %s loaded\n", MOD_VERSION );

    // DEVICE
    if ( alloc_chrdev_region(&adc_fifo_dev_t, 0, 1, MODNAME "-cdev" ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to alloc chrdev region\n" );
        return -1;
    }

    __adc_fifo_cdev = cdev_alloc();
    if ( !__adc_fifo_cdev ) {
        printk( KERN_ERR "" MODNAME " failed to alloc cdev\n" );
        return -ENOMEM;
    }

    cdev_init( __adc_fifo_cdev, &adc_fifo_cdev_fops );
    if ( cdev_add( __adc_fifo_cdev, adc_fifo_dev_t, 1 ) < 0 ) {
        printk( KERN_ERR "" MODNAME " failed to add cdev\n" );
        return msgdma_dtor( -1 );
    }

    __adc_fifo_class = class_create( THIS_MODULE, MODNAME );
    if ( !__adc_fifo_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return msgdma_dtor( -1 );
    }
    __adc_fifo_class->dev_uevent = msgdma_dev_uevent;

    // make_nod /dev/adc-fifo
    if ( !device_create( __adc_fifo_class, NULL, adc_fifo_dev_t, NULL, MODNAME "%d", MINOR( adc_fifo_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return msgdma_dtor( -1 );
    }

    // /proc create
    proc_create( MODNAME, 0666, NULL, &proc_file_fops );

    platform_driver_register( &__adc_fifo_platform_driver );

    printk(KERN_INFO "" MODNAME " registered.\n" );

    return 0;
}

static void
adc_fifo_module_exit( void )
{
    platform_driver_unregister( &__adc_fifo_platform_driver );

    remove_proc_entry( MODNAME, NULL ); // proc_create

    device_destroy( __adc_fifo_class, adc_fifo_dev_t ); // device_creeate

    class_destroy( __adc_fifo_class ); // class_create

    cdev_del( __adc_fifo_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( adc_fifo_dev_t, 1 ); // alloc_chrdev_region
    //

    printk( KERN_INFO "" MODNAME " driver %s unloaded\n", MOD_VERSION );
}

static int
adc_fifo_module_probe( struct platform_device * pdev )
{
    int irq = 0;
    const char * label = 0;

    dev_info( &pdev->dev, "adc_fifo_module_probe [%s], num_resources = %d\n", pdev->name, pdev->num_resources );

    if ( device_property_read_string( &pdev->dev, "label", &label ) == 0 ) {
        if ( strcmp( label, "adc_fifo_0" ) != 0 )
            return -1;
    }

    struct adc_fifo_driver * drv = devm_kzalloc( &pdev->dev, sizeof( struct adc_fifo_driver ), GFP_KERNEL );
    if ( ! drv )
        return -ENOMEM;

    platform_set_drvdata( pdev, drv );
    __pdevice = pdev;
    drv->label = label;
    drv->dma_alloc_size = dma_bufsize;

    // drv->dma_pool = dma_pool_create( MODNAME, &pdev->dev, dmalen, 4, 64 );

    if ( !(drv->dma_vaddr = dma_alloc_coherent( &pdev->dev, dma_bufsize, &drv->dma_handle, GFP_KERNEL )) ) {
        dev_err( &pdev->dev, "failed dma alloc_coherent %p\n", drv->dma_vaddr );
        return -ENOMEM;
    }

    dev_info( &pdev->dev, "dma alloc_coherent %p\t%x\n", drv->dma_vaddr, drv->dma_handle );
    drv->wp = drv->dma_handle; // physical addr

    for ( int i = 0; i < pdev->num_resources; ++i ) {
        struct resource * res = platform_get_resource( pdev, IORESOURCE_MEM, i );
        if ( res ) {
            void __iomem * reg = devm_platform_ioremap_resource(pdev, i);
            dev_info( &pdev->dev, "res [%d] 0x%08x, 0x%x = %p\n", i, res->start, res->end - res->start + 1, reg );
            switch( i ) {
            case 0: drv->reg_csr        = reg; break;
            case 1: drv->reg_descriptor = reg; break;
            case 2: drv->reg_response   = reg; break;
            }
        }
    }

    init_waitqueue_head( &drv->queue );
    drv->queue_condition = 0;
    sema_init( &drv->sem, 1 );

    if ( ( irq = platform_get_irq( pdev, 0 ) ) > 0 ) {
        if ( devm_request_irq( &pdev->dev, irq, handle_interrupt, IRQF_SHARED, MODNAME, pdev ) == 0 ) {
            dev_info( &pdev->dev, "got irq = %d\n", irq );
            drv->irq = irq;
            if ( drv->reg_csr ) {
                adc_fifo_clear_irq( drv->reg_csr );
                adc_fifo_enable_irq( drv->reg_csr );
                irq_set_irq_type( irq, 4 ); // interrpts<0, 44, 4> ; 4 := IRQ_TYPE_LEVEL_HIGH, see include/linux/irq.h
            }

            adc_fifo_fetch( drv );

        } else {
            dev_err( &pdev->dev, "Failed to register IRQ.\n" );
            return -ENODEV;
        }
    }

    return 0;
}

static int
adc_fifo_module_remove( struct platform_device * pdev )
{
    struct adc_fifo_driver * drv = platform_get_drvdata( pdev );

    if ( drv && drv->irq > 0 ) {
        dev_info( &pdev->dev, " %s IRQ %d, %x about to be freed\n", pdev->name, drv->irq, pdev->resource->start );
        if ( drv->reg_csr )
            adc_fifo_disable_irq( drv->reg_csr );
    }

    dma_free_coherent( &pdev->dev, 0x800, drv->dma_vaddr, drv->dma_handle );

    dev_info( &pdev->dev, "Unregistered '%s'\n", drv->label );

    return 0;
}

static const struct of_device_id __adc_fifo_module_id [] = {
    { .compatible = "altr,msgdma-1.0" },
    {}
};

static struct platform_driver __adc_fifo_platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __adc_fifo_module_id )
        ,
    }
    , .probe = adc_fifo_module_probe
    , .remove = adc_fifo_module_remove
};

module_init( adc_fifo_module_init );
module_exit( adc_fifo_module_exit );
