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

#include "bmp280.hpp"
#include "facade.hpp"
#include "websocket_session.hpp"
#include <adlog/logger.hpp>
#include <boost/format.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <chrono>

namespace peripheral {

    class bmp280::impl {
    public:
        impl() {}
    };

};

using namespace peripheral;

bmp280::bmp280() : impl_( new impl )
{
}

bmp280::~bmp280()
{
    delete impl_;
}

std::optional< facade::response_type >
bmp280::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    return {};
}

bool
bmp280::handle_websock_message( const std::string& msg, websocket_session * ws )
{
    return ws->subprotocol() == this->subprotocol;
}
