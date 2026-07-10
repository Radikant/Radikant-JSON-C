function(add_subdirectory_once path target)
    # Check if the target (e.g., 'radikant-json') already exists
    if(NOT TARGET ${target})
        message(STATUS "Including ${target} from ${path}")
        add_subdirectory(${path})
    else()
        message(STATUS "Target ${target} already exists. Skipping.")
    endif()
endfunction()

function(detect_apple_silicon)
    if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        message(STATUS "Apple Silicon detected. Enabling max optimizations.")
        
        # 1. Force Release Mode
        if(NOT CMAKE_BUILD_TYPE)
            set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
        endif()
        
        # 2. CPU Native + LTO + Omit Frame Pointer
        if(NOT CMAKE_C_FLAGS MATCHES "-mcpu=native")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -mcpu=native -flto -fomit-frame-pointer" CACHE STRING "Flags used by the C compiler." FORCE)
        endif()
        
        if(NOT CMAKE_CXX_FLAGS MATCHES "-mcpu=native")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mcpu=native -flto -fomit-frame-pointer" CACHE STRING "Flags used by the CXX compiler." FORCE)
        endif()
        
        # 3. Tell CMake to use Interprocedural Optimization natively
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "Enable LTO" FORCE)
    endif()
endfunction()