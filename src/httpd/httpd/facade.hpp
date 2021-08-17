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

#pragma once

#include "shared_state.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

class facade {
    facade( const facade& ) = delete;
    facade& operator = ( const facade& ) = delete;
    facade();
    boost::asio::io_context ioc_;
    std::mutex mutex_;
    std::weak_ptr< shared_state > state_;

public:
    ~facade();
    static facade * instance();

    static std::shared_ptr< shared_state > make_shared_state( const std::string& doc_root );

    typedef boost::beast::http::response<boost::beast::http::string_body> response_type;
    std::optional< response_type > handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req );

};
