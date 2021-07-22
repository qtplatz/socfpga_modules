
project( dtbocfg )
set ( MODNAME dtbocfg )

set ( OBJM ${MODNAME}.o )
set ( SRCS ${MODNAME}.c )
set ( PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )

if ( CMAKE_CROSSCOMPILING )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.ko
    COMMAND make
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.c
    )
endif()

add_custom_target( ${MODNAME}.ko ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.ko )

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/../template/Makefile.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
  @ONLY
  )

#--------------- DKMS ------------------
if ( DKMS )
  include( ${DKMS}/dkms.cmake )
endif()
#--------------- DKMS ------------------
