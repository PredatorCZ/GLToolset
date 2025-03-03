
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(GLEW REQUIRED)

add_executable(gltex_view gltex_view.cpp)

target_link_libraries(gltex_view PUBLIC glfw OpenGL::GL GLEW::GLEW ImGui prime spike prime_reflect prime_script prime_script_base prime_converters)

file(COPY res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
