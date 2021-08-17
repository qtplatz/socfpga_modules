/**************************************************************************
** Copyright (C) 2016-2018 Toshinobu Hondo, Ph.D.
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

#include <date/date.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <condition_variable>
#include <bitset>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <stdint.h>
#include <thread>
#include <unistd.h>
#include <linux/types.h>

#define countof(a) (sizeof(a) / sizeof((a)[0]))

using namespace std::chrono_literals;

static const char *__adc_device = "/dev/adc-fifo0";

int
main( int argc, char * argv [] )
{
    namespace po = boost::program_options;

    po::variables_map vm;
    po::options_description description( "spi" );
    {
        description.add_options()
            ( "help,h",    "Display this help message" )
            ( "device,d",  po::value<std::string>()->default_value( __adc_device), "dma device name '/dev/adc-fifo0'" )
            ( "count,c",   po::value< std::string >(), "count" )
            ( "skip,s",    po::value< std::string >(), "skip n-triggers" )
            ( "wait",      po::value< uint32_t >()->default_value( 0 ), "wait (ms) between next read" )
            ( "offset,o",  po::value< double >()->default_value( 0.0 ), "offset(V)" )
            ( "raw,r",     "raw values" )
            ( "seek",      "seek to the end" );
            ;
        po::store( po::command_line_parser( argc, argv ).options( description ).run(), vm );
        po::notify(vm);
    }

    if ( vm.count( "help" ) ) {
        std::cerr << description;
        return 0;
    }

    const std::string adc_device = vm["device"].as< std::string >();
    size_t count = 1;
    if ( vm.count("count") )
        count = std::strtol( vm["count"].as< std::string >().c_str(), 0, 0 );

    size_t skip = 0;
    if ( vm.count( "skip" ) )
         skip = std::strtol( vm["skip"].as< std::string >().c_str(), 0, 0 );

    std::ifstream file;
    file.open( adc_device, std::ios::in | std::ios::binary );
    double offset = vm[ "offset" ].as< double >() * 1000;
    uint64_t t0 = 0;

    std::chrono::milliseconds wait( vm[ "wait" ].as< uint32_t >() );

    if ( file.is_open() ) {
        uint8_t data[ sizeof(uint32_t) * 16 ]; // 512bits -- 2019-Mar-18

        size_t skipCount = skip;
        size_t readCount = 0;
        bool callSeek = vm.count( "seek" );

        while ( count-- ) {

            if ( wait.count() )
                std::this_thread::sleep_for( wait );

            if ( callSeek )
                file.seekg( 0, std::ifstream::end );

            if ( file.read( reinterpret_cast< char * >(data), sizeof( data ) ) ) {
                uint32_t ad[8];
                uint8_t * bp = data;

                uint64_t adc_counter(0);
                uint64_t clock_counter(0), flag_clock_counter(0), posix_time(0);
                uint32_t flags(0), flags_d(0);

                for ( size_t i = 0; i < sizeof(uint64_t); ++i )
                    adc_counter |= static_cast< uint64_t >( *bp++ ) << ((sizeof(uint64_t) - 1 - i) * 8);

                for ( size_t i = 0; i < sizeof(ad)/sizeof(ad[0]); ++i ) {
                    ad[i] = *bp++ << 16;
                    ad[i] |= *bp++ << 8;
                    ad[i] |= *bp++;
                }
                // 256 +: 64
                for ( size_t i = 0; i < sizeof(uint64_t); ++i )
                    clock_counter |= static_cast< uint64_t >( *bp++ ) << ((sizeof(uint64_t) - 1 - i) * 8);
                // 320 +: 64
                for ( size_t i = 0; i < sizeof(uint64_t); ++i )
                    flag_clock_counter |= static_cast< uint64_t >( *bp++ ) << ((sizeof(uint64_t) - 1 - i) * 8);
                // 384 +: 32
                for ( size_t i = 0; i < sizeof(uint32_t); ++i )
                    flags |= static_cast< uint32_t >( *bp++ ) << ((sizeof(uint32_t) - 1 - i) * 8);
                for ( size_t i = 0; i < sizeof(uint32_t); ++i )
                    flags_d |= static_cast< uint32_t >( *bp++ ) << ((sizeof(uint32_t) - 1 - i) * 8);
                for ( size_t i = 0; i < sizeof(uint64_t); ++i )
                    posix_time |= static_cast< uint64_t >( *bp++ ) << ((sizeof(uint64_t) - 1 - i) * 8);

                std::bitset<32> bits( flags );

                uint32_t nacc = uint32_t( adc_counter >> 52 ) + 1;
                adc_counter &= ~0xfff0000000000000LL;

                if ( t0 == 0 )
                    t0 = clock_counter;
                uint32_t usec = uint32_t( ( clock_counter - t0 ) * 20 / 1000 );
                t0 = clock_counter;

                if ( skipCount )
                    skipCount--;

                if ( usec > ( 32 * nacc ) )
                    skipCount = 0; // force print

                if ( skipCount == 0 ) {

                    skipCount = skip;

                    std::chrono::nanoseconds dur( clock_counter * 20 ); // microseconds, duration since adc started (51.125days to overflow)
                    auto seconds = std::chrono::duration_cast< std::chrono::seconds >( dur );
                    uint32_t hh = uint32_t( seconds.count() ) / 3600;
                    uint32_t mm = uint32_t( seconds.count() % 3600 ) / 60;
                    uint32_t ss = uint32_t( seconds.count() % 3600 ) % 60;
                    uint32_t us = ( dur.count() % std::nano::den ) / 1000; // ns -> us

                    if ( usec > 1'000'000'000 ) // 1'268'059'000
                        std::cout << boost::format("\n%02u:%02u:%02u.%06d (%.2fs) %d %d\t|")
                            % hh % mm % ss % us % (usec/1'000'000.0) % nacc % adc_counter;
                    else if ( usec > 10'000'000 )
                        std::cout << boost::format("\n%02u:%02u:%02u.%06d (%dms) %d %d\t|") % hh % mm % ss % us % (usec/1000) % nacc % adc_counter;
                    else
                        std::cout << boost::format("\n%02u:%02u:%02u.%06d (%dus) %d %d\t|") % hh % mm % ss % us % usec % nacc % adc_counter;
                    // std::cout << boost::format("\n%x %x\t|") % clock_counter % adc_counter;

                    if ( vm.count("raw") ) {
                        for ( size_t i = 0; i < sizeof(ad)/sizeof(ad[0]); ++i )
                            std::cout << boost::format("\t0x%06x") % ad[i];
                    }

                    for ( size_t i = 0; i < sizeof(ad)/sizeof(ad[0]); ++i ) {
                        std::cout << boost::format("\t%7.1lf") % ( ( double(ad[i]) / nacc ) - offset );
                        if ( i == 3 )
                            std::cout << "\t";
                    }
                    std::cout << "\t(mV)";
                    std::cout << ", " << bits.to_string();
                    std::cout << ", " << flag_clock_counter;

                    std::chrono::system_clock::time_point tp;
                    tp += std::chrono::nanoseconds(posix_time);
                    using namespace date;
                    std::cout << ", " << tp; // std::chrono::nanoseconds( posix_time );
                }
            }
        }
        std::cout << std::endl;
        return 0;
    }
    return 0;
}
