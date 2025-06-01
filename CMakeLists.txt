cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(clog C)

add_library(clog OBJECT)
set_target_properties(
  clog
  PROPERTIES
  C_STANDARD 23
  C_STANDARD_REQUIRED YES
  POSITION_INDEPENDENT_CODE 1
)
target_compile_definitions(clog PUBLIC UNICODE)
target_sources(clog PRIVATE ./src/clog.c)
target_include_directories(clog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(clog_shared SHARED $<TARGET_OBJECTS:clog>)
add_library(clog_static STATIC $<TARGET_OBJECTS:clog>)
set_target_properties(
  clog_shared 
  clog_static 
  PROPERTIES 
    OUTPUT_NAME "$<TARGET_PROPERTY:clog,NAME>"
    PUBLIC_HEADER ./include/clog.h
)

install(
  TARGETS clog_shared clog_static
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
)
