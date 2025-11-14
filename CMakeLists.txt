cmake_minimum_required(VERSION 3.16)
project(Radikant-Json VERSION 0.1.0 LANGUAGES C)

# ---------------------------------------------------------------
# General build settings
# ---------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
# ---------------------------------------------------------------
# Library target (Radikant-Json)
# ---------------------------------------------------------------
# This command creates a shared library named Radikant-Json
# from the source files listed.
add_library(Radikant-Json SHARED
    SRC/rjson.c
)

# These properties define the output name and versioning for the library file.
set_target_properties(Radikant-Json PROPERTIES
    OUTPUT_NAME "Radikant-Json"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# This specifies the public include directories for the library.
# Any target that links against Radikant-Json will automatically have
# the 'include' directory added to its include path.
target_include_directories(Radikant-Json PUBLIC
    # For projects building this directly (e.g., tests in this project)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    
    # For projects that install and use this library later
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------
# This creates an executable named 'test_json' from the test source file.
add_executable(test_json test/test_json.c)

# This links the test executable against the Radikant-Json library.
# It's crucial for allowing test_json to use the functions from the library
# and to find the header files like "rjson.h".
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
    # Note: The INCLUDES DESTINATION here is for the target properties,
    # it does not set the variable.
    INCLUDES DESTINATION ${INCLUDES_DESTINATION}
)

# Install the headers
# (Updated to use the variable for consistency)
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

# Generate the main config file (Radikant-JsonConfig.cmake)
configure_package_config_file(
    "Radikant-JsonConfig.cmake.in" # The template
    "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/Radikant-Json"
    # This line will now work because the INCLUDES_DESTINATION variable exists
    PATH_VARS INCLUDES_DESTINATION
)

# Install the two generated config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/Radikant-JsonConfigVersion.cmake"
    DESTINATION "lib/cmake/Radikant-Json"
)
