// -*- C++ -*-
/**************************************************************************
MIT License

Copyright (c) 2021-2022 Toshinobu Hondo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**************************************************************************/

#pragma once

#include "facade.hpp"
#include <memory>

class websocket_session;

namespace peripheral {

    class bmp280  : public std::enable_shared_from_this< bmp280 > {
    public:
        bmp280();
        ~bmp280();

        std::optional< facade::response_type >
        handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req );

        bool
        handle_websock_message( const std::string& msg, websocket_session * );

        constexpr static const char * const subprotocol = "bmp280";
        constexpr static const char * const prefix      = "/fpga/bmp280/";
        constexpr static const size_t prefix_size       = sizeof( "/fpga/bmp280/" ) - 1;
    private:
        class impl;
        impl * impl_;
    };

};
