set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR TMS320C28x)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(_brsp_ti_hints)
if(DEFINED ENV{TI_C2000_CGT_ROOT})
    list(APPEND _brsp_ti_hints "$ENV{TI_C2000_CGT_ROOT}")
endif()

find_program(CMAKE_C_COMPILER
    NAMES cl2000
    HINTS ${_brsp_ti_hints}
    PATH_SUFFIXES bin
    REQUIRED
)
find_program(CMAKE_AR
    NAMES ar2000
    HINTS ${_brsp_ti_hints}
    PATH_SUFFIXES bin
    REQUIRED
)

# Resolve runtime headers/libraries from the selected compiler, not a user path.
get_filename_component(_brsp_ti_bin "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(_brsp_ti_root "${_brsp_ti_bin}" DIRECTORY)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_ASM_FLAGS_INIT "--abi=eabi --silicon_version=28 --large_memory_model --float_support=fpu32")
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${_brsp_ti_root}/include")
set(CMAKE_C_FLAGS_INIT "--abi=eabi --silicon_version=28 --large_memory_model --float_support=fpu32 --fp_mode=strict --gen_func_subsections=on")
set(CMAKE_C_FLAGS_RELEASE_INIT "--opt_level=2")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--search_path=\"${_brsp_ti_root}/lib\"")
set(CMAKE_C_STANDARD_LIBRARIES "--library=libc.a")
