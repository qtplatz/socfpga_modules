
project( tsensor )
set ( MODNAME tsensor )

set ( OBJM ${MODNAME}.o )
set ( SRCS ${MODNAME}.c ${MODNAME}.h )
set ( PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.ko
  COMMAND make -C ${KERNEL_SOURCE} ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KERNELRELEASE=${KERNELRELEASE} M=${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.c ${CMAKE_CURRENT_SOURCE_DIR}/${MODNAME}.h
  )

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
