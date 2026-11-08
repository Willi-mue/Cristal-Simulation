cmake_minimum_required(VERSION 3.25)
project(CristalSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CRISTAL_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(CRISTAL_BUILD_TESTS "Build the tests" ON)
option(CRISTAL_GUI "Build the live viewer (--live); downloads raylib" OFF)

# Shared compiler settings for all targets.
add_library(cristal_settings INTERFACE)
if(MSVC)
    target_compile_options(cristal_settings INTERFACE /W4)
else()
    target_compile_options(cristal_settings INTERFACE -Wall -Wextra -Wpedantic)
endif()
# Bounds-checked std::vector etc. in Debug builds (libstdc++).
target_compile_definitions(cristal_settings INTERFACE $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
if(CRISTAL_SANITIZE AND NOT MSVC)
    target_compile_options(cristal_settings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(cristal_settings INTERFACE -fsanitize=address,undefined)
endif()

include(FetchContent)

# stb_image_write (single header, public domain) for writing PNG frames.
FetchContent_Declare(stb_image_write
    URL https://raw.githubusercontent.com/nothings/stb/4e9b368bfc071e97206ea5c6adba68dd0cbf8897/stb_image_write.h
    URL_HASH SHA256=CBD5F0AD7A9CF4468AFFB36354A1D2338034F2C12473CF1A8E32053CB6914A05
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(stb_image_write)

add_library(cristal_core STATIC
    src/simulation.cpp
    src/image_writer.cpp
    src/options.cpp
    src/scenario.cpp
)
target_include_directories(cristal_core PUBLIC src)
target_include_directories(cristal_core SYSTEM PRIVATE ${stb_image_write_SOURCE_DIR})
target_link_libraries(cristal_core PUBLIC cristal_settings)

add_executable(cristal src/main.cpp)
target_link_libraries(cristal PRIVATE cristal_core)

# Put the binaries in <build>/bin for every generator (no Release/Debug subfolder).
set_target_properties(cristal PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")

if(CRISTAL_GUI)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)  # no sound needed
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM
    )
    FetchContent_MakeAvailable(raylib)

    target_sources(cristal PRIVATE src/viewer.cpp)
    target_link_libraries(cristal PRIVATE raylib)
    target_compile_definitions(cristal PRIVATE CRISTAL_GUI)
endif()

if(CRISTAL_BUILD_TESTS)
    enable_testing()
    add_executable(cristal_tests tests/test_simulation.cpp)
    target_link_libraries(cristal_tests PRIVATE cristal_core)
    set_target_properties(cristal_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")
    add_test(NAME cristal_tests COMMAND cristal_tests)
endif()
