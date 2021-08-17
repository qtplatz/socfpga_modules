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

#include "slave_io.hpp"
#include <adlog/logger.hpp>

using namespace peripheral;

std::optional< facade::response_type >
slave_io::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    /*
    std::string method = req.method_string().to_string();
    std::string request_path = req.target().to_string();
    std::string resp;

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
