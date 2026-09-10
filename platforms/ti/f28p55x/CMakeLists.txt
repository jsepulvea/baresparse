if(NOT CMAKE_C_COMPILER_ID STREQUAL "TI" OR NOT BRSP_ARCH STREQUAL "c28x")
    message(FATAL_ERROR "F28P55x firmware requires the TI C28x toolchain")
endif()
set(BRSP_C2000WARE_ROOT "$ENV{C2000WARE_ROOT}" CACHE PATH "External C2000Ware SDK root")
set(_device "${BRSP_C2000WARE_ROOT}/device_support/f28p55x/common")
set(_driverlib "${BRSP_C2000WARE_ROOT}/driverlib/f28p55x/driverlib")
set(_linker "${_device}/cmd/28p55x_generic_flash_lnk.cmd")
foreach(_file "${_device}/source/device.c" "${_device}/source/f28p55x_codestartbranch.asm"
    "${_linker}" "${_driverlib}/ccs/Release/driverlib.lib")
    if(NOT EXISTS "${_file}")
        message(FATAL_ERROR "Missing C2000Ware file: ${_file}; set C2000WARE_ROOT")
    endif()
endforeach()
# Standard TI linker file supplies flash entry at 0x080000, RAM copy symbols,
# .TI.ramfunc in RAMLS0, stack in RAMM1 and the explicit ramgs0 output section.
# Startup disables the watchdog then branches to the TI C runtime _c_int00.
enable_language(ASM)
add_executable(brsp_f28p55x_benchmark main.c
    "${PROJECT_SOURCE_DIR}/benchmarks/brsp_benchmark.c"
    "${_device}/source/device.c"
    "${_device}/source/f28p55x_codestartbranch.asm")
set_target_properties(brsp_f28p55x_benchmark PROPERTIES
    SUFFIX ".out" C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
target_include_directories(brsp_f28p55x_benchmark PRIVATE "${_device}/include" "${_driverlib}")
target_compile_definitions(brsp_f28p55x_benchmark PRIVATE _FLASH _LAUNCHXL_F28P55X CPU1)
target_link_libraries(brsp_f28p55x_benchmark PRIVATE BareSparse::baresparse
    "${_driverlib}/ccs/Release/driverlib.lib" "${_linker}")
target_link_options(brsp_f28p55x_benchmark PRIVATE
    --entry_point=code_start --stack_size=0x400 --heap_size=0 --rom_model
    "--xml_link_info=${CMAKE_CURRENT_BINARY_DIR}/brsp_f28p55x_benchmark_link.xml")
set_property(TARGET brsp_f28p55x_benchmark APPEND PROPERTY LINK_DEPENDS "${_linker}")
include(BrspBenchmark)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${BRSP_C2000WARE_ROOT}"
    OUTPUT_VARIABLE BRSP_BENCH_SDK_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT BRSP_BENCH_SDK_REVISION)
    set(BRSP_BENCH_SDK_REVISION "external SDK; record installed version separately")
endif()
brsp_benchmark_build_info(brsp_f28p55x_benchmark)
