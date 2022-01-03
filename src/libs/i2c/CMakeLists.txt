
project( i2c )

set ( CMAKE_POSITION_INDEPENDENT_CODE ON )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  )

add_library(
  ${PROJECT_NAME}
  ad5593.cpp
  ad5593.hpp
  bmp280.cpp
  bmp280.hpp
  i2c.cpp
  i2c.hpp
  )

target_link_libraries(
  ${PROJECT_NAME}
  )

target_include_directories( ${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

install( TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT applications
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib COMPONENT libi2c
  )

install( FILES
  i2c.hpp
  ad5593.hpp
  bmp280.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/i2c COMPONENT libi2c
  )
