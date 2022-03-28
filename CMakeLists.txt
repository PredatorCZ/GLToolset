cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 17)
project(GLToolset VERSION 1.0)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -rdynamic")
endif()

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
set(PUGI_SHARED_LIBS
    ON
    CACHE BOOL "")
set(PC_SHARED_LIB
    ON
    CACHE BOOL "")
set(GLTF_SHARED_LIB
    ON
    CACHE BOOL "")

add_subdirectory(${TPD_PATH}/precore)
include(targetex)

add_subdirectory(${TPD_PATH}/precore/spike
                 ${CMAKE_CURRENT_BINARY_DIR}/spike)
add_subdirectory(${TPD_PATH}/precore/gltf)
set_target_properties(spike PROPERTIES OUTPUT_NAME gl_toolset)

add_subdirectory(gltex_view)

add_spike_subdir(gltex)
