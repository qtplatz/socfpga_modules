#
#
cmake_minimum_required (VERSION 3.19)
project( httpd CXX )

if ( CMAKE_CROSSCOMPILING )

set ( OPENSSL_CRYPTO_LIBRARY "/usr/lib/arm-linux-gnueabihf/libcrypt.so" )
set ( OPENSSL_SSL_LIBRARY "/usr/lib/arm-linux-gnueabihf/libssl.so" )
set ( OPENSSL_INCLUDE_DIR "/usr/include" ) #arm-linux-gnueabihf/include" )
set ( OPENSSL_ROOT_DIR "/usr/lib" )

endif()

find_package( OpenSSL 1.1.1 REQUIRED )

message( STATUS "## OpenSSL_FOUND:            ${OpenSSL_FOUND}" )
message( STATUS "## OPENSSL_CRYPTO_LIBRARIES: ${OPENSSL_CRYPTO_LIBRARIES}" )
message( STATUS "## OPENSSL_SSL_LIBRARIES:    ${OPENSSL_SSL_LIBRARIES}" )
message( STATUS "## OPENSSL_VERSION:          ${OPENSSL_VERSION}" )
message( STATUS "## OPENSSL_APPLINK_SOURCE:   ${OPENSSL_APPLINK_SOURCE}" )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  )

add_executable (
  ${PROJECT_NAME}
  ${BOOST_BEAST_FILES}
  common/server_certificate.hpp
  #advanced_server_flex.cpp
  advanced_server.cpp
  )

target_link_libraries (
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  OpenSSL::Crypto
  OpenSSL::SSL
  )


#[[
if (OPENSSL_FOUND)
    GroupSources(include/boost/beast beast)
    GroupSources(example/common common)
    GroupSources(example/advanced/server-flex "/")

    add_executable (advanced-server-flex
        ${BOOST_BEAST_FILES}
        ${PROJECT_SOURCE_DIR}/example/common/server_certificate.hpp
        Jamfile
        advanced_server_flex.cpp
    )

    set_property(TARGET advanced-server-flex PROPERTY FOLDER "example-advanced-server")

    target_link_libraries (advanced-server-flex
        OpenSSL::SSL OpenSSL::Crypto
        lib-asio
        lib-asio-ssl
        lib-beast
        )

endif()
]]
