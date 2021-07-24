/**************************************************************************
** Copyright (C) 2016-2018 MS-Cheminformatics LLC, Toin, Mie Japan
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
#include "dgfsm.h"
#include "dgmod_delay_pulse.h"
#include "dgmod_ioctl.h"
#include "hps.h"
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
MODULE_PARM_DESC(devname, "dgmod param");
module_param( devname, charp, S_IRUGO );

#define countof(x) (sizeof(x)/sizeof((x)[0]))

static struct platform_driver __slave_platform_driver;

struct bridge {
    uint32_t reg[4];
};

static u32 __gpio_user_led = gpio_user_led;
static struct bridge __bridge;

struct slave_device {
    uint32_t reg[ 3 ];
    volatile uint32_t * addr;
};

static struct slave_device __slave_device[ 3 ] = { 0 };

static int __irqNumber;
static int __button_irq [2];
static int __dipsw_irq [4];
static uint32_t __trigCounter = 0;

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

static struct region_allocated __resources = {  .size = 0, .regions = { { 0, 0 } } };

struct gpio_register {
    const char * name;
    const char * gpio_name;
    int gpio; // base
    uint32_t count;
    uint32_t regs[ 3 ];
    uint32_t start;
    volatile uint32_t * addr;
};

static struct gpio_register __gpio__[] = {
    { .name = "button_pio",  .gpio_name = "dgmod-button_pio", .gpio = 0, .count = 0 }
    , { .name = "pio_0",     .gpio_name = "dgmod-pio_0",      .gpio = 0, .count = 0 }
    , { .name = "dipsw_pio", .gpio_name = "dgmod-dipsw_pio",  .gpio = 0, .count = 0 }
};

// <-- 2017-APR-24

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
    struct dgmod_trigger_data * cache_;
    uint32_t cache_size_;
    uint32_t rp_;
    uint32_t wp_;
    uint32_t tailp_;
};

static struct irq_allocated __irq = {
    .size = 0
    , .irqNumber = { 0 }
};

static struct gpio_allocated __gpio_allocated = {
    .size = 0
};

static struct trig_data __trig_data = {
    .cache_size_ = 1024 * 16 // 1.6s @ 10kHz
    , .rp_ = 0
    , .wp_ = 0
    , .tailp_ = 0
};

static int __debug_level__ = 0;

struct pulse_register { uint32_t raising; uint32_t falling; };

static struct dgmod_protocol_sequence __protocol_sequence = {
    .interval_ = 100000 * 1, // (1ms @ 10ns resolution)
    .size_ = 1,
    .protocols_[ 0 ].replicates_ = 1,
    .protocols_[ 0 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 0, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 1 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 1, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 2 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 2, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 3 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 3, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
};

static struct dgmod_protocol_sequence __next_protocol_sequence = {
    .interval_ = 100000 * 1, // (1ms @ 10ns resolution)
    .size_ = 1,
    .protocols_[ 0 ].replicates_ = 1,
    .protocols_[ 0 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 0, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 1 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 1, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 2 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 2, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
    .protocols_[ 3 ].delay_pulses_ = { { 100, 10 }, { 0, 100 }, { 100, 10 }, { 3, 0 }, { 0, 0 }, { 100, 10 }, { 0, 0 } },
};

static struct dgfsm __dgfsm = {
    .state = fsm_stop,
    .next_protocol = 0,
    .replicates_remain = (-1)
};

static int  dgfsm_handle_irq( const struct dgmod_protocol_sequence * sequence, int verbose );
static int  dgfsm_init( const struct dgmod_protocol_sequence * sequence );
static void dgfsm_start(void);
static void dgfsm_stop(void);

inline static volatile struct dg_data * __data_0( enum dg0_reg idx )
{
    return ((struct dg_data *)( __slave_device[ 0 ].addr )) + idx;
}

inline static volatile struct dg_data * __data_1( enum dg1_reg idx )
{
    return ((struct dg_data *)( __slave_device[ 1 ].addr )) + idx;
}

inline static volatile struct dg_data64 * __data_2( enum dg2_reg idx )
{
    return ((struct dg_data64 *)( __slave_device[ 2 ].addr )) + idx;
}

inline static void irq_mask( struct gpio_register * pio, int mask )
{
    if ( pio->addr )
        pio->addr[ 2 ] = mask;
}

static int populate_device_tree( void )
{
    struct device_node * node;
    struct device_node * child;

    // trial to find hps_led0
    if ( ( node = of_find_node_by_path( "/sopc@0/leds/hps0" ) ) ) {
        const char * label = 0;
        u32 value[ 3 ];

        if ( ( of_property_read_string_index( node, "label", 0, &label ) == 0 ) ) {
            if ( of_property_read_u32_array( node, "gpios", value, 3 ) >= 0 ) {
                u32 gpio = 0;
                printk( KERN_INFO "" MODNAME "\tGPIO# found: %s, %d, %d, %d\n", label, value[0], value[1], value[2] );
                if ( of_property_read_u32( node, "gpio", &gpio ) >= 0 ) {
                    __gpio_user_led = gpio + value[1];
                }
            }
        }
    }

    if ( ( node = of_find_node_by_path( "/sopc@0/bridge@0xc0000000" ) ) ||
         ( node = of_find_node_by_path( "/sopc@0/bridge@c0000000" ) ) ) {

        for ( int i = 0; i < countof( __bridge.reg ); i += 2 ) {
            if ( of_property_read_u32_index( node, "reg", i, &__bridge.reg[ i ] ) == 0 &&
                 of_property_read_u32_index( node, "reg", i + 1, &__bridge.reg[ i + 1 ] ) == 0 ) {
                printk(KERN_INFO "" MODNAME " %s <0x%08x, 0x%08x>\n", node->full_name, __bridge.reg[ i ], __bridge.reg[ i + 1 ] );
            }
        }

        for_each_child_of_node( node, child ) {

            if ( strcmp( child->name, "gpio" ) == 0 ) {

                const char * label = 0;
                u32 value[3];

                if ( ( of_property_read_string_index( child, "label", 0, &label ) == 0 ) &&
                     ( of_property_read_u32_array( child, "reg", value, 3 ) >= 0 ) ) {

                    for ( int i = 0; i < countof( __gpio__ ); ++i ) {
                        if ( strcmp( label, __gpio__[ i ].name ) == 0 ) {
                            struct gpio_register * p = &__gpio__[ i ];
                            p->regs[ 0 ] = value[ 0 ];
                            p->regs[ 1 ] = value[ 1 ]; // offset addr
                            p->regs[ 2 ] = value[ 2 ];
                            p->start = __bridge.reg[ value[ 0 ] * 2 ] + value[ 1 ];
                            do {
                                u32 gpioNumber[ 2 ];
                                if ( of_property_read_u32_array( child, "gpios", gpioNumber, 2 ) >= 0 ) {
                                    p->gpio = gpioNumber[ 0 ];
                                    p->count = gpioNumber[ 1 ];
                                } else {
                                    printk( KERN_ALERT "" MODNAME " \tError: No GPIO# found: using default value %s\t<%d,%d>\n"
                                            , label, p->gpio, p->count );
                                }
                            } while(0);
                        }
                    }
                }

            } else if ( strcmp( child->name, "slave" ) == 0 ) { // slave template

                const char * label = 0;
                u32 reg[3];

                of_property_read_string_index( child, "label", 0, &label );

                if ( of_property_read_u32_array( child, "reg", reg, 3 ) >= 0 ) {
                    if ( label ) {
                        int idx = 0;
                        if ( strcmp( label, "dg_data_0" ) == 0 ) {
                            idx = 0;
                        } else if ( strcmp( label, "dg_data_1" ) == 0 ) {
                            idx = 1;
                        } else if ( strcmp( label, "dg_data_2" ) == 0 ) {
                            idx = 2;
                        }
                        if ( idx >= 0 ) {
                            __slave_device[ idx ].reg[ 0 ]  = reg[ 0 ];
                            __slave_device[ idx ].reg[ 1 ]  = reg[ 1 ];
                            __slave_device[ idx ].reg[ 2 ]  = reg[ 2 ];
                            printk(KERN_INFO "" MODNAME " \tdata_%d\t<0x%x, 0x%x, 0x%x>\n", idx, reg[ 0 ], reg[ 1 ], reg[ 2 ] );
                        }
                    }
                }

            } else {
                //printk(KERN_INFO "\t\tunhandled: { %s }\n", child->full_name );
            }
        }
        return 0;
    }
    return -1;
}

static irqreturn_t
handle_interrupt(int irq, void *dev_id)
{
    struct timespec ts;
    getnstimeofday( &ts );

    if ( irq == __irqNumber ) {

        int wp   = __trig_data.wp_++ % __trig_data.cache_size_;
        u64 tp   = __data_2( dg2_timepoint )->user_dataout;

        __trig_data.cache_[ wp ].kTimeStamp_     = (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec; // ktime_to_ns( ktime_get() );
        __trig_data.cache_[ wp ].fTimeStamp_     = tp * 10; // 100MHz precision
        __trig_data.cache_[ wp ].trigCount_      = __data_1( dg1_trigCount )->user_dataout;
        __trig_data.cache_[ wp ].protocolIndex_  = __dgfsm.next_protocol;
        __trig_data.cache_[ wp ].trigErrorCount_ = ++__trigCounter - __trig_data.cache_[ wp ].trigCount_;

        __trig_data.tailp_ = wp;

        __trig_queue_condition = 1;
        wake_up_interruptible( &__trig_queue );

        dgfsm_handle_irq( &__protocol_sequence, __debug_level__ > 8 ); // start trigger

        if ( ( __trigCounter & 0xff ) == 0 )
            gpio_set_value( __gpio_user_led, ( __trigCounter >> 8 ) & 01 );

    } else if ( irq == __button_irq[ 0 ] ) {
        __data_1( dg1_fsm_start )->user_datain = fsm_start;
        __dgfsm.state = 1;
        printk(KERN_INFO "" MODNAME " handle key irq %d -- fsm started.\n", irq );

    } else if ( irq == __button_irq[ 1 ] ) {
        dgfsm_stop();
        printk(KERN_INFO "" MODNAME " handle key irq %d -- fsm stopped.\n", irq );

    } else if ( ( irq >= __dipsw_irq[ 0 ] ) && ( irq <= __dipsw_irq[ 3 ] ) ) {
        int idx = irq - __dipsw_irq[ 0 ];
        struct gpio_register * p = &__gpio__[ 2 ]; // dipsw
        int value = gpio_get_value( p->gpio + idx );
        printk( KERN_INFO "" MODNAME " dipsw[%d] --> %x\n", idx, value );
        // __data_1( idx )->irqmask = value;

    } else {
        if ( __debug_level__ )
            printk(KERN_INFO "" MODNAME " handle irq %d\n", irq );
    }

    return IRQ_HANDLED;
}

static irqreturn_t
handle_slave_interrupt( int irq, void *dev_id )
{
    if ( __debug_level__ ) {
        printk(KERN_INFO "" MODNAME " handle slave irq %d\n", irq );
    }
    return IRQ_HANDLED;
}

static void dgfsm_start( void )
{
    dgfsm_init( &__protocol_sequence );
    dgfsm_handle_irq( &__protocol_sequence, __debug_level__ > 8 ); // commit trigger

    __data_1( dg1_fsm_start )->user_datain = fsm_start;
    __dgfsm.state = 1;
}

static void dgfsm_stop( void )
{
    __dgfsm.state = 0;
    __data_1( dg1_fsm_start )->user_datain = fsm_stop;
}

static void dgfsm_commit( const struct dgmod_protocol * proto, int protocolIndex )
{
    if ( __debug_level__ >= 1 && ( __data_0( dg0_period )->user_datain != __protocol_sequence.interval_ ) )
        printk(KERN_INFO "" MODNAME " dgfsm_commit To changed: %d\n", __protocol_sequence.interval_ );

    __data_0( dg0_period )->user_datain = __protocol_sequence.interval_;

    for ( size_t ch = 0; ch < 6 /*number_of_channels*/; ++ch ) {
        __data_0( dg0_delay_0_up + (ch * 2) )->user_datain = proto->delay_pulses_[ ch ].delay_;
        __data_0( dg0_delay_0_down + (ch * 2) )->user_datain = proto->delay_pulses_[ ch ].delay_ + proto->delay_pulses_[ ch ].width_;
    }

    __data_1( 6 )->user_datain = proto->delay_pulses_[ 6 ].delay_;
    __data_1( 7 )->user_datain = proto->delay_pulses_[ 6 ].width_; // this channel using delay,width pair

    __data_1( dg1_protocol )->user_datain = protocolIndex;

    __data_1( dg1_fsm_commit )->user_datain = 0;
    __data_1( dg1_fsm_commit )->user_datain = 0;
    __data_1( dg1_fsm_commit )->user_datain = 1;
}

static int dgfsm_init( const struct dgmod_protocol_sequence * sequence )
{
    if ( sequence ) {

        if ( __debug_level__ > 5 ) {
            printk( KERN_INFO "" MODNAME " dgfsm_init: sequence count= %d, To=%d", sequence->size_, sequence->interval_ );
            for ( int i = 0; i < sequence->size_; ++i ) {
                printk( KERN_INFO "" MODNAME " rep: %d { %d, %d },  { %d, %d },  { %d, %d },  { %d, %d },  { %d, %d },  { %d, %d }, {%d, %d}\n"
                        , sequence->protocols_[ i ].replicates_
                        , sequence->protocols_[ i ].delay_pulses_[ 0 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 0 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 1 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 1 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 2 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 2 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 3 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 3 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 4 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 4 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 5 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 5 ].width_
                        , sequence->protocols_[ i ].delay_pulses_[ 6 ].delay_, sequence->protocols_[ i ].delay_pulses_[ 6 ].width_
                    );
            }
        }

        __dgfsm.next_protocol = 0;
        __dgfsm.number_of_protocols =
            sequence->size_ <= countof( sequence->protocols_ ) ?
            sequence->size_ : countof( sequence->protocols_ );

        __dgfsm.replicates_remain = sequence->protocols_[ 0 ].replicates_;

    }

    return 0;
}

static int dgfsm_handle_irq( const struct dgmod_protocol_sequence * sequence, int verbose )
{
    if ( verbose ) {
        printk( KERN_INFO "" MODNAME " \tdgfsm_handle_irq: trig left:%u proto# %u/%u\tts: 0x%x:%08x\ttrig#=0x%08x\n"
                , __dgfsm.replicates_remain
                , __dgfsm.next_protocol
                , __dgfsm.number_of_protocols
                , __data_1( dg1_timestampH )->user_dataout
                , __data_1( dg1_timestampL )->user_dataout
                , __data_1( dg1_trigCount )->user_dataout );
    }

    if ( __dgfsm.replicates_remain && ( --__dgfsm.replicates_remain == 0 ) ) {

        if ( ++__dgfsm.next_protocol >= __dgfsm.number_of_protocols ) {
            __dgfsm.next_protocol = 0;
        }

        do {

            const struct dgmod_protocol * proto = &sequence->protocols_[ __dgfsm.next_protocol ];
            __dgfsm.replicates_remain = proto->replicates_;

            dgfsm_commit( proto, __dgfsm.next_protocol );

            if ( verbose )
                printk( KERN_INFO "" MODNAME " dgfsm_commit: #%d\n", __dgfsm.next_protocol );

        } while ( 0 );

    }

    return 0;
}

static void *
dgmod_request_mem_region( uint32_t start, size_t size, const char * name )
{
    struct resource * resource = request_mem_region( start, size, name );
    if ( resource ) {
        size_t idx = __resources.size++;
        __resources.regions[ idx ].resource_ = resource; // for dtor
        __resources.regions[ idx ].iomem_ = ioremap( resource->start, resource->end - resource->start + 1 );

        printk(KERN_INFO "" MODNAME " \trequested memory resource: %x, %x, %s\n", resource->start, resource->end, resource->name );

        if ( __resources.size >= sizeof( __resources.regions )/sizeof( __resources.regions[ 0 ] ) )
            printk(KERN_CRIT "" MODNAME " \tinsifficient memory region\n");

        return __resources.regions[ idx ].iomem_;

    } else {
        printk(KERN_CRIT "" MODNAME " \tfailed to allocate mem reagion: %x size %x for %s\n", start, size, name );
    }
    return 0;
}


static void
dgmod_release_regions( void )
{
    while ( __resources.size-- ) {
        size_t idx = __resources.size;

        struct resource * resource = __resources.regions[ idx ].resource_;

        if ( resource ) {

            iounmap( __resources.regions[ idx ].iomem_ );

            release_mem_region( resource->start, resource->end - resource->start + 1 );

            printk( KERN_INFO "" MODNAME " \trelease_mem_reagion[%d] %s( %x, %x )\n"
                    , idx, resource->name, resource->start, resource->end - resource->start + 1 );
        }
    }
}

static int
dgmod_proc_read( struct seq_file * m, void * v )
{
    if ( __data_2( 0 ) ) {
        for ( int i = 0; i < 4; i += 2 ) {
            struct slave_device * p = &__slave_device[ 2 ];
            uint32_t addr = p->reg[ 1 ] + (uint32_t)(((char *)(__data_2( i )) - (char *)p->addr));
            uint64_t counter = __data_2( i )->user_dataout;
            uint64_t fast_counter = __data_2( i + 1 )->user_dataout;
            seq_printf( m, "dg_data_2[0x%x]\t0x%016llx, 0x%016llx\t(%lld)\n"
                        , addr
                        , counter                    // 100MHz counter
                        , fast_counter               // 200MHz counter
                        , fast_counter - counter * 2 // delta
                );
        }
    }

    // dump dg_data_x
    if ( __debug_level__ >= 2 ) {
        for ( int i = 0; i < 16; i += 4 )
            seq_printf( m, "dg_data_0[0x%08x]\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n"
                        , __slave_device[ 0 ].reg[ 1 ] + (uint32_t)((char *)(__data_0(i)) - (char *)__data_0(0))
                        , __data_0( i )->user_dataout
                        , __data_0( i + 1 )->user_dataout
                        , __data_0( i + 2 )->user_dataout
                        , __data_0( i + 3 )->user_dataout );
    }
    if ( __debug_level__ >= 1 ) {
        for ( int i = 0; i < 16; i += 4 )
            seq_printf( m, "dg_data_1[0x%08x]\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n"
                        , __slave_device[ 1 ].reg[ 1 ] + (uint32_t)((char *)(__data_1(i)) - (char *)__data_1(0))
                        , __data_1( i )->user_dataout
                        , __data_1( i + 1 )->user_dataout
                        , __data_1( i + 2 )->user_dataout
                        , __data_1( i + 3 )->user_dataout );
    }
    seq_printf( m
                , "Delay pulse generator [0x%x] Rev. 0x%x\n\tTo: 0x%08x Start:0x%08x\tSubmission: 0x%04x\n"
                , __data_0( dg_reg_model )->user_dataout
                , __data_0( dg0_reg_revision )->user_dataout
                , __data_0( dg0_period )->user_dataout
                , __data_1( dg1_fsm_start )->user_dataout
                , __data_1( dg1_fsm_commit )->user_dataout );

    static char * channels [] = { "PUSH ", "INJ  ", "EXIT ", "GATE1", "GATE2", "ADC  " };
    for ( size_t i = 0; i < 6; ++i ) {
        uint32_t delay = __data_0( dg0_delay_0_up + ( i * 2 ) )->user_dataout;
        uint32_t width = __data_0( dg0_delay_0_down + ( i * 2 ) )->user_dataout - delay;
        seq_printf( m, "CH-%s: 0x%08x, 0x%08x\n", channels[i], delay, width );
    }
    { // exit2
        uint32_t delay = __data_1( dg1_6 )->user_dataout;
        uint32_t width = __data_1( dg1_7 )->user_dataout;
        seq_printf( m, "CH-EXIT2: 0x%08x, 0x%08x\n", delay, width );
    }

    if ( __protocol_sequence.size_ > 0 ) {
        seq_printf( m, "Has %d protocol sequence, To = %d\n"
                    , __protocol_sequence.size_, __protocol_sequence.interval_ );

        for ( size_t i = 0; i < __protocol_sequence.size_; ++i ) {
            seq_printf( m, "[%d] Replicates: %d\n", i, __protocol_sequence.protocols_[ i ].replicates_ );
            for ( size_t ch = 0; ch < number_of_channels; ++ch )
                seq_printf( m
                            , "\t[CH-%d] 0x%08x, 0x%08x\n"
                            , ch
                            , __protocol_sequence.protocols_[ i ].delay_pulses_[ ch ].delay_
                            , __protocol_sequence.protocols_[ i ].delay_pulses_[ ch ].width_ );
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

    if ( strncmp( readbuf, "start", 5 ) == 0 ) {
        dgfsm_start();
        if ( __debug_level__ > 1 )
            printk( KERN_INFO "" MODNAME " fsm started.\n" );

    } else if ( strncmp( readbuf, "stop", 4 ) == 0 ) {
        dgfsm_stop();
        if ( __debug_level__ > 1 )
            printk( KERN_INFO "" MODNAME " fsm stopped.\n" );

    } else if ( strncmp( readbuf, "commit", 6 ) == 0 ) {
        dgfsm_commit( &__protocol_sequence.protocols_[ 0 ], 0 );

    } else if ( strncmp( readbuf, "data1,6", 7 ) == 0 ) {
        unsigned long value = 0;
        const char * rp = &readbuf[7];
        while ( *rp && !isdigit( *rp ) )
            ++rp;
        if ( kstrtoul( rp, 0, &value ) == 0 ) {
            __data_1( 6 )->user_datain = value;
            printk( KERN_INFO "" MODNAME " data1,6 is %lu\n", value );
        }
    } else if ( strncmp( readbuf, "data1,7", 7 ) == 0 ) {
        unsigned long value = 0;
        const char * rp = &readbuf[7];
        while ( *rp && !isdigit( *rp ) )
            ++rp;
        if ( kstrtoul( rp, 0, &value ) == 0 ) {
            __data_1( 7 )->user_datain = value;
            printk( KERN_INFO "" MODNAME " data1,7 is %lu\n", value );
        }
    } else if ( strncmp( readbuf, "debug", 5 ) == 0 ) {

        unsigned long value = 0;
        const char * rp = &readbuf[5];
        while ( *rp && !isdigit( *rp ) )
            ++rp;
        if ( kstrtoul( rp, 10, &value ) == 0 )
            __debug_level__ = value;

        printk( KERN_INFO "" MODNAME " debug level is %d\n", __debug_level__ );

    } else {
        printk( KERN_INFO "" MODNAME " proc write received unknown command[%d]: %s.\n", size, readbuf );
        printk( KERN_INFO "" MODNAME " known commands: 'start', 'stop', 'commit', 'on', 'off', 'debug ###'\n" );
    }

    return size;
}

static int
dgmod_proc_open( struct inode * inode, struct file * file )
{
    return single_open( file, dgmod_proc_read, NULL );
}

static const struct file_operations proc_file_fops = {
    .owner   = THIS_MODULE,
    .open    = dgmod_proc_open,
    .read    = seq_read,
    .write   = dgmod_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int dgmod_cdev_open(struct inode *inode, struct file *file)
{
    if ( __debug_level__ )
        printk(KERN_INFO "" MODNAME " open dgmod char device\n");

    return 0;
}

static int dgmod_cdev_release(struct inode *inode, struct file *file)
{
    if ( __debug_level__ )
        printk(KERN_INFO "" MODNAME " release dgmod char device\n");

    return 0;
}

static long dgmod_cdev_ioctl( struct file* file, unsigned int code, unsigned long args)
{
    const static int version = IOCTL_VERSION;

    switch( code ) {

    case DGMOD_GET_VERSION:
        if ( copy_to_user( (char *)(args), ( const void * )(&version), sizeof( int ) ) )
            return -EFAULT;
        break;

    case DGMOD_GET_PROTOCOLS:
        if ( copy_to_user( (char *)(args), ( const void * )(&__protocol_sequence), sizeof( __protocol_sequence ) ) )
            return -EFAULT;
        break;

    case DGMOD_SET_PROTOCOLS:
        if ( copy_from_user( (char *)(&__next_protocol_sequence), (const void *)(args), sizeof( __next_protocol_sequence ) ) == 0 ) {

            if ( __debug_level__ >= 100 )
                __next_protocol_sequence.interval_ = 100000 * __debug_level__; // 100000 := 1ms

            __protocol_sequence = __next_protocol_sequence;

            dgfsm_init( &__protocol_sequence ); // it will be loaded at next irq

            if ( __dgfsm.state == 0 ) { // fsm 'stopped'
                dgfsm_handle_irq( &__protocol_sequence, __debug_level__ > 5 ); // force commit
                dgfsm_start();
            }
        } else
            return -EFAULT;
        break;

    case DGMOD_GET_DEVICE_DATA:
        do {
            struct dgmod_device_data d = { __data_0( dg_reg_model )->user_dataout, __data_0( dg0_reg_revision )->user_dataout };
            if ( copy_to_user( (char *)(args), ( const void * )(&d), sizeof( d ) ) )
                return -EFAULT;
        } while( 0 );
        break;
    case DGMOD_GET_TRIGGER_DATA:
        do {
            struct dgmod_trigger_data * p = &__trig_data.cache_[ __trig_data.tailp_ % __trig_data.cache_size_ ];

            if ( __debug_level__ > 4 )
                printk( KERN_INFO "" MODNAME " proto#%d, trig=%d/%d, FT:%lld\n", p->protocolIndex_, p->trigCount_, __trigCounter, p->fTimeStamp_);

            if ( copy_to_user( (char *)(args), ( const void * )(p), sizeof( struct dgmod_trigger_data ) ) )
                return -EFAULT;
        } while ( 0 );
        break;
    case DGMOD_SET_DG_DATA_0:
    case DGMOD_SET_DG_DATA_1:
        do {
            volatile struct dg_data * dp = ( code == DGMOD_SET_DG_DATA_0 ) ? __data_0( 0 ) : __data_1( 0 );
            struct dgmod_dg_data d;
            if ( copy_from_user( (char *)(&d), (const void *)(args), sizeof( d ) ) || d.index_ >= 16 )
                return -EFAULT;
            dp[ d.index_ ].user_datain = d.user_data_;
        } while(0);
        break;
    case DGMOD_GET_DG_DATA_0:
    case DGMOD_GET_DG_DATA_1:
        do {
            volatile struct dg_data * dp = ( code == DGMOD_GET_DG_DATA_0 ) ? __data_0( 0 ) : __data_1( 0 );
            struct dgmod_dg_data d;
            if ( copy_from_user( (char *)(&d), (const void *)(args), sizeof( d ) ) || d.index_ >= 16 )
                return -EFAULT;
            d.user_data_ = dp[ d.index_ ].user_dataout;
            if ( copy_to_user( (char *)args, ( const void * )(&d), sizeof( d ) ) )
                return -EFAULT;
        } while(0);
        break;
    case DGMOD_SET_DG_DATA_2:
        do {
            struct dgmod_dg_data64 d;
            if ( copy_from_user( (char *)(&d), (const void *)(args), sizeof( d ) ) || d.index_ >= 16 )
                return -EFAULT;
            __data_2( d.index_ )->user_datain = d.user_data_;
        } while(0);
        break;
    case DGMOD_GET_DG_DATA_2:
        do {
            struct dgmod_dg_data64 d;
            if ( copy_from_user( (char *)(&d), (const void *)(args), sizeof( d ) ) || d.index_ >= 16 )
                return -EFAULT;
            d.user_data_ = __data_2( d.index_ )->user_dataout;
            if ( copy_to_user( (char *)args, ( const void * )(&d), sizeof( d ) ) )
                return -EFAULT;
        } while(0);
        break;
    }
    return 0;
}

static ssize_t dgmod_cdev_read(struct file *file, char __user *data, size_t size, loff_t *f_pos )
{
    size_t count = 0;

    if ( __debug_level__ > 6 )
        printk(KERN_INFO "dgmod_cdev_read size=%d, offset=%lld wp=%d, rp=%d\n", size, *f_pos, __trig_data.wp_, __trig_data.rp_ );

    if ( down_interruptible( &__sem ) ) {
        printk(KERN_INFO "down_interruptible for read faild\n" );
        return -ERESTARTSYS;
    }

    wait_event_interruptible( __trig_queue, __trig_queue_condition != 0 );
    __trig_queue_condition = 0;

    do {
        uint32_t wp = __trig_data.wp_;

        if ( wp - __trig_data.rp_ > __trig_data.cache_size_ )
            __trig_data.rp_ = wp - __trig_data.cache_size_;

        while ( ( __trig_data.rp_ != wp ) && ( size >= sizeof( struct dgmod_trigger_data ) ) ) {

            struct dgmod_trigger_data * sp = &__trig_data.cache_[ __trig_data.rp_++ % __trig_data.cache_size_ ];

            if ( copy_to_user( data, ( const void * )(sp), sizeof( struct dgmod_trigger_data ) ) )
                return -EFAULT;

            count += sizeof( struct dgmod_trigger_data );
            data += sizeof( struct dgmod_trigger_data );
            size -= sizeof( struct dgmod_trigger_data );
        }
    } while ( 0 );

    *f_pos += count;

    up( &__sem );

    return count;
}

static ssize_t dgmod_cdev_write(struct file *file, const char __user *data, size_t size, loff_t *f_pos)
{
    static struct dgmod_protocol_sequence protocol_sequence;
    size_t count = sizeof( protocol_sequence ) - ( *f_pos );

    if ( __debug_level__ )
        printk(KERN_INFO "dgmod_cdev_write size=%d\n", size );

    if ( *f_pos >= sizeof( protocol_sequence ) ) {
        printk(KERN_INFO "dgmod_cdev_write size overrun size=%d, offset=%lld\n", size, *f_pos );
        return 0;
    }

    if ( copy_from_user( (char *)(&protocol_sequence) + *f_pos, data, count ) )
        return -EFAULT;

    *f_pos += count;

    if ( *f_pos >= sizeof( protocol_sequence ) ) {

        __protocol_sequence = protocol_sequence;

        dgfsm_init( &__protocol_sequence ); // it will be loaded at next irq

        if ( __dgfsm.state == 0 ) { // fsm 'stopped'
            dgfsm_handle_irq( &__protocol_sequence, __debug_level__ > 5 ); // force commit
            dgfsm_start();
        }

    }

    return count;
}

static int dgmod_gpio_output_init( int gpio, const char * name )
{
    if ( gpio_is_valid( gpio ) ) {

        __gpio_allocated.gpioNumber[ __gpio_allocated.size++ ] = gpio;

        gpio_request( gpio, name );
        gpio_direction_output( gpio, 1 );
        gpio_export( gpio, true );

        return 1;
    }
    return 0;
}

static int dgmod_gpio_input_init( int gpio, const char * name, int irqType )
{
    int irqNumber = 0;

    if ( gpio_is_valid( gpio ) ) {

        __gpio_allocated.gpioNumber[ __gpio_allocated.size++ ] = gpio;

        gpio_request( gpio, name );
        gpio_direction_input( gpio );

        gpio_export( gpio, false );     // export /sys/class, false prevents direction change

        if ( ( irqNumber = gpio_to_irq( gpio ) ) > 0 ) {
            printk( KERN_INFO "" MODNAME " GPIO [%d] (%s) mapped to IRQ: %d\n", gpio, name, irqNumber );

            // irq
            if ( request_irq( irqNumber, handle_interrupt, 0, "dgmod", NULL) < 0 ) {
                printk( KERN_ALERT "" MODNAME " ERROR: GPIO IRQ: %d request failed\n", irqNumber );
                return 0;
            }
        } else {
            printk( KERN_ALERT "" MODNAME " ERROR: GPIO [%d] (%s) map to IRQ: %d.\n", gpio, name, irqNumber );
            return 0;
        }

        irq_set_irq_type( irqNumber, irqType );  // see linux/irq.h

        return irqNumber;

    } else {
        printk(KERN_INFO "" MODNAME " gpio %d not valid\n", gpio );
    }
    return 0;
}


static struct file_operations dgmod_cdev_fops = {
    .owner   = THIS_MODULE,
    .open    = dgmod_cdev_open,
    .release = dgmod_cdev_release,
    .unlocked_ioctl = dgmod_cdev_ioctl,
    .read    = dgmod_cdev_read,
    .write   = dgmod_cdev_write,
};

static dev_t dgmod_dev_t = 0;
static struct cdev * __dgmod_cdev;
static struct class * __dgmod_class;

static int dgmod_dev_uevent( struct device * dev, struct kobj_uevent_env * env )
{
    add_uevent_var( env, "DEVMODE=%#o", 0644 );
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
    if ( alloc_chrdev_region(&dgmod_dev_t, 0, 1, "dgmod-cdev" ) < 0 ) {
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

    __dgmod_class = class_create( THIS_MODULE, "dgmod" );
    if ( !__dgmod_class ) {
        printk( KERN_ERR "" MODNAME " failed to create class\n" );
        return dgmod_dtor( -1 );
    }
    __dgmod_class->dev_uevent = dgmod_dev_uevent;

    // make_nod /dev/dgmod0
    if ( !device_create( __dgmod_class, NULL, dgmod_dev_t, NULL, "dgmod%d", MINOR( dgmod_dev_t ) ) ) {
        printk( KERN_ERR "" MODNAME " failed to create device\n" );
        return dgmod_dtor( -1 );
    }

    // /proc create
    proc_create( "dgmod", 0666, NULL, &proc_file_fops );

    if ( populate_device_tree() )
        return -ENODEV;

    platform_driver_register( &__slave_platform_driver );

    gpio_set_value( __gpio_user_led, 1 );

    // dg_data_[0,1,2]
    for ( int i = 0; i < 3; ++i ) {
        struct slave_device * p = &__slave_device[ i ];
        u32 base = __bridge.reg[ p->reg[ 0 ] * 2 ]; // reg[] is a pair of 2, so start addr for n'th item is [n * 2]
        u32 offs = p->reg[ 1 ];
        u32 size = p->reg[ 2 ];

        if ( (p->addr = (uint32_t *)( dgmod_request_mem_region( base + offs, size, "dgmod-dg_data" ) ) ) == 0 ) {
            return dgmod_dtor( -1 );
        }
    }

    // slave device interrupt cannot be cleard. so mask (disable) all
    if ( __data_1( 0 ) ) {
        for ( int i = 0; i < 16; ++i ) {
            __data_1( i )->edge_capture = 1;
            __data_1( i )->irqmask = 0;
        }
    }

    dgfsm_init( &__protocol_sequence );

    __trigCounter = __data_1( dg1_trigCount )->user_dataout;

    sema_init( &__sem, 1 );

    init_waitqueue_head( &__trig_queue );

    __trig_data.cache_ = kmalloc( __trig_data.cache_size_ * sizeof( struct dgmod_trigger_data ), GFP_KERNEL );
    memset( __trig_data.cache_, 0, __trig_data.cache_size_ * sizeof( struct dgmod_trigger_data ) );

    for ( int i = 0; i < countof( __gpio__ ); ++i ) {
        struct gpio_register * p = &__gpio__[ i ];

        if ( strcmp( p->name, "pio_0" ) == 0 ) {
            // pio_0 <- T0 periodic interrupt
            if ( (p->addr = dgmod_request_mem_region( p->start, p->regs[ 2 ], p->gpio_name )) )
                irq_mask( p, 0x0f );

            if ( ( __irq.irqNumber[ __irq.size ] = dgmod_gpio_input_init( p->gpio
                                                                          , "dgmod-pio_0", IRQ_TYPE_EDGE_RISING ) ) ) {
                __irqNumber = __irq.irqNumber[ __irq.size ]; // set IRQ number
                ++__irq.size;
            }

        } else if ( strcmp( p->name, "button_pio" ) == 0 ) {

            if ( (p->addr = dgmod_request_mem_region( p->start, p->regs[ 2 ], p->gpio_name )) )
                irq_mask( p, 0x0f );

            for ( int j = 0; j < p->count && countof(__button_irq); ++j ) {
                if ( ( __irq.irqNumber[ __irq.size ] = dgmod_gpio_input_init( p->gpio + j
                                                                              , "dgmod-button", IRQ_TYPE_EDGE_FALLING ) ) ) {
                    __button_irq[ j ] = __irq.irqNumber[ __irq.size ];
                    ++__irq.size;
                }
            }
        } else if ( strcmp( p->name, "dipsw_pio" ) == 0 ) {

            if ( (p->addr = dgmod_request_mem_region( p->start, p->regs[ 2 ], p->gpio_name )) )
                irq_mask( p, 0x0f );

            for ( int j = 0; j < p->count && countof( __dipsw_irq ); ++j ) {
                if ( ( __irq.irqNumber[ __irq.size ]
                       = dgmod_gpio_input_init( p->gpio + j, "dgmod-dipsw", IRQ_TYPE_EDGE_BOTH ) ) ) {
                    __dipsw_irq[ j ] = __irq.irqNumber[ __irq.size ];
                    ++__irq.size;
                    irq_mask( p, 0x0f );
                }
            }
        } else if ( strcmp( p->name, "led_pio" ) == 0 ) {
            dgmod_gpio_output_init( p->gpio, "dgmod-led0" );
            gpio_set_value( p->gpio, 1 );
        }
    }

    ///

    dgmod_gpio_output_init( __gpio_user_led, "dgmod-led0" );

    dgfsm_commit( &__protocol_sequence.protocols_[ 0 ], 0 );
    //
    printk(KERN_ALERT "dgmod registered.  read cache size is %d\n", __trig_data.cache_size_ );

    return 0;
}

static void
dgmod_module_exit( void )
{
    dgfsm_stop();
    gpio_set_value( __gpio_user_led, 0 );

    platform_driver_unregister( &__slave_platform_driver );

    for ( int i = 0; i < __irq.size; ++i ) {
        free_irq( __irq.irqNumber[ i ], NULL );
        printk( KERN_INFO "free_irq( %d )\n", __irq.irqNumber[ i ] );
    }

    for ( int i = 0; i < __gpio_allocated.size; ++i ) {
        gpio_free( __gpio_allocated.gpioNumber[ i ] );
        printk( KERN_INFO "gpio_free( %d )\n", __gpio_allocated.gpioNumber[ i ] );
    }

    kfree( __trig_data.cache_ );

    for ( int i = 0; i < countof( __gpio__ ); ++i ) {
        if ( __gpio__[ i ].gpio > 0 )
            irq_mask( &__gpio__[i], 0 ); // disable interrupts
    }

    dgmod_release_regions();

    remove_proc_entry( "dgmod", NULL ); // proc_create

    device_destroy( __dgmod_class, dgmod_dev_t ); // device_creeate

    class_destroy( __dgmod_class ); // class_create

    cdev_del( __dgmod_cdev ); // cdev_alloc, cdev_init

    unregister_chrdev_region( dgmod_dev_t, 1 ); // alloc_chrdev_region
    //

    printk( KERN_INFO "" MODNAME " driver v%s unloaded\n", MOD_VERSION );
}

static int
dgmod_slave_module_probe( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {

        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be allocated.\n", pdev->name, irqNumber, pdev->resource->start );

        if ( request_irq( irqNumber, handle_slave_interrupt, 0, "dgmod", NULL) < 0 ) {
            printk( KERN_ALERT "" MODNAME " ERROR: %s IRQ %d, %x request failed.\n"
                    , pdev->name, irqNumber, pdev->resource->start );
        }

        dgfsm_start(); // start triggering
    }
    return 0;
}

static int
dgmod_slave_module_remove( struct platform_device * pdev )
{
    int irqNumber;
    if ( ( irqNumber = platform_get_irq( pdev, 0 ) ) > 0 ) {
        printk( KERN_INFO "" MODNAME " %s IRQ %d, %x about to be freed\n", pdev->name, irqNumber, pdev->resource->start );
        free_irq( irqNumber, 0 );
    }
    return 0;
}

static const struct of_device_id __slave_module_id [] = {
    { .compatible = "dg_data" },
    // { .compatible = "altera,avalon-mm-slave-2.0" },
    {}
};

static struct platform_driver __slave_platform_driver = {
    .driver = {
        . name = MODNAME
        , .owner = THIS_MODULE
        , .of_match_table = of_match_ptr( __slave_module_id )
        ,
    }
    , .probe = dgmod_slave_module_probe
    , .remove = dgmod_slave_module_remove
};

module_init( dgmod_module_init );
module_exit( dgmod_module_exit );
