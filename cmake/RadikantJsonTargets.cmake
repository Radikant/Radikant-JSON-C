# RadikantJsonTargets.cmake
# Define an imported target for Radikant-Json library

if(NOT TARGET RadikantJson::Radikant-Json)
    add_library(RadikantJson::Radikant-Json SHARED IMPORTED)

    # Path to the installed library
    set_target_properties(RadikantJson::Radikant-Json PROPERTIES
        IMPORTED_LOCATION "/opt/radikant/Radikant-JSON-C/current/lib/libRadikant-Json.so"
        INTERFACE_INCLUDE_DIRECTORIES "/opt/radikant/Radikant-JSON-C/current/include"
    )
endif()