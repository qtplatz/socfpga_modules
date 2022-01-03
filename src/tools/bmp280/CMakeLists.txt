#
cmake_minimum_required (VERSION 2.8.9)

project ( bmp280 )

set ( CMAKE_ECLIPSE_VERSION 4.5 )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src/libs
  )

add_executable( ${PROJECT_NAME}
  main.cpp
  )

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

target_link_libraries( ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  i2c
  )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT tools )
