cmake_minimum_required(VERSION 3.3)

project(GLTEX VERSION 1.0)

build_target(
  NAME
  make_gltex
  TYPE
  ESMODULE
  SOURCES
  make_gltex.cpp
  LINKS
  prime_converters
  AUTHOR
  "Lukas Cone"
  DESCR
  "OpenGL texture creator"
  START_YEAR
  2022)
