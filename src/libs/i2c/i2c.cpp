// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include <boost/format.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>

using namespace i2c_linux;

i2c::i2c() : fd_( -1 )
           , address_( 0 )
{
}

i2c::~i2c()
{
    if ( fd_ >= 0 )
        ::close( fd_ );
}

bool
i2c::init( const char * device, int address )
{
    if ( fd_ > 0 )
        ::close( fd_ );

    if ( (fd_ = ::open( device, O_RDWR ) ) >= 0) {

        address_ = address;
        device_ = device;

        if ( ioctl( fd_, I2C_SLAVE, address ) < 0 ) {

            error_code_ = std::error_code( errno, std::system_category() );
            ::close( fd_ );

            fd_ = (-1);

            return false;
        }

    } else {
        error_code_ = std::error_code( errno, std::system_category() );
    }
    return true;
}

void
i2c::error_clear()
{
    error_code_ = std::error_code();
}

bool
i2c::write( const uint8_t * data, size_t size )
{
    if ( fd_ >= 0 ) {
        auto rcode = ::write( fd_, data, size );
        if ( rcode < 0 )
            error_code_ = std::error_code( errno, std::system_category() );
        return rcode == size;
    }
    return false;
}

bool
i2c::read( uint8_t * data, size_t size )
{
    if ( fd_ >= 0 ) {
        auto rcode = ::read( fd_, data, size );
        if ( rcode < 0 )
            error_code_ = std::error_code( errno, std::system_category() );
        return rcode == size;
    }
    return false;
}

bool
i2c::write( uint8_t address, const uint8_t * data, size_t size )
{
    if ( address != address_ )
        std::cerr << boost::format("%1%(%2%): ") % __FILE__ % __LINE__
                  << boost::format( "i2c::write: address does not match (0x%x == 0x%x)" ) % int(address) % int(address_);
    return write( data, size );
}

bool
i2c::read( uint8_t address, uint8_t * data, size_t size )
{
    if ( address != address_ )
        std::cerr << boost::format("%1%(%2%): ") % __FILE__ % __LINE__
                  << boost::format( "i2c::read: address does not match (0x%x == 0x%x)" ) % int(address) % int(address_);
    return read( data, size );
}
