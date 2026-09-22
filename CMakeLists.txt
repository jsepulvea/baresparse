cmake_minimum_required(VERSION 3.24)

set(BRSP_BUILD_PICO_TEST_FIRMWARE OFF CACHE BOOL
    "Build Raspberry Pi Pico RP2040 test firmware")

set(_brsp_project_languages C)
if(BRSP_BUILD_PICO_TEST_FIRMWARE)
    set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/pico-sdk" CACHE PATH
        "Path to the Raspberry Pi Pico SDK")
    set(PICO_TOOLCHAIN_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/toolchain/expanded/Payload" CACHE PATH
        "Path to the Arm GNU toolchain used by the Pico SDK")
    set(picotool_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/picotool" CACHE PATH
        "Path to the host picotool CMake package")
    set(_brsp_pico_sdk_import "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    if(NOT EXISTS "${_brsp_pico_sdk_import}")
        message(FATAL_ERROR
            "Missing Pico SDK import file: ${_brsp_pico_sdk_import}. "
            "Populate the ignored local vendor directory or set PICO_SDK_PATH.")
    endif()
    include("${_brsp_pico_sdk_import}")
    list(APPEND _brsp_project_languages CXX ASM)
endif()

project(
    BareSparse
    VERSION 0.1.0
    DESCRIPTION "Allocation-free sparse direct linear algebra for bare-metal real-time systems"
    LANGUAGES ${_brsp_project_languages}
)

if(BRSP_BUILD_PICO_TEST_FIRMWARE)
    pico_sdk_init()
endif()

include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BrspSanitizers)
include(BrspWarnings)

set(BRSP_ARCH "portable" CACHE STRING "BareSparse architecture backend")
set_property(CACHE BRSP_ARCH PROPERTY STRINGS portable c28x cortex-m4 dspic33a)

set(_brsp_default_host_components OFF)
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CROSSCOMPILING)
    set(_brsp_default_host_components ON)
endif()

option(BRSP_BUILD_TESTS "Build native unit tests" ${_brsp_default_host_components})
option(BRSP_BUILD_EXAMPLES "Build native examples" ${_brsp_default_host_components})
option(BRSP_BUILD_BENCHMARKS "Build native benchmark executables" ${_brsp_default_host_components})
option(BRSP_BUILD_COMPILE_CHECKS "Build a compile-only API smoke test" ON)
option(BRSP_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
option(BRSP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BRSP_BUILD_F28P55X_FIRMWARE "Build the TI flash benchmark firmware" OFF)
option(BRSP_INSTALL "Generate installation rules" ${PROJECT_IS_TOP_LEVEL})

if(BRSP_INSTALL)
    include(GNUInstallDirs)
else()
    set(CMAKE_INSTALL_INCLUDEDIR include)
endif()

set(_brsp_valid_arches portable c28x cortex-m4 dspic33a)
if(NOT BRSP_ARCH IN_LIST _brsp_valid_arches)
    message(FATAL_ERROR
        "Unsupported BRSP_ARCH='${BRSP_ARCH}'. Expected one of: ${_brsp_valid_arches}")
endif()

set(BRSP_ARCH_PORTABLE 0)
set(BRSP_ARCH_C28X 0)
set(BRSP_ARCH_CORTEX_M4 0)
set(BRSP_ARCH_DSPIC33A 0)

if(BRSP_ARCH STREQUAL "portable")
    set(BRSP_ARCH_PORTABLE 1)
    set(_brsp_backend_source src/arch/portable/brsp_backend.c)
elseif(BRSP_ARCH STREQUAL "c28x")
    set(BRSP_ARCH_C28X 1)
    set(_brsp_backend_source src/arch/c28x/brsp_backend.c)
elseif(BRSP_ARCH STREQUAL "cortex-m4")
    set(BRSP_ARCH_CORTEX_M4 1)
    set(_brsp_backend_source src/arch/cortex_m4/brsp_backend.c)
elseif(BRSP_ARCH STREQUAL "dspic33a")
    set(BRSP_ARCH_DSPIC33A 1)
    set(_brsp_backend_source src/arch/dspic33a/brsp_backend.c)
endif()

set(_brsp_generated_include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    include/baresparse/brsp_config.h.in
    "${_brsp_generated_include_dir}/baresparse/brsp_config.h"
    @ONLY
)

add_library(baresparse STATIC
    src/brsp_csc.c
    src/brsp_dense_lu.c
    src/brsp_sparse_lu.c
    src/brsp_status.c
    src/brsp_version.c
    "${_brsp_backend_source}"
)
add_library(BareSparse::baresparse ALIAS baresparse)

set_target_properties(baresparse PROPERTIES
    OUTPUT_NAME baresparse
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

target_include_directories(baresparse
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${_brsp_generated_include_dir}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(baresparse PRIVATE BRSP_BUILDING_LIBRARY=1)
brsp_set_project_warnings(baresparse)
brsp_enable_sanitizers(baresparse)

if(BRSP_BUILD_COMPILE_CHECKS)
    add_library(brsp_compile_smoke OBJECT tests/compile_smoke.c)
    target_link_libraries(brsp_compile_smoke PRIVATE BareSparse::baresparse)
    set_target_properties(brsp_compile_smoke PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    brsp_set_project_warnings(brsp_compile_smoke)
endif()

if(BRSP_BUILD_TESTS)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "BRSP_BUILD_TESTS requires a native build")
    endif()
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BRSP_BUILD_EXAMPLES)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "BRSP_BUILD_EXAMPLES requires a native build")
    endif()
    add_subdirectory(examples)
endif()

if(BRSP_BUILD_BENCHMARKS)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "BRSP_BUILD_BENCHMARKS requires a native build")
    endif()
    add_subdirectory(benchmarks)
endif()

if(BRSP_BUILD_F28P55X_FIRMWARE)
    add_subdirectory(platforms/ti/f28p55x)
endif()

if(BRSP_BUILD_PICO_TEST_FIRMWARE)
    add_subdirectory(platforms/raspberrypi/pico)
endif()

if(BRSP_INSTALL)
    install(
        TARGETS baresparse
        EXPORT BareSparseTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(
        DIRECTORY include/baresparse
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h"
    )

    install(
        FILES "${_brsp_generated_include_dir}/baresparse/brsp_config.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/baresparse"
    )

    configure_package_config_file(
        cmake/BareSparseConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/BareSparseConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BareSparse"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/BareSparseConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install(
        EXPORT BareSparseTargets
        FILE BareSparseTargets.cmake
        NAMESPACE BareSparse::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BareSparse"
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/BareSparseConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/BareSparseConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BareSparse"
    )
endif()

message(STATUS "BareSparse architecture: ${BRSP_ARCH}")
message(STATUS "BareSparse cross-compiling: ${CMAKE_CROSSCOMPILING}")
