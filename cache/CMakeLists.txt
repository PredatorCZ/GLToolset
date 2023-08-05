cmake_minimum_required(VERSION 3.3)

project(PrimeCache VERSION 1.0)

build_target(
  NAME
  make_cache
  TYPE
  ESMODULE
  SOURCES
  strip.cpp
  make_cache.cpp
  ../src/common/registry.cpp
  LINKS
  spike
  zstd
  AUTHOR
  "Lukas Cone"
  DESCR
  "Create Prime resource cache"
  START_YEAR
  2023)
