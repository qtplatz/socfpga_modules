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
#include "dgmod.hpp"
#include <adlog/logger.hpp>
#include <adportable/date_time.hpp>
#include <adportable/iso8601.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <optional>

namespace {
    struct null_t {};
    template< typename ... T > struct peripheral_list_t {};

    template< typename last >
    struct peripheral_list_t< last > {
        static std::optional< facade::response_type > // bool
        do_handle( const boost::beast::http::request<boost::beast::http::string_body>& ) {
            return {};
        }
    };

    template< typename first, typename ... args >
    struct peripheral_list_t< first, args ... > {
        static std::optional< facade::response_type > // bool
        do_handle( const boost::beast::http::request<boost::beast::http::string_body>& req ) {
            if ( req.target().compare( 0, first::prefix_size, first::prefix ) == 0 )
                return first().handle_request( req );
            return peripheral_list_t< args ... >::do_handle( req );
        }
    };

    using peripheral_list = peripheral_list_t< peripheral::dgmod, null_t >;
}

class facade::impl {
public:
    boost::asio::io_context ioc_;
    boost::asio::steady_timer _1s_timer_;
    std::weak_ptr< shared_state > state_;
    std::vector< std::thread > threads_;

    impl() : _1s_timer_( ioc_ ) {}

    void start_timer() {
        _1s_timer_.expires_from_now( std::chrono::milliseconds( 1000 ) );
        _1s_timer_.async_wait( [this]( const boost::system::error_code& ec ){ on_timer(ec); } );
    };

private:
    void on_timer( const boost::system::error_code& ec ) {
        if ( auto state = state_.lock() ) {
            auto dt = adportable::date_time::to_iso< std::chrono::microseconds >( std::chrono::system_clock::now(), true );
            state->send( "----- on_timer ----- " + dt );
        } else {
            ADTRACE() << "----- on_timer -----";
        }
        _1s_timer_.expires_from_now( std::chrono::milliseconds( 1000 ) );
        _1s_timer_.async_wait( [this]( const boost::system::error_code& ec ){ on_timer(ec); } );
    }

};

facade::facade() : impl_( std::make_unique< impl >() )
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
    instance()->impl_->state_ = state;
    return state;
}

std::optional< facade::response_type >
facade::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    if ( req.target().compare( 0, 6, "/fpga/" ) == 0 ) {
        return peripheral_list::do_handle( req );
    }
    return {};
}

void
facade::run()
{
    for ( auto i = 0; i < 2; ++i )
        impl_->threads_.emplace_back( [&]{ impl_->ioc_.run(); } );
    impl_->start_timer();
}

void
facade::stop()
{
    impl_->_1s_timer_.cancel();

    impl_->ioc_.stop();

    for ( auto& t: impl_->threads_ )
        t.join();
}
