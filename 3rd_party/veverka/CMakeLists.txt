cmake_minimum_required(VERSION 3.12)
project(squirrel
  VERSION 3.2
  DESCRIPTION "The Squirrel programming language"
  HOMEPAGE_URL "http://squirrel-lang.org/"
  LANGUAGES C CXX
  )

option(DISABLE_STATIC "Avoid building/installing static libraries.")
option(LONG_OUTPUT_NAMES "Use longer names for binaries and libraries: squirrel3 (not sq).")

if (NOT CMAKE_BUILD_TYPE)
  #set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_CXX_STANDARD 20)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions>"
    -fno-strict-aliasing
    -Wall
    -Wextra
    -pedantic
    -Wcast-qual
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:RelWithDebInfo>:-O3;-g>"
    "$<$<CONFIG:MinSizeRel>:-Os>"
    "$<$<CONFIG:Debug>:-pg;-pie;-g3;-Og>"
    )
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(squirrel)
add_subdirectory(sqstdlib)
#add_subdirectory(sqigui)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(tgts)
  #if(NOT DISABLE_DYNAMIC)
  #  list(APPEND tgts squirrel sqstdlib sqigui)
  #endif()
  if(NOT DISABLE_STATIC)
    list(APPEND tgts squirrel_static sqstdlib_static)# sqigui_static)
  endif()
  foreach(t ${tgts})
    target_compile_definitions(${t} PUBLIC -D_SQ64)
  endforeach()
endif()

#if(NOT DISABLE_DYNAMIC)
#  set_target_properties(squirrel sqstdlib sqigui PROPERTIES SOVERSION 0 VERSION 0.0.0)
#endif()
