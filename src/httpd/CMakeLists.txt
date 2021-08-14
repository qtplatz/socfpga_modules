# CMakeLists.txt

add_subdirectory( httpd )

set ( CPACK_INSTALL_PREFIX "/opt/httpd" ) # override qtplatz-config.cmake

install ( DIRECTORY html DESTINATION ${HTML_INSTALL_DIR} COMPONENT httpd )
