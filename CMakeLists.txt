#
cmake_minimum_required ( VERSION 3.7 )  # Xilinx Vitis (2020.2) contains 3.3.2, which does not support C++17
cmake_policy( SET CMP0020 NEW )
cmake_policy( SET CMP0042 NEW )
cmake_policy( SET CMP0053 NEW )

project ( socfpga )

set ( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set ( CMAKE_INSTALL_PREFIX /usr/local )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" )
include (version)

if ( CMAKE_CROSSCOMPILING )
  message( STATUS "------------- CROSSCOMPILING -------------" )
  set ( VERBOSE TRUE )
endif()

file( STRINGS "${CMAKE_SOURCE_DIR}/config.sh" configs )
foreach( i ${configs} )
  if ( NOT "${i}" MATCHES "^#.*" )
    string( REGEX MATCHALL "[^=]+=.+$" config "${i}" )
    string( REGEX REPLACE "([^=]+)=(.+$)" "\\1" key "${i}" )
    if ( "${key}" MATCHES "^(distro|cross_target|target|target_device|cross_kernel_source)$" )
      string( REGEX REPLACE "([^=]+)=(.+$)" "\\2" value ${i} )
      set( "${key}" "${value}" )
    endif()
  endif()
endforeach()

set ( KERNEL_DIR )
if ( CMAKE_CROSSCOMPILING AND cross_kernel_source )
  find_path( KERNEL_DIR NAMES "Kconfig" HINTS "$ENV{HOME}/${cross_kernel_source}" NO_CMAKE_FIND_ROOT_PATH REQUIRED )
  message( STATUS "######## KERNEL DIR: ${KERNEL_DIR}" )
  execute_process( COMMAND make kernelversion
    WORKING_DIRECTORY "${KERNEL_DIR}"
    OUTPUT_VARIABLE KERNELRELEASE  OUTPUT_STRIP_TRAILING_WHITESPACE )
  if ( NOT KERNELRELEASE )
    message( FATAL_ERROR "KERNELRELEASE: " ${KERNELRELEASE} )
  endif()
endif()


list ( APPEND CMAKE_INSTALL_RPATH ${CPACK_INSTALL_PREFIX}/lib )
set ( CPACK_SET_DESTDIR OFF )         # override qtplatz-config.cmake
set ( CPACK_INSTALL_PREFIX "/" )      # override qtplatz-config.cmake
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) # static libs, dll's import lib/exp
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) # plugins
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin ) # .exe | .dll
set ( CPACK_PACKAGING_INSTALL_PREFIX "/" ) # <-- must be root for "/lib/module/4.4.52/..."

include( version )
set ( CMAKE_CXX_STANDARD 17 )
message( STATUS "## version: " "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}(${VERSION_HASH})" )

find_package( Boost 1.75 REQUIRED COMPONENTS
  date_time
  filesystem
  json
  program_options
  regex
  system
  thread
  )

set ( DKMS_PACKAGES ) # This variable will be filled via drivers/dkms/dkms.cmake
set ( DKMS_PACKAGE_PREFIX "dkms-" )

add_subdirectory ( src )

include( CPackComponent )
cpack_add_component( socfpga  DISPLAY_NAME "socfpga" GROUP tools )
cpack_add_component_group( tools )

set( CPACK_DEB_COMPONENT_INSTALL 1 )
set( CPACK_PACKAGE_NAME socfpga_modules )
set( CPACK_PACKAGE_VENDOR "MS-Cheminformatics LLC" )
set( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )
set( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH} )
set( CPACK_GENERATOR "DEB" )
set( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "T. Hondo <toshi.hondo@qtplatz.com>" )
set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf )

###############################################
# dkms PACKAGES ---------
message( STATUS "### top level DKMS_PACKAGES=${DKMS_PACKAGES}" )

foreach( i ${DKMS_PACKAGES} )
  unset ( DKMS_COMPONENT )
  string( TOUPPER "${DKMS_PACKAGE_PREFIX}${i}" DKMS_COMPONENT )

  # build control_extra list
  unset ( __postinst CACHE )
  unset ( __prerm CACHE )
  unset ( __control_extra CACHE )
  find_file( __postinst "postinst" HINTS "${CMAKE_BINARY_DIR}/src/drivers/${i}/dkms/" NO_CMAKE_FIND_ROOT_PATH REQUIRED )
  set ( __control_extra ${__postinst} )
  find_file( __prerm "prerm" HINTS "${CMAKE_BINARY_DIR}/src/drivers/${i}/dkms/" NO_CMAKE_FIND_ROOT_PATH REQUIRED )
  list ( APPEND __control_extra ${__prerm} )
  if ( VERBOSE )
    message( STATUS "__control_extra: " ${__control_extra} )
  endif()
  # end control_extra

  set ( "CPACK_DEBIAN_${DKMS_COMPONENT}_PACKAGE_CONTROL_EXTRA" "${__control_extra}" )
  set ( "CPACK_DEBIAN_${DKMS_COMPONENT}-PACKAGE_DEPENDS" dkms )
  message( STATUS "## DKMS PACKAGE: ${i} ${DKMS_COMPONENT}" )
endforeach()
#---------------------------------

include( CPack )
