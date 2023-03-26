project(ispctex)

set(ISPCTEX_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

add_custom_target(kernel_ispc ispc -g --arch=x86-64 --target=avx,sse2 --opt=fast-math --pic -o ${ISPCTEX_DIR}/kernel_ispc.o -h ${ISPCTEX_DIR}/kernel_ispc.h ${TPD_PATH}/ispctex/kernel.ispc)
