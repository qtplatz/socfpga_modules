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
#include "version.h"
#include <adlog/logger.hpp>
#include <boost/format.hpp>
#include <fstream>

using namespace peripheral;

std::optional< facade::response_type >
dgmod::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    std::string method = req.method_string().to_string();
    std::string request_path = req.target().to_string();

    auto local_request = request_path.substr( prefix_size );

    if ( method == "GET" && local_request == "revision" ) {
        try {
            auto resp = GET_revision();

            boost::beast::http::string_body::value_type body( resp );
            boost::beast::http::response<boost::beast::http::string_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(boost::beast::http::status::ok, req.version())};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            if ( !resp.empty() || resp.at(0) == '{' )
                res.set(boost::beast::http::field::content_type, "text/json");
            else
                res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            return res;
        } catch ( std::exception& ex ) {
            ADTRACE() << "exception: " << ex.what();
        }
    }

    /*
    if ( http_request( method, request_path, req.body(), resp ) ) {
        boost::beast::http::string_body::value_type body( resp );
        boost::beast::http::response<boost::beast::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(boost::beast::http::status::ok, req.version())};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        if ( !resp.empty() || resp.at(0) == '{' )
            res.set(boost::beast::http::field::content_type, "text/json");
        else
            res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        return res;
    }
    */
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
        sz.first = in.read( reinterpret_cast< char * >( data[ 0 ].data() ), data[0].size() * sizeof( uint64_t ) ).gcount();
        sz.second = in.read( reinterpret_cast< char * >( data[ 1 ].data() ), data[1].size() * sizeof( uint64_t ) ).gcount();
        if ( sz.first == data[0].size() * sizeof( uint64_t ) )
            rev = static_cast< uint32_t >( data[ 0 ].at( 15 ) & 0xffff'ffff );
    } while ( 0 );

    std::ostringstream o;
    o << "<h5>Delay Generator V" << PACKAGE_VERSION;
    o << boost::format(" FPGA Rev. %d.%d.%d-%d")
        % ( ( rev >> 24 ) & 0xff ) % ( ( rev >> 16 ) & 0xff ) % ( ( rev >> 8 ) & 0xff ) % ( rev & 0xff ) << "</h5>";
    return o.str();
}
