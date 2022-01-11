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

#include "tsensor.hpp"
#include "facade.hpp"
#include "json_helper.hpp"
#include "json_extract.hpp"
#include "websocket_session.hpp"
#include <adlog/logger.hpp>
#include <adportable/date_time.hpp>
#include <adportable/iso8601.hpp>
#include <boost/format.hpp>
#include <boost/json.hpp>
#include <bitset>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>

extern bool __simulate__;

namespace peripheral {

    static std::atomic_int32_t atomic_counter = 0;
    static const char *__tsensor_device = "/dev/tsensor0";

    uint32_t temp_device( double temp ) { return temp * 4096.0 / 1024.0; };

    struct tsensor_datum {
        double   actual;
        double   setpt;
        bool     relay;
        bool     enable;
        uint32_t id;
        uint64_t tp;
        tsensor_datum( double _1 = 0, double _2 = 0, bool _3 = false, bool _4 = false, uint32_t _5 = 0, uint64_t _6 = 0 )
            : actual( _1 )
            , setpt ( _2 )
            , relay ( _3 )
            , enable( _4 )
            , id    ( _5 )
            , tp    ( _6 ) {
        }
    };

    void
    tag_invoke( boost::json::value_from_tag, boost::json::value& jv, const tsensor_datum& t ) {
        jv = boost::json::object{{ "tsensor", {
                    { "data", {
                            { "act",      t.actual }
                            , { "set",    t.setpt }
                            , { "flag",   t.relay }
                            , { "enable", t.enable }
                            , { "id", t.id } }
                    }
                    , { "tick", {{ "tp",  t.tp }}
                    }
                }
            }};
    }

    tsensor_datum
    tag_invoke( boost::json::value_to_tag< tsensor_datum >&, const boost::json::value& jv ) {
        tsensor_datum t;
        if ( auto dt = json_helper::find( jv, "tsensor.data" ) ) {
            auto obj = dt->as_object();
            json_extract( obj, t.actual, "act" );
            json_extract( obj, t.setpt,  "set" );
            json_extract( obj, t.relay,  "flag" );
            json_extract( obj, t.enable, "enable" );
            json_extract( obj, t.id,     "id" );
        }
        if ( auto tick = json_helper::find( jv, "tsensor.tick" ) ) {
            json_extract( tick->as_object(), t.tp, "tp" );
        }
        return t;
    }

    namespace {
        class singleton {
            static std::once_flag once_flag_;
            static std::mutex mutex_;
            std::ifstream file_;
            uint32_t data_[ 4 ];
            std::atomic_int32_t error_count_;
            std::vector< tsensor_datum > tsensor_data_;

            singleton() : file_( __tsensor_device, std::ios_base::in )
                   , data_{ 0 }
                   , error_count_( 0 ) {
            }
        public:
            static singleton * instance() {
                static singleton __instance;
                return &__instance;
            }

            bool set_temperature( double setpt ) {
                uint32_t device_value = temp_device( setpt );
                std::ofstream outf( __tsensor_device, std::ios_base::out );
                if ( outf.is_open() ) {
                    outf.seekp( 0 );
                    outf.write( reinterpret_cast< const char * >( &device_value ), sizeof( device_value ));
                    return true;
                } else if ( __simulate__ ) {
                    data_[ 0 ] = device_value;
                }
                return __simulate__ ? true : false;
            }

            bool set_onoff( bool flag ) {
                std::ofstream outf( __tsensor_device, std::ios_base::out );
                uint32_t device_value = (flag << 1);
                if ( outf.is_open() ) {
                    outf.seekp( 0 );
                    outf.seekp( 2 * sizeof(device_value) );
                    outf.write( reinterpret_cast< const char * >( &device_value ), sizeof( device_value ));
                    return true;
                } else if ( __simulate__ ) {
                    data_[ 2 ] = (device_value & 0x02) | (data_[2] & ~0x02);
                }
                return __simulate__ ? true : false;
            }

            std::optional< std::tuple< double, double, bool, bool, uint32_t > > read_tsensor() {
                auto to_celsius = []( const uint32_t d ){ return d * 1024.0 / 4096.0; };
                std::scoped_lock< std::mutex > lock( mutex_ );
                if ( file_.is_open() ) {
                    file_.seekg( 0 );
                    file_.read( reinterpret_cast< char * >( data_ ), sizeof( data_ ) );
                    std::bitset< 2 > flags( data_[ 2 ] );
                    return {{ to_celsius( data_[ 1 ] ), to_celsius( data_[ 0 ] ), flags[0], flags[1], data_[ 3 ] }};
                } else if ( __simulate__ ) {
                    data_[ 3 ]++;
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for( 3s );
                    return {{ to_celsius( data_[ 1 ] ), to_celsius( data_[ 0 ] ), data_[2]&0x01, data_[2]&0x02, data_[ 3 ] }};
                }
                return {};
            }

            std::shared_ptr< std::string >
            async_read_start( std::shared_ptr< tsensor > me ) {
                std::call_once( once_flag_, [this,me]{
                    ADTRACE() << "tsensor::async_read start";
                    facade::instance()->io_context().post( [this,me]{ async_read( me ); } );
                });
                std::scoped_lock< std::mutex > lock( mutex_ );
                if ( tsensor_data_.empty() )
                    return nullptr;
                return std::make_shared< std::string >( boost::json::serialize( boost::json::value_from( tsensor_data_ ) ) );
            }

        private:
            void async_read( std::shared_ptr< tsensor > me ) {
                using namespace std::chrono;
                uint64_t tp = duration_cast< milliseconds >( std::chrono::system_clock::now().time_since_epoch() ).count();

                if ( auto tdata = read_tsensor() ) {
                    auto [ actual, setpt, temp_control, master_control, read_count ] = *tdata;
                    tsensor_data_.emplace_back( actual, setpt, temp_control, master_control, read_count, tp );
                    if ( tsensor_data_.size() > 4800 ) // 4 hours
                        tsensor_data_.erase( tsensor_data_.begin(), tsensor_data_.begin() + 1200 ); // reduce to 3 hrs
                    auto jv = boost::json::value_from( tsensor_data_.back() );
                    facade::instance()->websock_forward( boost::json::serialize( jv ), "tsensor" );
                } else {
                    auto jv = boost::json::value{{ "tsensor",
                                                   {{ "error",    "device read error" }
                                                    , { "device",  __tsensor_device   }
                                                    , { "is_open",  file_.is_open()   }}
                        }
                    };
                    if ( ++error_count_ > 100 ) {
                        return;
                    }
                }
                facade::instance()->io_context().post( [this,me]{ async_read( me ); } );
            }

        };
        std::once_flag singleton::once_flag_;
        std::mutex singleton::mutex_;
    }
};


using namespace peripheral;

tsensor::tsensor()
{
    ADTRACE() << "### tsensor CTOR ### " << atomic_counter++;
}

tsensor::~tsensor()
{
    ADTRACE() << "### tsensor DTOR ### " << --atomic_counter;
}

// ajax entry point -- not in use
std::optional< facade::response_type >
tsensor::handle_request( const boost::beast::http::request<boost::beast::http::string_body>& req )
{
    return {};
}

bool
tsensor::handle_websock_message( const std::string& msg, websocket_session * ws )
{
    ADTRACE() << msg;
    // initial message should be: {"tsensor":{"msg":"hello"}}
    if ( ws->subprotocol() == this->subprotocol ) {
        if ( auto jv = json_helper::parse( msg ) ) {
            if ( auto top = json_helper::find( *jv, "tsensor" ) ) {
                if ( auto msg = json_helper::value_to< std::string >( *top, "msg" ) ) {
                    if ( *msg == "hello" ) {
                        if ( auto ss = singleton::instance()->async_read_start( this->shared_from_this() ) ) {
                            ws->send( ss );
                        }
                    }
                }
                if ( auto setpt = json_helper::value_to< double >( *top, "set" ) ) {
                    singleton::instance()->set_temperature( *setpt );
                }
                if ( auto enable = json_helper::value_to< double >( *top, "enable" ) ) {
                    singleton::instance()->set_onoff( *enable );
                }
            }
        }
        return true;
    }
    return false;
}
