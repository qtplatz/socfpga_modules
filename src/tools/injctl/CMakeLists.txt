#
project ( injctl )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" )
set ( CMAKE_CXX_STANDARD 17 )

include_directories(
  ${QTPLATZ_SOURCE_DIR}/src/libs
  ${QTPLATZ_SOURCE_DIR}/src/tools  # <-- eventbroker
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  /opt/qtplatz/include
  )

add_executable( ${PROJECT_NAME}
  main.cpp
  date_time.cpp
  date_time.hpp
  )

target_link_libraries( ${PROJECT_NAME} LINK_PUBLIC
  eventbroker
  ${Boost_LIBRARIES}
  )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT tools )
