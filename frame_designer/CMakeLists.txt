
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(GLEW REQUIRED)

add_executable(frame_designer frame_designer.cpp)

target_link_libraries(frame_designer PUBLIC glfw OpenGL::GL GLEW::GLEW ImGui prime)
