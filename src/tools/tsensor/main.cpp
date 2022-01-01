/**************************************************************************
** Copyright (C) 2016-2022 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2022 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
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
    size_t count = 10;
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
        uint32_t data[ 2 ]; // setpt, actual

        while ( count-- ) {
            if ( file.read( reinterpret_cast< char * >(data), sizeof( data ) ) ) {
                std::cout << boost::format( "%04x, %04x\t%.1f\t%.1f (degC)" )
                    % data[ 0 ] % data[ 1 ] % data[ 0 ] % ( data[ 1 ] * 1023.75 / 0xfff ) << std::endl;
                file.seekg( 0 ); // rewind
            }
        }
        std::cout << std::endl;
        return 0;
    }
    return 0;
}
