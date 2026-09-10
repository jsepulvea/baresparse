#!/usr/bin/env sh
# Explicit programming step, never invoked by CMake build or native tests.
set -eu
: "${UNIFLASH_ROOT:?Set UNIFLASH_ROOT to the installed UniFlash directory}"
: "${C2000WARE_ROOT:?Set C2000WARE_ROOT to the external SDK directory}"
image=${1:-build/ti-f28p55x-firmware-release/platforms/ti/f28p55x/brsp_f28p55x_benchmark.out}
config=${BRSP_TI_CCXML:-"$C2000WARE_ROOT/device_support/f28p55x/common/targetConfigs/TMS320F28P550SJ9_LaunchPad.ccxml"}
test -f "$image"
test -f "$config"
# Invoke the CLI binary directly: the vendor shell wrapper flattens arguments
# through eval. BRSP_DSLITE also permits a different SDK host layout.
dslite=${BRSP_DSLITE:-"$UNIFLASH_ROOT/deskdb/content/TICloudAgent/linux/ccs_base/DebugServer/bin/DSLite"}
test -x "$dslite"
exec "$dslite" flash --config="$config" -f -v "$image"
