cmake_minimum_required(VERSION 3.16)
project(Radikant-Json VERSION 0.1.0 LANGUAGES C)

# ---------------------------------------------------------------
# General build settings
# ---------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# This line was correctly removed to ensure relocatability
# set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# ---------------------------------------------------------------
# Library target (Radikant-Json)
# ---------------------------------------------------------------
add_library(Radikant-Json SHARED
    SRC/rjson.c
)

set_target_properties(Radikant-Json PROPERTIES
    OUTPUT_NAME "Radikant-Json"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_include_directories(Radikant-Json PUBLIC
    # For projects building this directly (e.g., tests in this project)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    
    # For projects that install and use this library later
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------
add_executable(test_json test/test_json.c)
target_link_libraries(test_json PRIVATE Radikant-Json)

# ---------------------------------------------------------------
# Installation
# ---------------------------------------------------------------

set(INCLUDES_DESTINATION include)

# Install the library target
install(TARGETS Radikant-Json
    EXPORT RadikantJsonTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ${INCLUDES_DESTINATION}
)

# Install the headers
install(DIRECTORY include/ DESTINATION ${INCLUDES_DESTINATION})

# Install the exported targets file (generates RadikantJsonTargets.cmake)
install(EXPORT RadikantJsonTargets
    FILE RadikantJsonTargets.cmake
    NAMESPACE RadikantJson::
    DESTINATION lib/cmake/Radikant-Json
)

# --- This is the configuration block ---

# Bring in the helper tools
include(CMakePackageConfigHelpers)

# Generate the version file (Radikant-JsonConfigVersion.cmake)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# --- THIS BLOCK IS THE REVISION ---
# Generate the main config file (Radikant-JsonConfig.cmake)
# Note the removal of "PATH_VARS".
# This assumes your .cmake.in file uses @PACKAGE_INIT@.
configure_package_config_file(
    "Radikant-JsonConfig.cmake.in" # The template
    "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/Radikant-Json"
)
# --- END OF REVISION ---

# Install the two generated config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfigVersion.cmake"
    DESTINATION "lib/cmake/Radikant-Json"
)
