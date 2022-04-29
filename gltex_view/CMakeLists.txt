
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(GLEW REQUIRED)

add_executable(gltex_view gltex_view.cpp ${TPD_PATH}/mikktspace/mikktspace.c)

target_link_libraries(gltex_view PUBLIC glfw OpenGL::GL GLEW::GLEW precore ImGui)

file(COPY res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(gltex_view PRIVATE ${TPD_PATH}/mikktspace)

add_subdirectory(shaders)