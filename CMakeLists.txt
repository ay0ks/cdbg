cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cdbg C)

add_library(cdbg OBJECT)
set_target_properties(
  cdbg
  PROPERTIES
  C_STANDARD 23
  C_STANDARD_REQUIRED YES
  POSITION_INDEPENDENT_CODE 1
)
target_compile_definitions(cdbg PUBLIC UNICODE)
target_compile_options(cdbg PUBLIC
  "-UNDEBUG"
  "-O2"
  "-Wall"
  "-Wextra"
  "-Wshadow"
)
target_sources(cdbg PRIVATE ./src/cdbg.c)
target_include_directories(cdbg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(cdbg_shared SHARED $<TARGET_OBJECTS:cdbg>)
add_library(cdbg_static STATIC $<TARGET_OBJECTS:cdbg>)
set_target_properties(cdbg_shared cdbg_static PROPERTIES OUTPUT_NAME "$<TARGET_PROPERTY:cdbg,NAME>")
