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

#include "json_helper.hpp"
#include <boost/json.hpp>
#include <boost/optional.hpp>
#include <vector>

namespace {
    struct tokenizer {
        std::vector< std::string > tokens_;
        tokenizer( std::string s ) {
            size_t pos(0);
            while ( ( pos = s.find( "." ) ) != std::string::npos ) {
                tokens_.emplace_back( s.substr( 0, pos ) );
                s.erase( 0, pos + 1 );
            }
            if ( !s.empty() )
                tokens_.emplace_back( s );
        }
        std::vector< std::string >::const_iterator begin() const { return tokens_.begin(); }
        std::vector< std::string >::const_iterator end() const { return tokens_.end(); }
    };
}

std::optional< boost::json::value >
json_helper::parse( const std::string& s )
{
    boost::system::error_code ec;
    auto jv = boost::json::parse( s, ec );
    if ( ! ec )
        return jv;
    return {};
}

std::optional< boost::json::value >
json_helper::find( const boost::json::value& jv, const std::string& keys ) // dot delimited key-list
{
    tokenizer tok( keys );
    boost::json::value value = jv;
    for ( const auto& key: tok ) {
        if ( value.kind() != boost::json::kind::object ) {
            return {}; // none
        }
        if ( auto child = value.as_object().if_contains( key ) ) {
            value = *child;
        } else {
            return {};
        }
    }
    return value;
}
