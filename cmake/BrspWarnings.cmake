function(brsp_set_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(BRSP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        return()
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wstrict-prototypes
            -Wmissing-prototypes
        )
        if(BRSP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        return()
    endif()

    if(CMAKE_C_COMPILER_ID STREQUAL "TI")
        target_compile_options(${target} PRIVATE --display_error_number)
        if(BRSP_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE --emit_warnings_as_errors)
        endif()
    endif()
endfunction()
