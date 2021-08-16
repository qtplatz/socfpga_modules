# CMakeLists.txt

add_subdirectory( httpd )

install ( DIRECTORY html DESTINATION ${HTML_INSTALL_DIR} COMPONENT httpd )
