set(RADIKANT_SANITIZER "address" CACHE STRING
    "Debug-build sanitizer: 'address' (ASan+UBSan) or 'thread' (TSan+UBSan)")
set_property(CACHE RADIKANT_SANITIZER PROPERTY STRINGS address thread)

function(radikant_debug_sanitize)
    if(RADIKANT_SANITIZER STREQUAL "thread")
        set(_radikant_sanitize_flags -fsanitize=thread,undefined)  # thread
    else()
        set(_radikant_sanitize_flags -fsanitize=address,undefined) # address
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # clang-cl: real Clang sanitizers, cl-style driver
        add_compile_options($<$<CONFIG:Debug>:${_radikant_sanitize_flags}> $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
        add_link_options($<$<CONFIG:Debug>:${_radikant_sanitize_flags}>)
    elseif(MSVC)
        # real cl.exe: ASan only, no UBSan, no TSan
        add_compile_options($<$<CONFIG:Debug>:/fsanitize=address>)
        add_link_options($<$<CONFIG:Debug>:/fsanitize=address> $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
    else()
        add_compile_options($<$<CONFIG:Debug>:${_radikant_sanitize_flags}> $<$<CONFIG:Debug>:-fno-omit-frame-pointer> $<$<CONFIG:Debug>:-g>)
        add_link_options($<$<CONFIG:Debug>:${_radikant_sanitize_flags}>)
    endif()
endfunction()
