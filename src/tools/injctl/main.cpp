/*
 * MIT License
 *
 * Copyright (c) 2021 Toshinobu Hondo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "date_time.hpp"
#include <eventbroker/eventbroker.h>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h> // close

bool __verbose = true;

int
main( int argc, char **argv )
{
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description description( argv[ 0 ] );
    {
        description.add_options()
            ( "help,h",         "Display this help message" )
            ( "device,d",       po::value< std::string >()->default_value("/dev/pio0"), "pio device" )
            ( "wait",           "wait for injection" )
            ( "replicates,r",   po::value< uint32_t >()->default_value(1), "replicates for inject wait" )
            ( "event_receiver", po::value< std::string >(), "event receiver host or bcast, ex 192.168.1.1" )
            ;
        po::positional_options_description p;
        p.add( "args",  -1 );
        po::store( po::command_line_parser( argc, argv ).options( description ).positional(p).run(), vm );
        po::notify(vm);
    }

    if ( vm.count( "help" ) ) {
        std::cerr << description;
        return 0;
    }

    if ( vm.count( "event_receiver" ) ) {
        std::cerr << "event_receiver: " << vm[ "event_receiver" ].as< std::string >();
        eventbroker_bind( vm[ "event_receiver" ].as< std::string >().c_str(), "7125", false );
    }


    size_t replicates = vm[ "replicates" ].as< uint32_t >();

    if ( vm.count( "wait" ) ) {
        uint64_t rep[ 2 ];
        int fd = ::open( vm[ "device" ].as< std::string >().c_str(), O_RDONLY );
        if ( fd > 0 ) {
            while ( replicates-- ) {
                std::cerr  << "waiting..." << std::endl;
                auto count = ::read( fd, &rep, sizeof( rep ) );
                std::cout << "read count: " << count << ", rep: " << rep[0] << ", " << rep[1] << std::endl;
                auto dur = std::chrono::nanoseconds( rep[ 0 ] );
                auto tp = std::chrono::time_point< std::chrono::system_clock >( dur );
                std::cout << adportable::date_time::to_iso< std::chrono::microseconds >( tp, true ) << std::endl;
            }
        }
    }
    return 0;
}
