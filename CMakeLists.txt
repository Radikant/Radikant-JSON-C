cmake_minimum_required(VERSION 3.20)
project(Radikant-Json-C VERSION 0.1.0 LANGUAGES C)

# ---- Include Radikant CMake functions ----
include("cmake/Radikant.cmake")

# ---- Detect Apple Silicon ----
detect_apple_silicon()

# ---- Submodules ----
# add_subdirectory_once(lib/Radikant-Socket-C    Radikant-Socket-C)
# add_subdirectory_once(lib/Radikant-Log-C       Radikant-Log-C)
# add_subdirectory_once(lib/Radikant-Probe-C     Radikant-Probe-C)

# ---- Global settings ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Sanitizers for Debug Builds ----
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options("-fsanitize=address,undefined" "-fno-omit-frame-pointer" "-g")
    add_link_options("-fsanitize=address,undefined")
endif()

# ---- Profile Guided Optimization (PGO) ----
option(RADIKANT_JSON_PGO_GENERATE "Build with PGO instrumentation" OFF)
option(RADIKANT_JSON_PGO_USE "Build using PGO profile data" OFF)

if(RADIKANT_JSON_PGO_GENERATE)
    add_compile_options("-fprofile-generate")
    add_link_options("-fprofile-generate")
elseif(RADIKANT_JSON_PGO_USE)
    add_compile_options("-fprofile-use=${CMAKE_SOURCE_DIR}/default.profdata")
    add_link_options("-fprofile-use=${CMAKE_SOURCE_DIR}/default.profdata")
endif()

# ---- Build shared library ----
add_library(Radikant-Json-C SHARED
    SRC/rjson.c
    SRC/error.c
    SRC/decode.c
    SRC/stream.c
    SRC/zone.c
    SRC/object.c
    SRC/encode.c
)

# ---- Includess ----
target_include_directories(Radikant-Json-C
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SRC>
        $<INSTALL_INTERFACE:include>
)

# ---- Link dependencies ----
target_link_libraries(Radikant-Json-C
    PUBLIC

    PRIVATE

)

# ---- Output settings ----
set_target_properties(Radikant-Json-C PROPERTIES
    OUTPUT_NAME "radikant_json_c"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)


if(NOT PROJECT_IS_TOP_LEVEL)
    return() 
endif()

add_subdirectory_once(lib/Radikant-Probe-C Radikant-Probe-C)
add_subdirectory_once(lib/yyjson yyjson)


enable_testing()
add_subdirectory(test)
add_subdirectory(example)