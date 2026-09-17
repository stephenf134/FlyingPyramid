cmake_minimum_required(VERSION 4.3)
project(MyAnimation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MyAnimation WIN32 src/main.cpp
        include/MyAnimation/ObjectManager.hpp
        src/ObjectManager.cpp)
target_include_directories(MyAnimation PRIVATE "${PROJECT_SOURCE_DIR}\\include")

if (MSVC)
    # Match the static runtime used by the prebuilt BGFX dependencies.
    set_property(TARGET MyAnimation PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(MyAnimation PRIVATE /Zc:__cplusplus)
    target_compile_options(MyAnimation PRIVATE /Zc:preprocessor)
    target_compile_definitions(MyAnimation PRIVATE
        BX_CONFIG_DEBUG=1
            NOMINMAX
    )
endif()

# BGFX directory
set(BGFX_DIR    "C:/Users/safre/msvc-pkg-dev/bgfx")
set(BX_DIR      "C:/Users/safre/msvc-pkg-dev/bx")
set(BIMG_DIR    "C:/Users/safre/msvc-pkg-dev/bimg")

# includes/headers
target_include_directories(MyAnimation PRIVATE
        ${BGFX_DIR}/include
        ${BX_DIR}/include
        ${BIMG_DIR}/include
)

# lib dirs
set(BGFX_LIB_DIR "${BGFX_DIR}/.build//win64_vs2026/bin")
target_link_directories(MyAnimation PRIVATE
    ${BGFX_LIB_DIR}
)

# libs
target_link_libraries(MyAnimation PRIVATE
        bgfxDebug
        bxDebug
        bimgDebug

        # Windows
        user32
        gdi32
        shell32

        # DirectX
        d3d11
        dxgi
        dxguid
)
