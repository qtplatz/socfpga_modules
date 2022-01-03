/**************************************************************************
** Copyright (C) 2016-2019 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2019 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Reference: http://www.analog.com/media/en/technical-documentation/data-sheets/AD5593R.pdf
**           AD5593R: 8-Channel, 12-Bit, Configurable ADC/DAC with On-Chip Reference, I2C Interface Data Sheet
**           Rev.C (Last Content Update: 05/03/2017)
**************************************************************************/

#pragma once

#include <boost/optional.hpp>
#include <array>
#include <bitset>
#include <string>
#include <ostream>
#include <system_error>
#include <memory>

namespace i2c_linux { class i2c; }

namespace i2c_device {

    namespace ad5593 {

        enum AD5593R_IO_FUNCTION {
            ADC
            , DAC
            , GPIO
            , UNUSED_HIGH
            , UNUSED_LOW
            , UNUSED_TRISTATE
            , UNUSED_PULLDOWN
            , nFunctions
        };

        enum AD5593_GPIO_DIRECTION {
            GPIO_INPUT
            , GPIO_OUTPUT
        };

        constexpr size_t number_of_pins = 8;
        constexpr size_t number_of_regs = 7;

        class AD5593 {
            std::array< std::bitset< 8 >, number_of_regs > bitmaps_;
            std::unique_ptr< i2c_linux::i2c > i2c_;
            bool dirty_;
            double Vref_;
            std::bitset< 16 > pd_;
            std::bitset< 16 > ctrl_;
            std::bitset< 16 > adc_sequence_;

        public:
            ~AD5593();
            AD5593();
            AD5593( const std::string& device = "/dev/i2c-1", int address = 0x10 );
            bool open( const std::string& device = "/dev/i2c-1", int address = 0x10 );

            const std::error_code& error_code() const;
            operator bool () const;
            double Vref() const;

            const std::bitset< 16 >& pd() const { return pd_; }
            const std::bitset< 16 >& ctrl() const { return ctrl_; }

            inline bool is_dirty() const { return dirty_; }

        private:
            bool write( uint8_t addr, uint16_t value ) const;
            bool read( uint8_t addr, uint8_t * data, size_t size ) const;
            boost::optional< uint16_t > read( uint8_t addr ) const;

            void save();
            bool restore();

        public:
            bool dac_set_value( int pin, uint16_t value );

            bool reset_function( int pin, AD5593R_IO_FUNCTION );
            bool set_function( int pin, AD5593R_IO_FUNCTION );
            bool is_function( int pin, AD5593R_IO_FUNCTION ) const;

            bool fetch( bool force = true );
            bool commit();

            bool is_adc_en( int pin ) const;
            bool is_dac_en( int pin ) const;
            uint16_t dac_readback( int pin );
            uint16_t adc_readback( int pin );

            void print_config( std::ostream& ) const;
            void print_values( std::ostream& );
            bool set_adc_sequence( uint16_t sequence );
            std::bitset< 16 > adc_sequence( bool fetch = true ) const;
            const std::bitset< 8 >& adc_enabled() const;
            std::pair< std::string, std::bitset< 8 > > reg_enabled( size_t idx ) const;

            bool read_adc_sequence ( uint16_t * data, size_t size );
            void print_adc_sequence( std::ostream&, const uint16_t * sequence, size_t size, bool has_temp ) const;
            double toVolts( uint32_t, bool is_dac ) const;
            uint16_t fromVolts( double, bool is_dac ) const;
            double temperature( uint32_t ) const;
            bool set_vref( bool );
            bool set_ctrl( uint16_t );
            bool set_pd( uint16_t );
        };
    }
}
