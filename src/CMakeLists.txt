# CMakeLists.txt

add_subdirectory( tools )
add_subdirectory( drivers )

set ( DKMS_PACKAGES ${DKMS_PACKAGES} PARENT_SCOPE )
