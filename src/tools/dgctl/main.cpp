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
#include <ratio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

bool __verbose = true;

class protocol {
public:
    uint32_t user_flags;
    double interval;
    uint32_t replicates;
    std::array< std::pair< double, double >, 9 > delay_pulse;
    protocol() : user_flags( 0 )
               , interval( 0 )
               , replicates( 0 ) {
    }
};

class dgmod {
public:
    std::array< uint64_t, 16 > read( uint32_t page ) const {
        std::array< uint64_t, 16 > data = { 0 };
        std::ifstream in( "/dev/dgmod0", std::ios::binary | std::ios::in );
        if ( in.is_open() ) {
            ssize_t pos = page * (16 * sizeof( uint64_t ));
            in.seekg( pos, std::ios::beg );
            auto gcount = in.read( reinterpret_cast< char * >( data.data() ), data.size() * sizeof( uint64_t ) ).gcount();
            if ( gcount != data.size() * sizeof( uint64_t ) )
                std::cerr << "read count " << gcount << " does not match with " << data.size() * sizeof( uint64_t );
        }
        return data;
    }

    protocol as_protocol( const std::array< uint64_t, 16 >& t ) const {
        protocol p;
        p.user_flags = t[ 0 ] >> 32;
        p.interval = double( t[ 0 ] & 0xffffffff ) / ( std::micro::den / 10 );
        for ( size_t i = 1; i <= 9; ++i ) {
            p.delay_pulse[ i - 1 ] = std::make_pair(
                double( t[ i ] >> 32 ) / ( std::micro::den / 10 )
                , double( t[ i ] & 0xffff'ffff ) / ( std::micro::den / 10 )
                );
        }
        return p;
    }
};

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
            ( "pages,p",       po::value<std::vector< uint32_t > >()->multitoken(),  "pages [0..4] to be listed." )
            ( "json,j",        "list register as json" )
            ( "commit,c",      "commit" )
            ( "mmap",          "use mmap" )
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

    if ( vm.count( "mmap" ) ) {
        struct slave_data64 {
            uint64_t user_dataout;    // 0x00
            uint64_t user_datain;     // 0x08
            uint64_t irqmask;         // 0x10
            uint64_t edge_capture;    // 0x18
            uint64_t resv[12];        //
        };

        std::cerr << "-------------- memory map test ------------- file: "
                  <<  vm[ "device" ].as< std::string >() << "unit size: " << sizeof( slave_data64 ) << std::endl;

        int fd = ::open( vm[ "device" ].as< std::string >().c_str(), O_RDWR );
        if ( fd > 0 ) {
            void * mp = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
            if( mp == reinterpret_cast< void * >(-1) ){
                perror( "mmap error" );
                close(fd);
                exit(1);
            }

            std::cout << "mapped address = " << std::hex << mp << std::endl;
            volatile slave_data64 * iop = reinterpret_cast< volatile slave_data64 *>( mp );
            iop[ 15 ].user_datain = 0x00000001LL << 32;
            for ( size_t i = 0; i < 16; ++i ) {
                auto p = &iop[ i ];
                std::cout << boost::format( "[%2d] %016x\t%016x\t%016x\t%016x" )
                    % i
                    % p->user_dataout
                    % p->user_datain
                    % p->irqmask
                    % p->edge_capture
                          << std::endl;
            }
        } else {
            perror("open failed");
        }
        return 0;
    }

    if ( vm.count( "pages" ) ) {
        auto vec = vm[ "pages" ].as< std::vector< uint32_t > >();
        std::ifstream in( vm[ "device" ].as< std::string >(), std::ios::binary | std::ios::in );
        if ( in.is_open() ) {
            std::vector< std::pair< uint32_t, std::array< uint64_t, 16 > > > data;
            for ( auto& page: vec ) {
                if ( page <= 4 )
                    data.emplace_back( page, dgmod().read( page ) );
            }
            for ( size_t i = 0; i < 16; ++i ) {
                std::cout << boost::format( "%2d" ) % i;
                for ( size_t k = 0; k < data.size(); ++k )
                    std::cout << boost::format( "\t0x%016x" ) % data.at(  k ).second.at( i );
                std::cout << std::endl;
            }
        }
        return 0;
    }

    if ( vm.count( "json" ) ) {
        boost::json::array ja;
        for ( size_t i = 0; i < 4; ++i ) {
            auto proto = dgmod().as_protocol( dgmod().read( i ) );
            boost::json::array a;
            for ( auto& value: proto.delay_pulse ) {
                a.emplace_back( boost::json::object{ {"delay", value.first}, {"width", value.second} } );
            }

            ja.emplace_back(
                boost::json::object{ { "protocol"
                        , {
                            { "user_flags", proto.user_flags }
                            , { "interval", proto.interval   }
                            , { "pulses", a }
                        }
                    } } );
        }
        boost::json::object json{{ "user_protocol", ja }};
        std::cout << boost::json::serialize( json ) << std::endl;
        return 0;
    };

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
        if ( out.is_open() ) {
            out.seekp( sizeof( uint64_t ) * 15, std::ios::beg );
            out.write( "\0\0\0\0", 4 );
        }
    }
    return 0;
}
