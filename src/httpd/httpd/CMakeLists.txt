#
#
cmake_minimum_required (VERSION 3.13.4)
project( httpd CXX )

if ( CMAKE_CROSSCOMPILING )

set ( OPENSSL_CRYPTO_LIBRARY "/usr/lib/arm-linux-gnueabihf/libcrypt.so" )
set ( OPENSSL_SSL_LIBRARY "/usr/lib/arm-linux-gnueabihf/libssl.so" )
set ( OPENSSL_INCLUDE_DIR "/usr/include" ) #arm-linux-gnueabihf/include" )
set ( OPENSSL_ROOT_DIR "/usr/lib" )

endif()

find_package( Threads REQUIRED)
find_package( OpenSSL 1.1.1 REQUIRED )

message( STATUS "## OpenSSL_FOUND:            ${OpenSSL_FOUND}" )
message( STATUS "## OPENSSL_CRYPTO_LIBRARIES: ${OPENSSL_CRYPTO_LIBRARIES}" )
message( STATUS "## OPENSSL_SSL_LIBRARIES:    ${OPENSSL_SSL_LIBRARIES}" )
message( STATUS "## OPENSSL_VERSION:          ${OPENSSL_VERSION}" )
message( STATUS "## OPENSSL_APPLINK_SOURCE:   ${OPENSSL_APPLINK_SOURCE}" )

if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  configure_file( "${PROJECT_SOURCE_DIR}/httpd.sh.in" "${PROJECT_BINARY_DIR}/${PROJECT_NAME}" @ONLY )
  configure_file( "${PROJECT_SOURCE_DIR}/postinst.in" "${PROJECT_BINARY_DIR}/postinst" @ONLY )
  configure_file( "${PROJECT_SOURCE_DIR}/prerm.in" "${PROJECT_BINARY_DIR}/prerm" @ONLY )
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${QTPLATZ_INCLUDE_DIRS}
  )

add_definitions( -DPID_NAME="/var/run/${PROJECT_NAME}.pid" -DDOC_ROOT="${HTML_INSTALL_DIR}/html" )

if ( FALSE )
  add_executable (
    ${PROJECT_NAME}
    ${BOOST_BEAST_FILES}
    common/server_certificate.hpp
    advanced_server_flex.cpp
    #advanced_server.cpp
    )
else()
  add_executable (
    ${PROJECT_NAME}
    ${BOOST_BEAST_FILES}
    common/server_certificate.hpp
    main.cpp
    listener.cpp
    http_session.cpp
    websocket_session.cpp
    shared_state.cpp
    )
endif()

target_link_libraries (
  ${PROJECT_NAME}
  OpenSSL::Crypto
  OpenSSL::SSL
  ${Boost_LIBRARIES}
  adportable
  crypto
  ssl
  )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT httpd )
install( FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME} DESTINATION /etc/init.d COMPONENT httpd )
