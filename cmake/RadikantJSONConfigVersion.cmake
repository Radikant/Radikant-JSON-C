# RadikantJSONConfigVersion.cmake
set(PACKAGE_VERSION "0.1.0")

# Optional: fail if an incompatible version is requested
if(PACKAGE_VERSION VERSION_LESS "0.1.0")
    message(FATAL_ERROR "Installed RadikantJSON version is too old")
endif()