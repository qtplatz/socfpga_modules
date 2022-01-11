// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2021-2022 MS-Cheminformatics LLC
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
#include "tsensor.hpp"
#include "bmp280.hpp"
#include "websocket_session.hpp"
#include <adlog/logger.hpp>
#include <adportable/date_time.hpp>
#include <adportable/iso8601.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/json.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include <optional>

namespace {
    struct null_t {};
    template< typename ... T > struct peripheral_list_t {};

    // terminal
    template< typename last >
    struct peripheral_list_t< last > {
        static std::optional< facade::response_type >
        do_handle( const boost::beast::http::request<boost::beast::http::string_body>& ) { return {}; }

        static bool
        handle_websock_message( const std::string& msg, websocket_session * ws ) { return false; }
    };

    template< typename first, typename ... args >
    struct peripheral_list_t< first, args ... > {
        static std::optional< facade::response_type > // bool
        do_handle( const boost::beast::http::request<boost::beast::http::string_body>& req ) {
            if ( req.target().compare( 0, first::prefix_size, first::prefix ) == 0 ) {
                return std::make_shared< first >()->handle_request( req );
            }
            return peripheral_list_t< args ... >::do_handle( req );
        }

        static bool
        handle_websock_message( const std::string& msg, websocket_session * ws ) {
            if ( std::make_shared< first >()->handle_websock_message( msg, ws ) )
                return true;
            return peripheral_list_t< args ... >::handle_websock_message( msg, ws );
        }
    };

    using peripheral_list = peripheral_list_t< peripheral::dgmod
                                               , peripheral::bmp280
                                               , peripheral::tsensor
                                               , null_t >;
}

using namespace std::chrono_literals;

class facade::impl {
public:
    boost::asio::io_context ioc_;
    boost::asio::steady_timer _1s_timer_;
    std::weak_ptr< shared_state > state_;
    std::vector< std::thread > threads_;
    size_t tick_;
    tick_handler_t tick_handler_;

    impl() : _1s_timer_( ioc_ )
           , tick_( 0 ) {}

    void start_timer() {
        auto tpi = std::chrono::floor< std::chrono::seconds >( std::chrono::steady_clock::now() ) + 5s;
        _1s_timer_.expires_at ( tpi ); //std::chrono::milliseconds( 1000 ) );
        _1s_timer_.async_wait( [this]( const boost::system::error_code& ec ){ on_timer(ec); } );
    };

    void __websock_forward( std::string&& msg, const std::string& protocol ) {
        if ( auto state = state_.lock() ) {
            state->send( std::move( msg ), protocol );
        }
    }

private:
    void on_timer( const boost::system::error_code& ec ) {
        if ( ec )
            ADTRACE() << ec;

        tick_handler_();

        auto dt = adportable::date_time::to_iso< std::chrono::microseconds >( std::chrono::steady_clock::now(), true );
        boost::json::object obj{
            { "tick", {{ "counts", tick_ }, { "tp", dt }} }
        };
        __websock_forward( boost::json::serialize( obj ), "chat" );

        auto tp = std::chrono::floor< std::chrono::seconds >( std::chrono::steady_clock::now() ) + 1s;
        _1s_timer_.expires_at ( tp );
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
    impl_->start_timer();
    for ( auto i = 0; i < 2; ++i )
        impl_->threads_.emplace_back( [&]{ impl_->ioc_.run(); } );
}

void
facade::stop()
{
    impl_->_1s_timer_.cancel();

    impl_->ioc_.stop();

    for ( auto& t: impl_->threads_ )
        t.join();
}

boost::signals2::connection
facade::register_tick_handler( const tick_handler_t::slot_type& slot )
{
    return impl_->tick_handler_.connect( slot );
}

void
facade::websock_forward( std::string&& msg, const std::string& subprotocol )
{
    impl_->__websock_forward( std::move( msg ), subprotocol );
}

void
facade::websock_onread( std::string&& msg, websocket_session * ws )
{
    if ( ! peripheral_list::handle_websock_message( msg, ws ) ) {
        impl_->__websock_forward( std::move( msg ), ws->subprotocol() );
    }
}

boost::asio::io_context&
facade::io_context()
{
    return impl_->ioc_;
}
