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

#include "bmp280.hpp"
#include "i2c.hpp"
#include <atomic>
#include <sstream>
#if defined __linux
#include <boost/format.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#else
//#include "stream.hpp"
//#include "debug_print.hpp"
#endif

namespace i2c_device {
    namespace bmp280 {

        struct trimming_parameter {
            template< typename T > void operator()( T& d, const uint8_t *& p ) const {
                d = T( uint16_t( p[0] ) | uint16_t( p[1] ) << 8 );
                p += sizeof(T);
            }
        };

        //	Table 5, section 3.3.2 p12
        //	temp oversampling 0xf4 [7:5]
        //  press oversampling 0xf4 [4:2]
        enum BMP280_OVERSAMP_SKIPPED {
            BMP280_OVERSAMP_SKIPPED = 0
            , BMP280_OVERSAMP_1X = 1
            , BMP280_OVERSAMP_2X = 2
            , BMP280_OVERSAMP_4X = 3
            , BMP280_OVERSAMP_8X = 4
            , BMP280_OVERSAMP_16X = 5
        };

        // 0xf4, bit [1:0]
        enum BMP280_POWER_MODE {
            BMP280_SLEEP_MODE    = 0
            , BMP280_FORCED_MODE = 1
            , BMP280_NORMAL_MODE = 3
        };

        enum BMP280_STANDBYTIME {
            BMP280_STANDBYTIME_1_MS       =    0x00
            , BMP280_STANDBYTIME_63_MS    =    0x01
            , BMP280_STANDBYTIME_125_MS   =    0x02
            , BMP280_STANDBYTIME_250_MS   =    0x03
            , BMP280_STANDBYTIME_500_MS   =    0x04
            , BMP280_STANDBYTIME_1000_MS  =    0x05
            , BMP280_STANDBYTIME_2000_MS  =    0x06
            , BMP280_STANDBYTIME_4000_MS  =    0x07
        };

        // 0xf5 [4:2]
        // 3.4, p14 Table 7
        enum BMP280_FILTER_COEFF {
            BMP280_FILTER_COEFF_OFF  = 0
            , BMP280_FILTER_COEFF_2  = 1
            , BMP280_FILTER_COEFF_4  = 2
            , BMP280_FILTER_COEFF_8  = 3
            , BMP280_FILTER_COEFF_16 = 4
        };

    };
}

using namespace i2c_device::bmp280;

BMP280::~BMP280()
{
}

BMP280::BMP280() : address_( 0 )
                 , has_trimming_parameter_( false )
                 , dig_T1(0), dig_T2(0), dig_T3(0)
                 , dig_P1(0), dig_P2(0), dig_P3(0), dig_P4(0)
                 , dig_P5(0), dig_P6(0), dig_P7(0), dig_P8(0)
                 , dig_P9(0)
{
}

BMP280::BMP280( const std::string& device, int address ) : address_( address )
                                                         , has_trimming_parameter_( false )
                                                         , dig_T1(0), dig_T2(0), dig_T3(0)
                                                         , dig_P1(0), dig_P2(0), dig_P3(0), dig_P4(0)
                                                         , dig_P5(0), dig_P6(0), dig_P7(0), dig_P8(0)
                                                         , dig_P9(0)
{
    open( device, address );
}

bool
BMP280::open( const std::string& devname, int address )
{
    std::cerr << "BMP280::open(" << devname << ", " << address << ")\n";
    device_ = devname;
    address_ = address;
    if ( i2c_ = std::make_unique< i2c_linux::i2c >() ) {
        if ( i2c_->init( devname.c_str(), address ) )
            return trimming_parameter_readout() && measure();
        std::cerr << boost::format( "bmp280::init %s, 0x%x -- failed: " )
            % devname % address << i2c_->error_code().message()
                  << std::endl;
        return false;
    }
    std::cerr << "BMP280::open(" << devname << ", " << address << ") return false (end-of-function)\n";
    return false;
}

BMP280::operator bool () const
{
    return i2c_ && *i2c_;
}

bool
BMP280::trimming_parameter_readout()
{
    std::array< uint8_t, 24 > data = { 0 };

    if ( read( 0x88, data.data(), data.size() ) ) {
        const uint8_t * p = data.data();
        trimming_parameter()( dig_T1, p );
        trimming_parameter()( dig_T2, p );
        trimming_parameter()( dig_T3, p );
        trimming_parameter()( dig_P1, p );
        trimming_parameter()( dig_P2, p );
        trimming_parameter()( dig_P3, p );
        trimming_parameter()( dig_P4, p );
        trimming_parameter()( dig_P5, p );
        trimming_parameter()( dig_P6, p );
        trimming_parameter()( dig_P7, p );
        trimming_parameter()( dig_P8, p );
        trimming_parameter()( dig_P9, p );

        std::ostringstream o;
        for ( auto& d: data )
            o << boost::format("%2x,") % int(d);
        std::cerr << __FUNCTION__ << " : " << o.str() << std::endl;

        has_trimming_parameter_ = true;
        return true;
    }

    has_trimming_parameter_ = false;
    return has_trimming_parameter_;
}

/*
 * ------------------|------------------
 *	0x00             | BMP280_SLEEP_MODE
 *	0x01 and 0x02    | BMP280_FORCED_MODE
 *	0x03             | BMP280_NORMAL_MODE
 */

bool
BMP280::measure()
{
    // 0xf4 ctrl_meas
    // oversampling temp[7:5], press[4:2], power mode[1:0]
    constexpr uint8_t ctrl_meas = BMP280_OVERSAMP_8X << 5 | BMP280_OVERSAMP_8X << 2 | BMP280_NORMAL_MODE;

    // 0xf5 config (section 4.3.5, p26)
    // t_sb[7:5] (standby time), filter[4:2], spi3w_en[0]
    constexpr uint8_t config = BMP280_STANDBYTIME_500_MS << 5 | BMP280_FILTER_COEFF_16 << 2;

    if ( write( std::array< uint8_t, 4 >( { 0xf4, ctrl_meas, 0xf5, config } ) ) )
        return true;

    has_trimming_parameter_ = false;
    return true;
}

bool
BMP280::write( const uint8_t * data, size_t size ) const
{
    return i2c_ && i2c_->write( data, size );
}

bool
BMP280::read(  uint8_t addr, uint8_t * data, size_t size ) const
{
    return i2c_ && ( i2c_->write( &addr, 1 ) && i2c_->read( data, size ) );
}

std::pair< uint32_t, uint32_t >
BMP280::readout()
{
    if ( has_trimming_parameter_ ) {
        std::array< uint8_t, 6 > data;
        if ( read( 0xf7, data.data(), data.size() ) ) {
            uint32_t adc_P = uint32_t( data[0] ) << 12 | uint32_t( data[1] ) << 4 | data[2] & 0x0f;
            uint32_t adc_T  = uint32_t( data[3] ) << 12 | uint32_t( data[4] ) << 4 | data[5] & 0x0f;
            int32_t t_fine = 0;
            auto temp = compensate_T( adc_T, t_fine );
            auto press = compensate_P32( adc_P, t_fine );

            return { press, temp };
        }
    }

    trimming_parameter_readout() && measure();

    return { -1, -1 };
}

/*!
 *	@brief Reads actual temperature
 *	from uncompensated temperature
 *	@note Returns the value in 0.01 degree Centigrade
 *	@note Output value of "5123" equals 51.23 DegC.
 *
 *  @param adc_T : value of uncompensated temperature
 *
 *  @return Actual temperature output as int32_t
 *
 */
int32_t
BMP280::compensate_T( int32_t adc_T, int32_t& t_fine ) const
{
    int32_t var1 = ((((adc_T>>3) - (int32_t(dig_T1)<<1))) * (int32_t(dig_T2))) >> 11;
    int32_t var2 = (((((adc_T>>4) - (int32_t(dig_T1))) * ((adc_T>>4) - (int32_t(dig_T1)))) >> 12) * (int32_t(dig_T3))) >> 14;

    t_fine = var1 + var2;
    int32_t T = (t_fine * 5 + 128) >> 8;

    return T;
}

/*!
 *	@brief Reads actual pressure from uncompensated pressure
 *	and returns the value in Pascal(Pa)
 *	@note Output value of "96386" equals 96386 Pa =
 *	963.86 hPa = 963.86 millibar
 *
 *  @param  adc_P: value of uncompensated pressure
 *
 *  @return Returns the Actual pressure out put as s32
 */
uint32_t
BMP280::compensate_P32( uint32_t adc_P, int32_t t_fine ) const
{
	int32_t var1(0);
	int32_t var2(0);
	uint32_t P(0);

	var1 = (( int32_t(t_fine) ) >> 1 ) - 64000;

	var2 = (((var1 >> 02)  * (var1 >> 02)) >> 11) * (int32_t(dig_P6));
	var2 = var2 + ((var1 * int32_t(dig_P5) ) << 01 );
	var2 = (var2 >> 02) + (( int32_t(dig_P4)) << 16);

	var1 = (((dig_P3 * (((var1 >> 02) * (var1 >> 02)) >> 13))  >> 03) + (( dig_P2 * var1 ) >> 01 ))  >> 18;
	var1 = ((((32768 + var1)) * ( int32_t(dig_P1))) >> 15);

	// calculate pressure
	P = (( uint32_t((1048576) - adc_P) - (var2 >> 12))) * 3125;

	// check overflow
	if ( P < 0x80000000) {
		if ( var1 != 0 )
			P = (P << 01) / ( uint32_t(var1));
		else
			return 0; // invalid
    } else {
        if ( var1 != 0 )
            P = (P / uint32_t(var1)) * 2;
        else
            return 0; // invalid
    }

	var1 = (( int32_t(dig_P9)) * (int32_t(  ((P >> 03) * (P >> 03)) >> 13))) >> 12;

	var2 = ( ( int32_t(P >> 02)) * ( int32_t(dig_P8) ) )  >> 13;

	// calculate true pressure
	P = int32_t(P) + ( (var1 + var2 + dig_P7) >> 04 );

	return P;
}

#if 0
uint32_t
BMP280::compensate_P64( uint32_t adc_P, int32_t t_fine ) const
{
    int64_t var1 = int64_t(t_fine) - 128000;
    int64_t var2 = var1 * var1 * int64_t(dig_P6);
    var2 = var2 + ((var1 * int64_t(dig_P5))<<17);
    var2 = var2 + (int64_t(dig_P4) << 35);
    var1 = ((var1 * var1 * int64_t(dig_P3)) >> 8 ) + ((var1 * int64_t(dig_P2)) << 12);
    var1 = (( ( 1ll << 47 ) + var1 )) * int64_t(dig_P1) >> 33;
    if (var1 == 0)
        return 0; // avoid exception caused by division by zero
    int64_t p = 1048576ll - adc_P;

    p = (( ( p << 31 ) - var2 ) * 3125) / int32_t( var1 );  // <-- 64bit div
    //p = div64_s64( (( ( p << 31 ) - var2 ) * 3125), var1);

    var1 = (int64_t(dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (int64_t(dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (int64_t(dig_P7) << 4);
    return p;
}

#endif
