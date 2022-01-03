/**************************************************************************
** Copyright (C) 2016-2019 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2019 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
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

#include <i2c/bmp280.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <ostream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace po = boost::program_options;

int
main( int argc, char * argv [] )
{
    po::variables_map vm;
    po::options_description description( "bmp280" );
    {
        description.add_options()
            ( "help,h",    "Display this help message" )
            ( "device,d",   po::value<std::string>()->default_value("/dev/i2c-2", "device name e.g. /dev/i2c-1" ) )
            ( "address,a",  po::value<std::string>()->default_value("0x76", "device address") )
            ( "replicates", po::value< int >()->default_value( 1, "# replicates" ) )
            ( "interval",   po::value< double >()->default_value( 0.1, "interval (s)" ) )
            ;
        po::store( po::command_line_parser( argc, argv ).options( description ).run(), vm );
        po::notify(vm);
    }

    if ( vm.count( "help" ) ) {
        std::cout << description;
        return 0;
    }

    const std::string filename = vm["device"].as< std::string >();
    int address = std::strtoul( vm["address"].as< std::string >().c_str(), 0, 0 );

    auto replicates = vm[ "replicates" ].as< int >();

    i2c_device::bmp280::BMP280 bmp280;
    using namespace std::chrono_literals;

    if ( bmp280.open( filename, address ) ) {
        if ( bmp280.measure() ) {
            while ( replicates-- > 0 ) {
                std::this_thread::sleep_for(1s);
                auto pair = bmp280.readout();
                std::cout << int( pair.first ) << " (Pa)\t" << boost::format( "%.2lf (degC)" ) % (double(pair.second)/100.0) << std::endl;
            }
        }
    } else {
        std::cerr << "bmp280.open failed" << std::endl;
    }
}
