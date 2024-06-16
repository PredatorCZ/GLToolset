project(ispctex)

set(ISPCTEX_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

add_custom_target(kernel_ispc ispc -g --arch=x86-64 --target=avx2,sse4 --opt=fast-math --pic
-o ${CMAKE_CURRENT_BINARY_DIR}/kernel_ispc.o
-h ${CMAKE_CURRENT_BINARY_DIR}/kernel_ispc.h
${CMAKE_CURRENT_SOURCE_DIR}/kernel.ispc)
