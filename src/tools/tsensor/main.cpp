/**************************************************************************
MIT License

Copyright (c) 2021-2022 Toshinobu Hondo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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

static const char *__tsensor_device = "/dev/tsensor0";

int
main( int argc, char * argv [] )
{
    namespace po = boost::program_options;

    po::variables_map vm;
    po::options_description description( "spi" );
    {
        description.add_options()
            ( "help,h",    "Display this help message" )
            ( "device,d",  po::value< std::string >()->default_value( __tsensor_device ), "dma device name '/dev/tsensor0'" )
            ( "count,c",   po::value< std::string >(), "count" )
            ( "set",       po::value< int >(), "temp setpoint" )
            ;
        po::store( po::command_line_parser( argc, argv ).options( description ).run(), vm );
        po::notify(vm);
    }

    if ( vm.count( "help" ) ) {
        std::cerr << description;
        return 0;
    }

    const std::string tsensor_device = vm["device"].as< std::string >();
    size_t count = (-1);
    if ( vm.count("count") )
        count = std::strtol( vm["count"].as< std::string >().c_str(), 0, 0 );

    if ( vm.count( "set" ) ) {
        int32_t setpt = vm[ "set" ].as< int >();
        std::cerr << "setpoint: " << setpt << std::endl;
        std::ofstream outf;
        outf.open( tsensor_device, std::ios::out | std::ios::binary );
        if ( outf.is_open() ) {
            outf.write( reinterpret_cast< const char * >( &setpt ), sizeof( setpt ));
        }
    }

    std::ifstream file;
    file.open( tsensor_device, std::ios::in | std::ios::binary );

    if ( file.is_open() ) {
        uint32_t data[ 3 ]; // setpt, actual, sw

        while ( count-- ) {
            if ( file.read( reinterpret_cast< char * >(data), sizeof( data ) ) ) {
                std::cout << boost::format( "%04x, %04x\t%.1f\t%.1f (degC)\t" )
                    % data[ 0 ] % data[ 1 ] % data[ 0 ] % ( data[ 1 ] * 1023.75 / 0xfff )
                          << std::bitset< 12 >( data[ 1 ] ).to_string()
                          << "\t" << std::bitset< 2 >( data[ 2 ] ).to_string()
                          << std::endl;
                file.seekg( 0 ); // rewind
            }
        }
        std::cout << std::endl;
        return 0;
    }
    return 0;
}
