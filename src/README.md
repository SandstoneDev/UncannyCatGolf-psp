# PSP source build

These are the frozen RC21 compiler inputs: 457 Main files in `runtime/` and 226 Editor files in `editor-runtime/`, with the matching menu media in `xmb/`. The two source directories preserve the release inputs byte for byte, including historical Makefile defaults. Use the wrapper below for a normal release build.

Use [PSPDEV](https://pspdev.github.io/) with PSPSDK, zlib, libogg, libvorbis, libpng and libjpeg. From a Unix shell at the repository root, run:

```sh
PSPDEV=/path/to/pspdev bash src/build.sh
```

If the toolchain is already on `PATH`, `bash src/build.sh` is sufficient. The script builds clean temporary source copies; set `TMPDIR` to choose their location. It produces:

- `src/dist/UCG054/EBOOT.PBP` — normal Main.
- `src/dist/UCG054/data/editor/EBOOT.PBP` — Editor module.

Both builds explicitly use `PSP_LARGE_MEMORY=1` (SFO `MEMSIZE=1`), with `VISUAL_TEST=` for Main and `EDITOR_TEST=` for the module. Main uses `XMB_MEDIA_DIR=../xmb` and the title `Uncanny Cat Golf`. The Editor defines only its required `UCG_EDITOR_MODULE` build mode. Direct `make` without these overrides uses historical diagnostic or memory defaults and does not reproduce the normal release.

EBOOT and ISO use the **same normal Main executable**. Do not build the obsolete `UCG_DISC_BUILD` variant. ISO packaging must supply the matching flat asset transport and Editor handoff from RC21; this script does not create an ISO.

Use the matching **RC21 release data** with these executables. The release includes the asset banks, descriptors and original ExampleWorld data. Building the executables does not regenerate those files, and executables from another version must not be mixed with them. The Editor remains severely slow on real PSP hardware and is currently barely usable; a successful build does not resolve that known issue.

The source tree contains the PSP runtime, embedded resources used by that runtime, and PSP menu assets. Original game assets retain their respective credits and rights. The port's publication permission does not grant a blanket licence to redistribute the original game assets separately. See the repository credits and rights notices.
