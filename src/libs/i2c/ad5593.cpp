/**************************************************************************
** Copyright (C) 2016-2019 MS-Cheminformatics LLC, Toin, Mie Japan
** Author: Toshinobu Hondo, Ph.D.
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

#include "ad5593.hpp"
#include "i2c.hpp"
#include <iostream>
#include <boost/format.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

namespace i2c_device {

    namespace ad5593 {
        enum AD5593R_REG {
            AD5593R_REG_NOOP           = 0x0
            , AD5593R_REG_DAC_READBACK = 0x1
            , AD5593R_REG_ADC_SEQ      = 0x2
            , AD5593R_REG_CTRL         = 0x3 // General Purpose control register
            , AD5593R_REG_ADC_EN       = 0x4
            , AD5593R_REG_DAC_EN       = 0x5
            , AD5593R_REG_PULLDOWN     = 0x6
            , AD5593R_REG_LDAC         = 0x7
            , AD5593R_REG_GPIO_OUT_EN  = 0x8
            , AD5593R_REG_GPIO_SET     = 0x9
            , AD5593R_REG_GPIO_IN_EN   = 0xa
            , AD5593R_REG_PD           = 0xb
            , AD5593R_REG_OPEN_DRAIN   = 0xc
            , AD5593R_REG_TRISTATE     = 0xd
            , AD5593R_REG_RESET        = 0xf
        };

        enum AD5593R_MODE {
            AD5593R_MODE_CONF            = (0 << 4)
            , AD5593R_MODE_DAC_WRITE     = (1 << 4)
            , AD5593R_MODE_ADC_READBACK  = (4 << 4)
            , AD5593R_MODE_DAC_READBACK  = (5 << 4)
            , AD5593R_MODE_GPIO_READBACK = (6 << 4)
            , AD5593R_MODE_REG_READBACK  = (7 << 4)
        };

        constexpr static AD5593R_REG __reg_list [] = {
            AD5593R_REG_NOOP
            , AD5593R_REG_DAC_READBACK
            , AD5593R_REG_ADC_SEQ
            , AD5593R_REG_CTRL
            , AD5593R_REG_ADC_EN
            , AD5593R_REG_DAC_EN
            , AD5593R_REG_PULLDOWN
            , AD5593R_REG_LDAC
            , AD5593R_REG_GPIO_OUT_EN
            , AD5593R_REG_GPIO_SET
            , AD5593R_REG_GPIO_IN_EN
            , AD5593R_REG_PD
            , AD5593R_REG_OPEN_DRAIN
            , AD5593R_REG_TRISTATE
            , AD5593R_REG_RESET
        };

        constexpr static AD5593R_REG __fetch_reg_list [] = {
            AD5593R_REG_ADC_EN        // bitmaps_[0]
            , AD5593R_REG_DAC_EN      // bitmaps_[1]
            , AD5593R_REG_PULLDOWN    // bitmaps_[2]
            , AD5593R_REG_GPIO_OUT_EN // bitmaps_[3]
            , AD5593R_REG_GPIO_SET    // bitmaps_[4]
            , AD5593R_REG_GPIO_IN_EN  // bitmaps_[5]
            , AD5593R_REG_TRISTATE    // bitmaps_[6]
        };

        template< AD5593R_REG > struct index_of {
            constexpr static size_t value = 0;
        };

        template<> struct index_of<AD5593R_REG_ADC_EN>      { constexpr static size_t value = 0; };
        template<> struct index_of<AD5593R_REG_DAC_EN>      { constexpr static size_t value = 1; };
        template<> struct index_of<AD5593R_REG_PULLDOWN>    { constexpr static size_t value = 2; };
        template<> struct index_of<AD5593R_REG_GPIO_OUT_EN> { constexpr static size_t value = 3; };
        template<> struct index_of<AD5593R_REG_GPIO_SET>    { constexpr static size_t value = 4; };
        template<> struct index_of<AD5593R_REG_GPIO_IN_EN>  { constexpr static size_t value = 5; };
        template<> struct index_of<AD5593R_REG_TRISTATE>    { constexpr static size_t value = 6; };

        static_assert(AD5593R_REG_ADC_EN      == __fetch_reg_list[ index_of< AD5593R_REG_ADC_EN >::value ]);
        static_assert(AD5593R_REG_DAC_EN      == __fetch_reg_list[ index_of< AD5593R_REG_DAC_EN >::value ]);
        static_assert(AD5593R_REG_PULLDOWN    == __fetch_reg_list[ index_of< AD5593R_REG_PULLDOWN >::value ]);
        static_assert(AD5593R_REG_GPIO_OUT_EN == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_OUT_EN >::value ]);
        static_assert(AD5593R_REG_GPIO_SET    == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_SET >::value ]);
        static_assert(AD5593R_REG_GPIO_IN_EN  == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_IN_EN >::value ]);
        static_assert(AD5593R_REG_TRISTATE    == __fetch_reg_list[ index_of< AD5593R_REG_TRISTATE >::value ]);

        constexpr static const char * __fetch_reg_name [] = {
            "AD5593R_REG_ADC_EN"        // bitmaps_[0]
            , "AD5593R_REG_DAC_EN"      // bitmaps_[1]
            , "AD5593R_REG_PULLDOWN"    // bitmaps_[2]
            , "AD5593R_REG_GPIO_OUT_EN" // bitmaps_[3]
            , "AD5593R_REG_GPIO_SET"    // bitmaps_[4]
            , "AD5593R_REG_GPIO_IN_EN"  // bitmaps_[5]
            , "AD5593R_REG_TRISTATE"    // bitmaps_[6]
        };

        template< AD5593R_REG ... regs > struct set_io_function;

        template< AD5593R_REG reg > struct set_io_function< reg >  { // last
            void operator()( std::array< std::bitset< 8 >, number_of_regs >& masks, int pin ) const {
                masks[ index_of< reg >::value ].set( pin );
            }
        };

        template< AD5593R_REG first, AD5593R_REG ... regs > struct set_io_function< first, regs ...> {
            void operator()( std::array< std::bitset< 8 >, number_of_regs >& masks, int pin ) const {
                masks[ index_of< first >::value ].set( pin );
            }
        };

        template< AD5593R_REG ... regs > struct reset_io_function;

        template< AD5593R_REG last > struct reset_io_function< last >  { // last
            void operator()( std::array< std::bitset< 8 >, number_of_regs >& masks, int pin ) const {
                masks[ index_of< last >::value ].reset( pin );
                // std::cout << "reset<last>(" << pin << ", " << __fetch_reg_name[ index_of< last >::value ] << ")";
                // std::cout << "\t=" << masks[ index_of< last >::value ].to_ulong() << std::endl;
            }
        };

        template< AD5593R_REG first, AD5593R_REG ... regs > struct reset_io_function< first, regs ...> {
            void operator()( std::array< std::bitset< 8 >, number_of_regs >& masks, int pin ) const {
                masks[ index_of< first >::value ].reset( pin );
                reset_io_function< regs ... >()( masks, pin );
                // std::cout << "reset(" << pin << ", " << __fetch_reg_name[ index_of< first >::value ] << ")";
                // std::cout << "\t=" << masks[ index_of< first >::value ].to_ulong() << std::endl;
            }
        };

        struct io_function {
            AD5593R_IO_FUNCTION operator()( const std::array< std::bitset< 8 >, number_of_regs >& masks, int pin ) const {
                return AD5593R_IO_FUNCTION(0);
            }
        };
    }
}

using namespace i2c_device::ad5593;

AD5593::~AD5593()
{
}

AD5593::AD5593( const std::string& device
                , int address ) : i2c_( std::make_unique< i2c_linux::i2c >() )
                                , dirty_( true )
                                , Vref_( 3.3 )
{
    i2c_->init( device.c_str(), address );
    fetch( true );
}

bool
AD5593::open( const std::string& device, int address )
{
    if ( i2c_->init( device.c_str(), address ) )
        return fetch( true );
    return false;
}

AD5593::operator bool () const
{
    return i2c_ && *i2c_;
}

const std::error_code&
AD5593::error_code() const
{
    return i2c_->error_code();
}

bool
AD5593::write( uint8_t addr, uint16_t value ) const
{
    std::array< uint8_t, 3 > buf = {{ addr, uint8_t(value >> 8), uint8_t(value & 0xff) }};
    if ( i2c_ && i2c_->write( buf.data(), buf.size() ) )
        return true;
    return false;
}

boost::optional< uint16_t >
AD5593::read( uint8_t addr ) const
{
    if ( i2c_ && i2c_->write( &addr, 1 ) ) {
        std::array< uint8_t, 2 > buf = {{ 0 }};
        if ( i2c_->read( buf.data(), buf.size() ) )
            return uint16_t( buf[ 0 ] ) << 8 | buf[ 1 ];
    }
    return boost::none;
}

bool
AD5593::read( uint8_t addr, uint8_t * data, size_t size ) const
{
    if ( i2c_ && i2c_->write( &addr, 1 ) )
        return i2c_->read( data, size );
    return false;
}

bool
AD5593::reset_function( int pin, AD5593R_IO_FUNCTION f )
{
    dirty_ = true;
    switch( f ) {
    case ADC:
        reset_io_function< AD5593R_REG_ADC_EN >()( bitmaps_, pin );
        break;
    case DAC:
        reset_io_function< AD5593R_REG_DAC_EN >()( bitmaps_, pin );
        break;
    default:
        break;
    };
    return true;
}

bool
AD5593::set_function( int pin, AD5593R_IO_FUNCTION f )
{
    dirty_ = true;
    switch( f ) {
    case ADC:
        set_io_function< AD5593R_REG_ADC_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_GPIO_OUT_EN
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case DAC:
        set_io_function< AD5593R_REG_DAC_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_GPIO_OUT_EN
                           ,  AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case GPIO:
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN >()( bitmaps_, pin );
        break;
    case UNUSED_LOW:
        set_io_function< AD5593R_REG_GPIO_OUT_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case UNUSED_HIGH:
        set_io_function< AD5593R_REG_GPIO_SET, AD5593R_REG_GPIO_OUT_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case UNUSED_TRISTATE:
        set_io_function< AD5593R_REG_TRISTATE >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_OUT_EN
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case UNUSED_PULLDOWN:
        set_io_function< AD5593R_REG_PULLDOWN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_OUT_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_TRISTATE
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    default:
        break;
    };
    return false;
}

bool
AD5593::is_function( int pin, AD5593R_IO_FUNCTION f ) const
{
    switch( f ) {
    case ADC: return is_adc_en( pin );
    case DAC: return is_dac_en( pin );
    case GPIO:
        return ( !is_adc_en(pin) && !is_dac_en( pin ) && !bitmaps_[ index_of< AD5593R_REG_PULLDOWN >::value ].test(pin) );
    case UNUSED_LOW:
        return bitmaps_[ index_of< AD5593R_REG_GPIO_OUT_EN >::value ].test(pin);
    case UNUSED_HIGH:
        return bitmaps_[ index_of< AD5593R_REG_GPIO_IN_EN >::value ].test(pin);
    case UNUSED_TRISTATE:
        return bitmaps_[ index_of< AD5593R_REG_TRISTATE >::value ].test(pin);
    case UNUSED_PULLDOWN:
        return bitmaps_[ index_of< AD5593R_REG_PULLDOWN >::value ].test(pin);
    default:
        break;
    };
    return false;
}

bool
AD5593::fetch( bool force )
{
    size_t i = 0;

    if ( !force && !dirty_ && !i2c_->error_code() )
        return true;

    if ( i2c_->error_code() ) {
        i2c_->error_clear();

        // restore parameter
        if ( !commit() )
            return false;

        if ( !set_ctrl( ctrl_.to_ulong() ) )
            return false;

        if ( !write( AD5593R_REG_PD, pd_.to_ulong() & 0xffff ) )
            return false;

        for ( int pin = 0; pin < 8; ++pin )  // force 0 dac value for all enabled dac channels
            dac_set_value( pin, 0 );

        if ( !write( AD5593R_REG_ADC_SEQ, adc_sequence_.to_ulong() ) )
            return false;
    }

    dirty_ = true;

    for ( auto reg: __fetch_reg_list ) {
        if ( auto value = read( reg | AD5593R_MODE_REG_READBACK ) ) {
            bitmaps_[ i++ ] = uint8_t( value.get() );
        } else {
            dirty_ = true;
            return false;
        }
    }

    if ( auto pd = read( AD5593R_REG_PD | AD5593R_MODE_REG_READBACK ) ) {
        pd_ = std::bitset< 16 >( pd.get() );
    } else {
        dirty_ = true;
        return false;
    }

    if ( auto ctrl = read( AD5593R_REG_CTRL | AD5593R_MODE_REG_READBACK ) ) {
        ctrl_ = std::bitset< 16 >( ctrl.get() );
    } else {
        dirty_ = true;
        return false;
    }

    if ( auto sequence = read( AD5593R_MODE_REG_READBACK | AD5593R_REG_ADC_SEQ ) ) {
        write( AD5593R_REG_ADC_SEQ, sequence.get() | 0x0300 );
        adc_sequence_ = std::bitset< 16 >( sequence.get() );
    } else {
        dirty_ = true;
        return false;
    }

    Vref_ = pd_.test( 9 ) ? 2.5 : 3.3;
    dirty_ = false;
    return true;
}

void
AD5593::print_config( std::ostream& o ) const
{
    size_t i = 0;

    for ( auto& bitmap: bitmaps_ )
        o << boost::format( "%s\t%s\t0x%x" ) % __fetch_reg_name[ i++ ] % bitmap.to_string() % bitmap.to_ulong() << std::endl;

    o << "AD5593R_REG_PD        \t" << pd_.to_string() << "\tInt. Reference: " << ( pd_.test( 9 ) ? "enabled" : "disabled" ) << std::endl;
    o << "AD5593R_REG_CTRL      \t" << ctrl_.to_string()
              << boost::format("\tADC Gain: %d, DAC Gain: %d")  % ( ctrl_.test(5) ? 2 : 1 ) % ( ctrl_.test(4) ? 2 : 1 ) << std::endl;
    o << std::endl;
}

void
AD5593::print_values( std::ostream& o )
{
    for ( int pin = 0; pin < 8; ++pin ) {

        if ( is_adc_en( pin ) ) {
            const uint16_t a = adc_readback( pin );
            o << "adc pin# " << pin << " " << boost::format("0x%03x\t%.2lfV\t") % a % (double(a) * Vref_ / 4096);
        }
        if ( is_dac_en( pin ) ) {
            const uint16_t a = dac_readback( pin );
            o << "dac pin# " << pin << " " << boost::format("0x%03x\t%.2lfV") % a % (double(a) * Vref_ / 4096);
        }
        o << std::endl;
    }
}

bool
AD5593::commit()
{
    size_t i = 0;
    for ( auto reg: __fetch_reg_list ) {
        if ( !write( reg, static_cast< uint16_t >( bitmaps_[ i++ ].to_ulong() ) ) ) {
            dirty_ = true;
            return false;
        }
    }
    dirty_ = false;
    return true;
}

bool
AD5593::dac_set_value( int pin, uint16_t value )
{
    if ( is_dac_en( pin ) )
        return write( AD5593R_MODE_DAC_WRITE | pin, value );
    return true;
}

bool
AD5593::is_adc_en( int pin ) const
{
    return bitmaps_[ index_of< AD5593R_REG_ADC_EN >::value ].test( pin );
}

bool
AD5593::is_dac_en( int pin ) const
{
    return bitmaps_[ index_of< AD5593R_REG_DAC_EN >::value ].test( pin );
}

uint16_t
AD5593::dac_readback( int pin )
{
    if ( fetch( false ) && is_dac_en( pin ) ) {
        if ( auto value = read( AD5593R_MODE_DAC_READBACK | pin ) )
            return value.get() & 0x0fff;
    }
    return -1;
}

uint16_t
AD5593::adc_readback( int pin )
{
    if ( fetch( false ) && is_adc_en( pin ) ) {
        if ( auto value = read( AD5593R_MODE_ADC_READBACK | pin ) )
            return value.get() & 0x0fff;
    }
    return -1;
}

bool
AD5593::set_adc_sequence( uint16_t sequence )
{
    adc_sequence_ = std::bitset< 16 >( sequence );
    return fetch( false ) && write( AD5593R_REG_ADC_SEQ, sequence );
}

std::bitset< 16 >
AD5593::adc_sequence( bool fetch ) const
{
    if ( fetch ) {
        if ( auto sequence = read( AD5593R_MODE_REG_READBACK | AD5593R_REG_ADC_SEQ ) )
            return std::bitset< 16 >( sequence.get() );
        return std::bitset< 16 >( 0 );
    } else {
        return adc_sequence_;
    }
}

bool
AD5593::read_adc_sequence ( uint16_t * data, size_t size )
{
    if ( fetch( false ) && read( AD5593R_MODE_ADC_READBACK, reinterpret_cast< uint8_t *>( data ), size * sizeof( uint16_t ) ) ) {
        std::transform( data, data + size, data
                        , []( const uint16_t& b ) {
                              auto p = reinterpret_cast< const uint8_t * >(&b);
                              return uint16_t( p[0] ) << 8 | p[1];
                          } );
        return true;
    }
    return false;
}

void
AD5593::print_adc_sequence( std::ostream& o, const uint16_t * sequence, size_t size, bool has_temp ) const
{
    //size_t pin(0);
    if ( has_temp && size > 1 ) {
        std::for_each( sequence, sequence + (size - 1)
                       , [&]( const uint16_t& a ) {
                             o << boost::format("[%d] %.3lf, ") % (a >> 12) % toVolts( a, false );
                         });
    }

    if ( has_temp && size > 0 ) {
        auto a = sequence[ size - 1 ];
        o << boost::format( "[T] %.2lf (%d)" ) % temperature( a ) % a;
    }

    o << std::endl;
}

double
AD5593::toVolts( uint32_t a, bool is_dac ) const
{
    int gain = ctrl_.test( is_dac ? 4 : 5 ) ? 2 : 1;
    return double( a & 0x0fff ) * (Vref_ * gain) / 4096;
}

uint16_t
AD5593::fromVolts( double volts, bool is_dac ) const
{
    int gain = ctrl_.test( is_dac ? 4 : 5 ) ? 2 : 1;
    return uint16_t( 0.5 + volts * 4095 / (Vref_ * gain));
}


double
AD5593::temperature( uint32_t a ) const
{
    if ( ctrl_.test( 5 ) ) {
        return ( int( a & 0x0fff ) - ( 0.5/(2*Vref_) ) * 4095 ) / ( 1.327 * (2.5/Vref_) ) + 25;
    } else {
        return ( int( a & 0x0fff ) - (0.5/Vref_) * 4095 ) / ( 2.654 * (2.5/Vref_) ) + 25;
    }
}

bool
AD5593::set_vref( bool enable )
{
    auto pd( pd_ );
    pd.set( 9, enable );

    dirty_ = true;
    return write( AD5593R_REG_PD, pd.to_ulong() & 0xffff );
}

bool
AD5593::set_ctrl( uint16_t ctrl )
{
    return write( AD5593R_REG_CTRL, ctrl );
}

bool
AD5593::set_pd( uint16_t pd )
{
    return write( AD5593R_REG_PD, pd );
}

const std::bitset< 8 >&
AD5593::adc_enabled() const
{
    return bitmaps_[ index_of<AD5593R_REG_ADC_EN>::value ];
}

std::pair< std::string, std::bitset< 8 > >
AD5593::reg_enabled( size_t i ) const
{
    switch( i ) {
    case index_of<AD5593R_REG_ADC_EN>::value:      return std::make_pair( "adc_en",     bitmaps_[index_of<AD5593R_REG_ADC_EN>::value ] );
    case index_of<AD5593R_REG_DAC_EN>::value:      return std::make_pair( "dac_en",     bitmaps_[index_of<AD5593R_REG_DAC_EN>::value ] );
    case index_of<AD5593R_REG_PULLDOWN>::value:    return std::make_pair( "pulldown",   bitmaps_[index_of<AD5593R_REG_PULLDOWN>::value ] );
    case index_of<AD5593R_REG_GPIO_OUT_EN>::value: return std::make_pair( "gpio_out_en",bitmaps_[index_of<AD5593R_REG_GPIO_OUT_EN>::value] );
    case index_of<AD5593R_REG_GPIO_SET>::value:    return std::make_pair( "gpio_set",   bitmaps_[index_of<AD5593R_REG_GPIO_SET>::value] );
    case index_of<AD5593R_REG_GPIO_IN_EN>::value:  return std::make_pair( "gpio_in_en", bitmaps_[index_of<AD5593R_REG_GPIO_IN_EN>::value] );
    case index_of<AD5593R_REG_TRISTATE>::value:    return std::make_pair( "tristate",   bitmaps_[index_of<AD5593R_REG_TRISTATE>::value] );
    };
    return std::make_pair( "", 0 );
}
