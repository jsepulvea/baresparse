function(brsp_benchmark_build_info target)
    execute_process(COMMAND git describe --always --dirty --abbrev=40
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE BRSP_BENCH_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT BRSP_BENCH_REVISION)
        set(BRSP_BENCH_REVISION "unversioned")
    endif()
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _config)
    set(BRSP_BENCH_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${_config}}")
    # Escape C string literals when toolchain flags contain quoted paths.
    string(REPLACE "\\" "\\\\" BRSP_BENCH_FLAGS "${BRSP_BENCH_FLAGS}")
    string(REPLACE "\"" "\\\"" BRSP_BENCH_FLAGS "${BRSP_BENCH_FLAGS}")
    configure_file("${PROJECT_SOURCE_DIR}/benchmarks/brsp_benchmark_build.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/brsp_benchmark_build.h" @ONLY)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/benchmarks")
endfunction()
