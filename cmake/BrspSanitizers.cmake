function(brsp_enable_sanitizers target)
    if(NOT BRSP_ENABLE_SANITIZERS)
        return()
    endif()

    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "BRSP_ENABLE_SANITIZERS is only supported for native builds")
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -fno-omit-frame-pointer
            -fsanitize=address,undefined
        )
        target_link_options(${target} PRIVATE
            -fno-omit-frame-pointer
            -fsanitize=address,undefined
        )
    else()
        message(FATAL_ERROR
            "Sanitizers are not configured for compiler '${CMAKE_C_COMPILER_ID}'")
    endif()
endfunction()
