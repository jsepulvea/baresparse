# Third-party material

BareSparse's sources, tests, and structured synthetic fixture are original project
code under the root MIT license. The long-double host reference and Boolean
symbolic oracle are independently implemented here, with no imported solver code.

The optional TI firmware build consumes Texas Instruments C2000Ware device
startup, linker configuration, device initialization source, headers, and a
prebuilt driverlib from an **external SDK installation**. TI's compiler also
links its runtime support library. These vendor distributions are not copied
into this repository and are not relicensed under BareSparse's MIT license.
Keep their original copyright/license notices and comply with their distribution
terms if redistributing a linked firmware image or any SDK material. Inspect
licenses/manifests in the selected compiler and SDK distributions. Reproduction
instructions identify the versions used; public snapshots exclude their binaries.

Arm GNU Toolchain, Microchip XC-DSC and Microchip device-family packs used for
cross-build verification are also external installations. The link-check
consumers use vendor/toolchain startup, runtime libraries and linker scripts;
those generated ELF files are ignored build artifacts. Their external components
retain their respective licenses and are not relicensed under this project's
MIT license. Download sources and distribution hashes are recorded in
[docs/VALIDATION.md](docs/VALIDATION.md).
