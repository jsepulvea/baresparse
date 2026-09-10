#!/usr/bin/env sh
# Compile/archive the portable library and link an API consumer. Never run/flash.
set -eu

if [ -n "${ARM_NONE_EABI_ROOT:-}" ]; then
    PATH="$ARM_NONE_EABI_ROOT/bin:$PATH"
fi
if [ -n "${XC_DSC_ROOT:-}" ]; then
    PATH="$XC_DSC_ROOT/bin:$PATH"
fi
export PATH
: "${XC_DSC_DFP:?Set XC_DSC_DFP to the device pack's xc16 directory}"

arm-none-eabi-gcc --version
xc-dsc-gcc --version
cmake --fresh --preset arm-cortex-m4-release -DBRSP_WARNINGS_AS_ERRORS=ON
cmake --build --preset arm-cortex-m4-release
cmake --fresh --preset microchip-dspic33a-release -DBRSP_WARNINGS_AS_ERRORS=ON
cmake --build --preset microchip-dspic33a-release

# The vendor/default memory layouts below are link checks, not board firmware.
cat > build/cross-link.c <<'SOURCE'
#include <baresparse/brsp.h>
brsp_status brsp_compile_ordered_smoke(void);
int main(void) {
    return brsp_compile_ordered_smoke() == BRSP_STATUS_OK ? 0 : 1;
}
SOURCE
arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 \
    -mfloat-abi=hard --specs=nosys.specs \
    -Iinclude -Ibuild/arm-cortex-m4-release/generated build/cross-link.c \
    build/arm-cortex-m4-release/CMakeFiles/brsp_compile_smoke.dir/tests/compile_smoke.c.obj \
    build/arm-cortex-m4-release/libbaresparse.a \
    -o build/arm-cortex-m4-release/ordered-consumer.elf
xc-dsc-gcc -std=c11 -mcpu=33AK512MPS506 -mdfp="$XC_DSC_DFP" \
    -Wl,-T,"$XC_DSC_DFP/support/dsPIC33A/gld/p33AK512MPS506.gld" \
    -Iinclude -Ibuild/microchip-dspic33a-release/generated build/cross-link.c \
    build/microchip-dspic33a-release/CMakeFiles/brsp_compile_smoke.dir/tests/compile_smoke.c.obj \
    build/microchip-dspic33a-release/libbaresparse.a \
    -o build/microchip-dspic33a-release/ordered-consumer.elf

arm-none-eabi-readelf -A build/arm-cortex-m4-release/ordered-consumer.elf
xc-dsc-objdump -mdfp="$XC_DSC_DFP" -f build/microchip-dspic33a-release/ordered-consumer.elf
# Both consumers must resolve the complete ordered lifecycle and runtime calls.
arm_undefined=$(arm-none-eabi-nm -u build/arm-cortex-m4-release/ordered-consumer.elf)
dspic_undefined=$(xc-dsc-nm -mdfp="$XC_DSC_DFP" -u build/microchip-dspic33a-release/ordered-consumer.elf)
# Vendor startup may leave optional weak hooks undefined.
strong_undefined=$(printf '%s\n%s\n' "$arm_undefined" "$dspic_undefined" | awk '$1 == "U" {print}')
if [ -n "$strong_undefined" ]; then
    printf '%s\n%s\n' "$arm_undefined" "$dspic_undefined"
    exit 1
fi
