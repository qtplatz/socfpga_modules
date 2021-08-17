#
project ( adc )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${QTPLATZ_INCLUDE_DIRS}
  )

add_executable( ${PROJECT_NAME}
  main.cpp
  )

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

target_link_libraries( ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  )

#set( CPACK_DEBIAN_PACKAGE_DEPENDS "adc-fifo" )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT tools )
