// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2010-2017 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2017 MS-Cheminformatics LLC
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

#pragma once

#include "facade.hpp"
#include <memory>

class websocket_session;

namespace peripheral {

    class dgmod : public std::enable_shared_from_this< dgmod > {
    public:
        dgmod();
        ~dgmod();

        std::optional< facade::response_type >
        handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req );

        bool handle_websock_message( const std::string& msg, websocket_session * );

        constexpr static const char * const subprotocol = "dgmod";
        constexpr static const char * const prefix = "/fpga/dgmod/";
        constexpr static const size_t prefix_size = sizeof( "/fpga/dgmod/" ) - 1;
    private:
        std::string GET_revision() const;
        std::string GET_status() const;
        std::string POST_commit( const std::string& ) const;
        class impl;
        impl * impl_;
    };

};
