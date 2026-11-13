# PSP source build

Use [PSPDEV](https://pspdev.github.io/) with PSPSDK, zlib, libogg and libvorbis. From a Unix shell, run `bash src/build.sh`. With a custom toolchain location, set `PSPDEV` to that directory first.

The build produces `src/runtime/PSP-GAME/UCG053/EBOOT.PBP` for a loose-file installation and `src/runtime/build/EBOOT-disc.PBP` for ISO packaging. The GitHub release ISO includes the matching game data. Building the executable from source does not regenerate the asset banks.

The source tree contains the PSP runtime, embedded resources used by that runtime, and PSP menu assets. Original game assets retain their respective credits and rights. The port's publication permission does not grant a blanket licence to redistribute the original game assets separately.
