// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2021 Toshinobu Hondo, Ph.D.
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid ScienceLiaison commercial licenses may use this
** file in accordance with the ScienceLiaison Commercial License Agreement
** provided with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and ScienceLiaison.
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

#include "dgmod.hpp"
#include "facade.hpp"
#include "jsonhelper.hpp"
#include "version.h"
#include <adlog/logger.hpp>
#include <boost/format.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <chrono>

namespace peripheral {

    class dgmod::impl {
        dgmod& dgmod_;
    public:
        impl( dgmod& d ) : dgmod_( d ) {}

        std::array< uint64_t, 16 > read( uint32_t page ) const {
            std::array< uint64_t, 16 > data = { 0 };
            std::ifstream in( "/dev/dgmod0", std::ios::binary | std::ios::in );
            if ( in.is_open() ) {
                for ( size_t i = 0; i <= page; ++i ) {
                    // in.seekg( page * ( 16 * sizeof( uint64_t ) ), std::ios::beg );
                    in.read( reinterpret_cast< char * >( data.data() ), data.size() * sizeof( uint64_t ) );
                }
            } /*else {
                auto tp = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::steady_clock::now().time_since_epoch() );
                data[ 13 ] = tp.count();
                data[ 14 ] = uint64_t( tp.count() ) << 32;
                }
              */
            for ( size_t i = 0; i < 16; ++i )
                ADTRACE() << boost::format( "0x%016x" ) % data.at( i );
            return data;
        }

        void operator()() const {
            auto data = read( 1 );
            boost::json::object obj{
                { "dgmod"
                  , {   { "timestamp", data.at( 13 ) }
                        , { "t0_counter", uint32_t ( data.at( 14 ) >> 32 ) }
                        , { "d0", ( boost::format("%016x") % data.at( 0 ) ).str() }
                    }
                }
            };
            ADTRACE() << boost::json::serialize( obj );
            facade::instance()->websock_forward( boost::json::serialize( obj ), "dgmod" );
        }
    };

};

using namespace peripheral;

dgmod::dgmod() : impl_( new impl( *this ) )
{
    facade::instance()->register_tick_handler( *impl_ );
}

dgmod::~dgmod()
{
    delete impl_;
}

std::optional< facade::response_type >
dgmod::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    auto method = req.method(); // _string().to_string();
    std::string request_path = req.target().to_string();
    // auto local_request = request_path.substr( prefix_size );
    auto local_request = req.target().substr( prefix_size );

    if ( method == boost::beast::http::verb::get && local_request == "revision" ) {
        auto resp = GET_revision();
        boost::beast::http::string_body::value_type body( resp );
        boost::beast::http::response<boost::beast::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        return res;
    }

    if ( method == boost::beast::http::verb::get && local_request == "ctl$status.json" ) {
        auto resp = GET_status();
        boost::beast::http::string_body::value_type body( resp );
        boost::beast::http::response<boost::beast::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/json");
        res.keep_alive(req.keep_alive());
        return res;
    }

    if ( method == boost::beast::http::verb::post && local_request == "ctl$commit" ) {
        auto resp = POST_commit( req.body() );
        boost::beast::http::string_body::value_type body( resp );
        boost::beast::http::response<boost::beast::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/json");
        res.keep_alive(req.keep_alive());
        return res;
    }

    return {};
}

std::string
dgmod::GET_revision() const
{
    uint32_t rev = 0x0000'0000; // default, use in case of debug on non arm linux platform.

    std::array< uint64_t, 16 > data[ 2 ] = { 0 };
    std::pair< size_t, size_t > sz; // read twice -- driver change io page by seek
    do {
        std::ifstream in( "/dev/dgmod0", std::ios::binary | std::ios::in );
        sz.first = in.read( reinterpret_cast< char * >( data[ 0 ].data() ), data[0].size() * sizeof( uint64_t ) ).gcount(); // p0 := set points
        sz.second = in.read( reinterpret_cast< char * >( data[ 1 ].data() ), data[1].size() * sizeof( uint64_t ) ).gcount(); // p1 := actuals
        if ( sz.first == data[0].size() * sizeof( uint64_t ) )
            rev = static_cast< uint32_t >( data[ 0 ].at( 15 ) & 0xffff'ffff );
    } while ( 0 );

    std::ostringstream o;
    o << "<h5>Delay Generator V" << PACKAGE_VERSION;
    o << boost::format(" FPGA Rev. %d.%d.%d-%d")
        % ( ( rev >> 24 ) & 0xff ) % ( ( rev >> 16 ) & 0xff ) % ( ( rev >> 8 ) & 0xff ) % ( rev & 0xff ) << "</h5>";

    return o.str();
}

std::string
dgmod::GET_status() const
{
    std::array< uint64_t, 16 > data[ 2 ] = { 0 };
    std::pair< size_t, size_t > sz; // read twice -- driver change io page by seek
    do {
        std::ifstream in( "/dev/dgmod0", std::ios::binary | std::ios::in );
        sz.first = in.read( reinterpret_cast< char * >( data[ 0 ].data() ), data[0].size() * sizeof( uint64_t ) ).gcount(); // p0 := set points
        sz.second = in.read( reinterpret_cast< char * >( data[ 1 ].data() ), data[1].size() * sizeof( uint64_t ) ).gcount(); // p1 := actuals
    } while ( 0 );

    boost::json::array a;
    for ( size_t i = 1; i < 9; ++i ) { // uint64_t slave_io[1..9] := delay_width
        a.push_back(
            boost::json::object{
                { "delay", double( data[ 0 ].at( i ) >> 32 ) * 1.0e-5 }
                , { "width", double( data[ 0 ].at( i ) & 0xffffffff ) * 1.0e-5 } }
            );
    }

    boost::json::object obj{
        { "status"
          , {   { "pulses",          a }
              , { "interval",        double( data[ 0 ].at( 0 ) & 0xffffffff ) * 1.0e-5 }
              , { "replicates",      uint32_t( data[ 0 ].at( 12 ) & 0xffffffff ) }
              , { "timestamp",       data[ 1 ].at( 13 ) }
              , { "t0_counter",      uint32_t( data[ 1 ].at( 14 ) >> 32 ) }
              , { "protocol_number", uint32_t( data[ 1 ].at( 14 ) & 07 ) }
              , { "model_number",    uint32_t( data[ 1 ].at( 15 ) >> 32 ) }
              , { "revision_number", uint32_t( data[ 1 ].at( 15 ) & 0xffffffff ) } }
        }
    };
    return boost::json::serialize( obj );

    return "{}";
}

std::string
dgmod::POST_commit( const std::string& body ) const
{
    boost::system::error_code ec;

    struct proto {
        uint32_t replicates;
        std::vector< std::pair< uint32_t, uint32_t > > pulse;
    };

    struct data {
        uint32_t interval;
        std::array< proto, 4 > proto_;
    };

    data data = { 0, { 0 }};

    auto jobj = boost::json::parse( body, ec );
    if ( !ec ) {
        if ( auto protocols = jsonhelper::get_optional( jobj, "protocols" ) ) {
            if ( auto interval = jsonhelper::get_optional( *protocols, "interval" ) ) {
                data.interval = jsonhelper::to_< double >( *interval ) * 1e5;
            }
            if ( auto protocol = jsonhelper::get_optional( *protocols, "protocol" ) ) {
                for ( auto p: protocol->as_array() ) {
                    auto proto      = p.as_object();
                    size_t index = jsonhelper::to_< int >( proto[ "index" ] );
                    if ( index < data.proto_.size() ) {
                        data.proto_[ index ].replicates = jsonhelper::to_< int >( proto[ "replicates" ] );
                        for ( auto pulse: proto.at( "pulses" ).as_array() ) {
                            auto delay = boost::json::value_to< double >( pulse.at( "delay" ) );
                            auto width = boost::json::value_to< double >( pulse.at( "width" ) );
                            data.proto_[ index ].pulse.emplace_back( delay * 1e5, width * 1e5);
                        }
                    }
                }
            }
        }

        return boost::json::serialize( boost::json::object{{ "status", "OK" }} );
    }

    return boost::json::serialize( boost::json::object{{ "status", "ERROR" }} );
}
