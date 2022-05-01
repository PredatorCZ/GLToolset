project(ImGui)

add_library(
  ImGui OBJECT
  imgui_impl_glfw.cpp
  imgui_impl_opengl3.cpp
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  ImGuiFileDialog.cpp)

target_link_libraries(ImGui INTERFACE dl)
target_include_directories(ImGui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
