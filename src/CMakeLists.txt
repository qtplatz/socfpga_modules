# CMakeLists.txt

if ( NOT APPLE )
  add_subdirectory( drivers )
endif()

if ( Boost_FOUND )
  add_subdirectory( tools )
  add_subdirectory( httpd )
endif()


set ( DKMS_PACKAGES ${DKMS_PACKAGES} PARENT_SCOPE )
