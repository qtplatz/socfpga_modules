//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

//------------------------------------------------------------------------------
/*
    WebSocket chat server, multi-threaded

    This implements a multi-user chat room using WebSocket. The
    `io_context` runs on any number of threads, specified at
    the command line.

*/
//------------------------------------------------------------------------------

#include "listener.hpp"
#include "shared_state.hpp"
#include "version.h"
#include <adportable/debug.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>

int __verbose_level__ = 0;
bool __debug_mode__ = false;
bool __simulate__ = false;

int
main( int argc, char *argv[] )
{
    namespace po = boost::program_options;
    po::variables_map vm;
    try {
        po::options_description desc("options");
        desc.add_options()
            ( "help", "print help message" )
            ( "version", "print version number" )
            ( "port",     po::value<std::string>()->default_value("80"), "http port number" )
            ( "recv",     po::value<std::string>()->default_value("0.0.0.0"), "For IPv4, try 0.0.0.0, IPv6, try 0::0" )
            ( "doc_root", po::value<std::string>()->default_value( DOC_ROOT ), "document root" )
            ( "verbose",  po::value<int>()->default_value(0), "verbose level" )
            ( "debug,d",  "debug mode" )
            ( "simulate", "simulate mode" )
            ( "event_receiver", po::value<std::string>(), "event receiver host or bcast, ex 192.168.1.1" )
            ( "threads",  po::value<int>()->default_value(2), "number of server threads" )
            ;
        po::store( po::command_line_parser( argc, argv ).options( desc ).run(), vm );
        po::notify( vm );

        if ( vm.count( "help" ) ) {
            std::cout << desc;
            return 0;
        } else if ( vm.count( "version" ) ) {
            std::cout << PACKAGE_VERSION << std::endl;
            return 0;
        }

        __debug_mode__ = vm.count( "debug" ) > 0 ;
        __simulate__ = vm.count( "simulate" ) > 0 ;

        if ( ! __debug_mode__ ) {
            int fd = open( PID_NAME, O_RDWR|O_CREAT, 0644 );
            if ( fd < 0 ) {
                std::cerr << "Can't open " PID_NAME << std::endl;
                exit(1);
            }
            int lock = lockf( fd, F_TLOCK, 0 );
            if ( lock < 0 ) {
                std::cerr << "Process " << argv[0] << " already running" << std::endl;
                exit(1);
            }
            std::ostringstream o;
            o << getpid() << std::endl;
            write( fd, o.str().c_str(), o.str().size() );
        }
    }  catch (std::exception& e)  {
        std::cerr << "exception: " << e.what() << "\n";
    }
    // Initialise the server.

    // Check command line arguments.
    // if ( argc != 5 ) {
    //     std::cerr << "Usage: websocket-chat-multi <address> <port> <doc_root> <threads>\n"
    //               << "Example:\n"
    //               << "    websocket-chat-server 0.0.0.0 8080 . 5\n";
    //     return EXIT_FAILURE;
    // }
    auto address = net::ip::make_address( vm[ "recv" ].as<std::string>() );
    auto port = vm[ "port" ].as< std::string >(); // static_cast<unsigned short>( std::atoi( argv[2] ) );
    auto doc_root = vm[ "doc_root" ].as< std::string >(); // argv[3];
    auto const threads = std::max<int>( 1, vm[ "threads" ].as< int >() ); // std::atoi( argv[4] ) );

    // The io_context is required for all I/O
    net::io_context ioc;

    boost::asio::ip::tcp::resolver resolver(ioc);
    boost::asio::ip::tcp::resolver::query query( vm[ "recv" ].as< std::string >(), port ); // "daytime");
    auto endpoint = *resolver.resolve( query );
    uint16_t port_number = static_cast< const boost::asio::ip::tcp::endpoint& >(endpoint).port();

    ADDEBUG() << "hostname: " << endpoint.host_name() << ":"
              << static_cast< const boost::asio::ip::tcp::endpoint& >(endpoint).port();

    // Create and launch a listening port
    boost::make_shared< listener >( ioc
                                    , tcp::endpoint{ address, port_number }
                                    , boost::make_shared< shared_state >( doc_root ) )->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals( ioc, SIGINT, SIGTERM );
    signals.async_wait( [&ioc]( boost::system::error_code const &, int ) {
        // Stop the io_context. This will cause run()
        // to return immediately, eventually destroying the
        // io_context and any remaining handlers in it.
        ioc.stop();
    } );

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve( threads - 1 );
    for ( auto i = threads - 1; i > 0; --i )
        v.emplace_back( [&ioc] { ioc.run(); } );
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    for ( auto &t : v )
        t.join();

    return EXIT_SUCCESS;
}
