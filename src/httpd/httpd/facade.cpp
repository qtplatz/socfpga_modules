// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2021 MS-Cheminformatics LLC
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

#include "facade.hpp"
#include "shared_state.hpp"
#include "slave_io.hpp"
#include <memory>
#include <optional>

namespace {
    struct null_t {};
    template< typename ... T > struct peripheral_list_t {};

    template< typename last >
    struct peripheral_list_t< last > {
        static std::optional< facade::response_type > // bool
        can_handle( const boost::beast::http::request<boost::beast::http::string_body>& ) {
            return {};
        }
    };

    template< typename first, typename ... args >
    struct peripheral_list_t< first, args ... > {
        static std::optional< facade::response_type > // bool
        can_handle( const boost::beast::http::request<boost::beast::http::string_body>& req ) {
            if ( req.target().compare( 0, first::prefix_size, first::prefix ) == 0 )
                return first::instance()->handle_request( req );
            return peripheral_list_t< args ... >::can_handle( req );
        }
    };
    using peripheral_list = peripheral_list_t< peripheral::slave_io, null_t >;
}

facade::facade()
{
}

facade::~facade()
{
}

facade *
facade::instance()
{
    static facade __instance;
    return &__instance;
}

std::shared_ptr< shared_state >
facade::make_shared_state( const std::string& doc_root )
{
    auto state = std::make_shared< class shared_state >( doc_root );
    instance()->state_ = state;
    return state;
}

std::optional< facade::response_type >
facade::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    return {};
}
