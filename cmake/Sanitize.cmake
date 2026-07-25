function(radikant_debug_sanitize)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # clang-cl: real Clang sanitizers, cl-style driver
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined> $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    elseif(MSVC)
        # real cl.exe: ASan only, no UBSan
        add_compile_options($<$<CONFIG:Debug>:/fsanitize=address>)
        add_link_options($<$<CONFIG:Debug>:/fsanitize=address> $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
    else()
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined> $<$<CONFIG:Debug>:-fno-omit-frame-pointer> $<$<CONFIG:Debug>:-g>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    endif()
endfunction()