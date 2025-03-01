cmake_minimum_required(VERSION 3.3)

project(CompileNutty VERSION 1.0)

build_target(
  NAME
  compile_nutty
  TYPE
  ESMODULE
  SOURCES
  compile_nutty.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/playground.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/debug.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/reflect/script.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/reflect/reflect.cpp
  INCLUDES
  ${CMAKE_SOURCE_DIR}/include/script/detail/sqbase
  ${CMAKE_SOURCE_DIR}/src/script/sqbase
  LINKS
  spike-interface
  prime_script_base
  AUTHOR
  "Lukas Cone"
  DESCR
  "Compile Nutty shader"
  START_YEAR
  2025)
