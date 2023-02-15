cmake_minimum_required(VERSION 3.3)

project(GLMOD VERSION 1.0)

build_target(
  NAME
  make_glmod
  TYPE
  ESMODULE
  SOURCES
  make_glmod.cpp
  ${TPD_PATH}/mikktspace/mikktspace.c
  LINKS
  precore
  INCLUDES
  ${TPD_PATH}/mikktspace
  ../src/shaders
  AUTHOR
  "Lukas Cone"
  DESCR
  "OpenGL model creator"
  START_YEAR
  2022)
