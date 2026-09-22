if(NOT PICO_BOARD STREQUAL "pico")
    message(FATAL_ERROR "Pico test firmware requires PICO_BOARD=pico")
endif()

if(NOT BRSP_ARCH STREQUAL "portable")
    message(FATAL_ERROR "Pico RP2040 test firmware requires BRSP_ARCH=portable")
endif()

set(_brsp_pico_sources
    main.c
    "${PROJECT_SOURCE_DIR}/tests/basic_suite.c"
)
set(_brsp_pico_warnings
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes)
if(BRSP_WARNINGS_AS_ERRORS)
    list(APPEND _brsp_pico_warnings -Werror)
endif()
set_source_files_properties(${_brsp_pico_sources} PROPERTIES
    COMPILE_OPTIONS "${_brsp_pico_warnings}")

add_executable(brsp_pico_tests ${_brsp_pico_sources})
target_include_directories(brsp_pico_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(brsp_pico_tests PRIVATE BareSparse::baresparse pico_stdlib)
set_target_properties(brsp_pico_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
pico_enable_stdio_usb(brsp_pico_tests 1)
pico_enable_stdio_uart(brsp_pico_tests 0)
pico_add_extra_outputs(brsp_pico_tests)
