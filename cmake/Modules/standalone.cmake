
find_library( adio_LIBRARY NAMES adio PATHS "/opt/qtplatz/lib/qtplatz" )
add_library( adio STATIC IMPORTED )
set_target_properties( adio PROPERTIES IMPORTED_LOCATION ${adio_LIBRARY} )

find_library( adportable_LIBRARY NAMES adportable PATHS "/opt/qtplatz/lib/qtplatz" )
if ( adportable_LIBRARY )
  message( STATUS "adportable_LIBRARY: " ${adportable_LIBRARY} )
  add_library( adportable STATIC IMPORTED )
  set_target_properties( adportable PROPERTIES IMPORTED_LOCATION ${adportable_LIBRARY} )
else()
  message( FATAL_ERROR "adportable_LIBRARY: " ${adportable_LIBRARY} )
endif()

if ( CMAKE_COMPILER_IS_GNUCC )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals -Wno-psabi" )
endif()

include_directories(
  "/opt/qtplatz/include"
  ${CMAKE_CURRENT_SOURCE_DIR}/../drivers
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

list ( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules" )



