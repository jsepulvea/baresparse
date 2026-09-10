add_executable(brsp_tests test_main.c)
target_link_libraries(brsp_tests PRIVATE BareSparse::baresparse)
set_target_properties(brsp_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
brsp_set_project_warnings(brsp_tests)
brsp_enable_sanitizers(brsp_tests)

add_test(NAME brsp.unit COMMAND brsp_tests)

add_executable(brsp_sparse_tests test_sparse.c)
target_link_libraries(brsp_sparse_tests PRIVATE BareSparse::baresparse)
set_target_properties(brsp_sparse_tests PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
brsp_set_project_warnings(brsp_sparse_tests)
brsp_enable_sanitizers(brsp_sparse_tests)
add_test(NAME brsp.sparse COMMAND brsp_sparse_tests)
