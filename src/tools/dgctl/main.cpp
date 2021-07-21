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

#include "pretty_print.hpp"
#include <boost/program_options.hpp>
#include <boost/json.hpp>
#include <boost/format.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>

bool __verbose = true;

int
main( int argc, char **argv )
{
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description description( argv[ 0 ] );
    {
        description.add_options()
            ( "help,h",        "Display this help message" )
            ( "device,d",      po::value< std::string >()->default_value("/dev/dgmod0"), "dgmod device" )
            ( "list,l",        "list register" )
            ( "json,j",        "list register as json" )
            ( "commit,c",      "commit" )
            ( "set",           po::value<std::vector< uint32_t > >()->multitoken(),  "set timings" )
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

    std::array< uint32_t, 32 > data[ 2 ];
    do {
        std::ifstream in( vm[ "device" ].as< std::string >(), std::ios::binary | std::ios::in );
        in.read( reinterpret_cast< char * >( data[ 0 ].data() ), data[0].size() * sizeof( uint32_t ) );
        in.read( reinterpret_cast< char * >( data[ 1 ].data() ), data[1].size() * sizeof( uint32_t ) );
    } while ( 0 );

    if ( vm.count( "list" ) || argc == 1 ) {
        for ( size_t i = 0; i < data[ 0 ].size(); i += 2 ) {
            std::cout <<
                boost::format( "%d\t0x%08x\t0x%08x\t|\t0x%08x\t%08x" )
                % i
                % data[ 0 ].at( i )
                % data[ 0 ].at( i + 1 )
                % data[ 1 ].at( i )
                % data[ 1 ].at( i + 1 )
                      << std::endl;
        }
    }

    if ( vm.count( "json" ) ) {
        boost::json::array ja;
        boost::json::value jv = {
            { { "flags", data[0].at( 0 ) }, { "t0_set", data[0].at( 1 ) } }
                , { { "flags", data[1].at( 0 ) }, { "t0", data[1].at ( 1 ) } }
        };
        // std::cout << boost::json::serialize( jv ) << std::endl;
        ja.push_back( jv );
        for ( size_t i = 1; i < data[ 0 ].size(); i += 2 ) {
            boost::json::value jv = {
                { { "delay_s", data[ 0 ].at( i ) }, { "width_s", data[ 0 ].at( i + 1 ) } }
                , { { "delay", data[ 1 ].at( i ) }, { "width", data[ 1 ].at( i + 1 ) } }
            };
            ja.push_back( jv );
        }
        //boost::json::object jobj;
        //jobj[ "dgmod" ] = ja;
        //std::cout << boost::json::serialize( jobj ) << std::endl;
        std::cout << boost::json::serialize( ja ) << std::endl;
    }

    if ( vm.count( "set" ) ) {
        auto vec = vm[ "set" ].as< std::vector< uint32_t > >();
        for ( const auto& v: vec )
            std::cout << v << std::endl;
        if ( vec.size() >= 2 && vec[ 0 ] < 8 ) {
            size_t i = vec[ 0 ];
            uint32_t value = vec[ 12 ];
            std::ofstream out( vm[ "device" ].as< std::string >(), std::ios::binary | std::ios::out );
            out.seekp( i * 4 );
            out.write( reinterpret_cast< const char * >( &value ), sizeof( uint32_t ) );
        }
    }

    if ( vm.count( "commit" ) ) {
        std::ofstream out( vm[ "device" ].as< std::string >(), std::ios::binary | std::ios::out );
        out.write( reinterpret_cast< const char * >( data[ 0 ].at( 0 ) ), sizeof( uint32_t ) );
    }
    return 0;
}
