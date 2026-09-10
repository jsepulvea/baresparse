set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR dspic33a)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(BRSP_DSPIC_DEVICE "33AK512MPS506" CACHE STRING
    "XC-DSC -mcpu device name")
set(BRSP_DSPIC_DFP "$ENV{XC_DSC_DFP}" CACHE PATH
    "XC-DSC device-family pack compiler directory (the pack's xc16 directory)")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES BRSP_DSPIC_DEVICE BRSP_DSPIC_DFP)

set(_brsp_dspic_hints)
if(DEFINED ENV{XC_DSC_ROOT})
    list(APPEND _brsp_dspic_hints "$ENV{XC_DSC_ROOT}")
endif()

find_program(CMAKE_C_COMPILER
    NAMES xc-dsc-gcc
    HINTS ${_brsp_dspic_hints}
    PATH_SUFFIXES bin
    REQUIRED
)
find_program(CMAKE_AR
    NAMES xc-dsc-ar
    HINTS ${_brsp_dspic_hints}
    PATH_SUFFIXES bin
    REQUIRED
)
find_program(CMAKE_RANLIB
    NAMES xc-dsc-ranlib
    HINTS ${_brsp_dspic_hints}
    PATH_SUFFIXES bin
    REQUIRED
)

set(CMAKE_C_FLAGS_INIT "-mcpu=${BRSP_DSPIC_DEVICE} -ffunction-sections -fdata-sections")
if(BRSP_DSPIC_DFP)
    if(NOT EXISTS "${BRSP_DSPIC_DFP}/bin/device_files/${BRSP_DSPIC_DEVICE}.info")
        message(FATAL_ERROR "BRSP_DSPIC_DFP must contain bin/device_files/${BRSP_DSPIC_DEVICE}.info")
    endif()
    string(APPEND CMAKE_C_FLAGS_INIT " -mdfp=\"${BRSP_DSPIC_DFP}\"")
    # The XC-DSC archive tools also need the pack to identify dsPIC33A objects.
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -mdfp=\"${BRSP_DSPIC_DFP}\" qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> -mdfp=\"${BRSP_DSPIC_DFP}\" q <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -mdfp=\"${BRSP_DSPIC_DFP}\" <TARGET>")
endif()
