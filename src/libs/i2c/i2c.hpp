// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//


#include <array>
#include <atomic>
#include <cstdint>
#include <system_error>

namespace i2c_linux {

    class i2c {
        i2c( const i2c& ) = delete;
        i2c& operator = ( const i2c& ) = delete;
        int fd_;
        int address_;
        std::string device_;
        std::error_code error_code_;

    public:

        i2c();
        ~i2c();

        const std::error_code& error_code() const { return error_code_; }
        void error_clear();

        bool init( const char * device = "/dev/i2c-2", int address = 0x10 );

        inline operator bool () const { return fd_ != (-1); }

        inline int address() const { return address_; }
        inline const std::string& device() const { return device_; }

        bool write( const uint8_t * data, size_t );
        bool read( uint8_t * data, size_t );

        bool write( uint8_t address, const uint8_t * data, size_t );
        bool read( uint8_t address, uint8_t * data, size_t );
    };

}
