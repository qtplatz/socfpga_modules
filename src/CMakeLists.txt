# CMakeLists.txt

if ( NOT APPLE )
  add_subdirectory( tools )
  add_subdirectory( drivers )
endif()
add_subdirectory( httpd )


set ( DKMS_PACKAGES ${DKMS_PACKAGES} PARENT_SCOPE )
