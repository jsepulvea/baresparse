set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(_brsp_arm_hints)
if(DEFINED ENV{ARM_NONE_EABI_ROOT})
    list(APPEND _brsp_arm_hints "$ENV{ARM_NONE_EABI_ROOT}")
endif()

find_program(CMAKE_C_COMPILER
    NAMES arm-none-eabi-gcc
    HINTS ${_brsp_arm_hints}
    PATH_SUFFIXES bin
    REQUIRED
)
find_program(CMAKE_AR
    NAMES arm-none-eabi-gcc-ar arm-none-eabi-ar
    HINTS ${_brsp_arm_hints}
    PATH_SUFFIXES bin
    REQUIRED
)
find_program(CMAKE_RANLIB
    NAMES arm-none-eabi-gcc-ranlib arm-none-eabi-ranlib
    HINTS ${_brsp_arm_hints}
    PATH_SUFFIXES bin
    REQUIRED
)

set(CMAKE_C_FLAGS_INIT
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections"
)
