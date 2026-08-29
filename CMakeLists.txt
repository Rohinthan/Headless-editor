cmake_minimum_required(VERSION 3.20)
project(AntigravityVideoEditor LANGUAGES CXX)

# Target C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC and Resource Compiler
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Configure search paths for user environment
list(PREPEND CMAKE_PREFIX_PATH 
    "$ENV{HOME}/.local" 
    "$ENV{HOME}/.local/usr" 
    "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake"
)
set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")

if(EXISTS "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6Quick")
    set(Qt6Quick_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6Quick")
    set(Qt6Qml_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6Qml")
    set(Qt6QmlTools_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6QmlTools")
    set(Qt6QuickTools_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6QuickTools")
    set(Qt6QmlIntegration_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6QmlIntegration")
    set(Qt6QmlModels_DIR "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/cmake/Qt6QmlModels")
endif()

# Strict compiler flags & optimization
add_compile_options(-Wall -Wextra -Wpedantic -O3)

# Add user library directories to linker search path
link_directories(
    "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu"
    "/usr/lib/x86_64-linux-gnu"
)

# Locate Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick OpenGL)

# Locate PkgConfig, FFmpeg, and VA-API
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale libswresample)
pkg_check_modules(LIBVA IMPORTED_TARGET libva libva-drm)

# Locate OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $ENV{HOME}/.local/usr/include
)

# ----------------------------------------------------
# Target 1: Headless CLI Diagnostic & Benchmark Tool
# ----------------------------------------------------
add_executable(cli_test_decoder
    tests/cli_test_decoder.cpp
    src/core/DecoderEngine.cpp
)

target_link_libraries(cli_test_decoder PRIVATE
    PkgConfig::FFMPEG
    $<$<BOOL:${LIBVA_FOUND}>:PkgConfig::LIBVA>
    Threads::Threads
)

target_compile_definitions(cli_test_decoder PRIVATE
    SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# ----------------------------------------------------
# Target 2: Video Editor GUI Application
# ----------------------------------------------------
add_executable(video_editor
    src/main.cpp
    src/core/DecoderEngine.cpp
    src/core/GraphEngine.cpp
    src/ui/ViewportItem.cpp
)

target_link_libraries(video_editor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::OpenGL
    PkgConfig::FFMPEG
    $<$<BOOL:${LIBVA_FOUND}>:PkgConfig::LIBVA>
    OpenGL::GL
    Threads::Threads
)

target_compile_definitions(video_editor PRIVATE
    SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
