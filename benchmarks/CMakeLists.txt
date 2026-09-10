add_executable(brsp_benchmark_dense_lu host_dense_lu.c)
target_link_libraries(brsp_benchmark_dense_lu PRIVATE BareSparse::baresparse)
set_target_properties(brsp_benchmark_dense_lu PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
brsp_set_project_warnings(brsp_benchmark_dense_lu)
brsp_enable_sanitizers(brsp_benchmark_dense_lu)

include(BrspBenchmark)
add_executable(brsp_benchmark_sparse host_sparse.c brsp_benchmark.c)
target_link_libraries(brsp_benchmark_sparse PRIVATE BareSparse::baresparse m)
set_target_properties(brsp_benchmark_sparse PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
brsp_set_project_warnings(brsp_benchmark_sparse)
brsp_enable_sanitizers(brsp_benchmark_sparse)
brsp_benchmark_build_info(brsp_benchmark_sparse)
if(BRSP_BUILD_TESTS)
    add_test(NAME brsp.benchmark COMMAND brsp_benchmark_sparse)
endif()
