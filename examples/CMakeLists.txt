add_executable(brsp_example_solve host_solve.c)
target_link_libraries(brsp_example_solve PRIVATE BareSparse::baresparse)
set_target_properties(brsp_example_solve PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
brsp_set_project_warnings(brsp_example_solve)
brsp_enable_sanitizers(brsp_example_solve)

add_executable(brsp_example_sparse host_sparse.c)
target_link_libraries(brsp_example_sparse PRIVATE BareSparse::baresparse)
set_target_properties(brsp_example_sparse PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
brsp_set_project_warnings(brsp_example_sparse)
brsp_enable_sanitizers(brsp_example_sparse)
if(BRSP_BUILD_TESTS)
    add_test(NAME brsp.example COMMAND brsp_example_sparse)
endif()

add_executable(brsp_example_ordered host_ordered.c)
target_link_libraries(brsp_example_ordered PRIVATE BareSparse::baresparse)
set_target_properties(brsp_example_ordered PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
brsp_set_project_warnings(brsp_example_ordered)
brsp_enable_sanitizers(brsp_example_ordered)
if(BRSP_BUILD_TESTS)
    add_test(NAME brsp.example_ordered COMMAND brsp_example_ordered)
endif()
