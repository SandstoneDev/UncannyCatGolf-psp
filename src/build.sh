#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ -n ${PSPDEV:-} ]]; then
    export PATH="$PSPDEV/bin:$PATH"
fi
command -v psp-config >/dev/null || { echo 'Install PSPSDK or set PSPDEV.' >&2; exit 1; }
command -v make >/dev/null || { echo 'Install make.' >&2; exit 1; }

# Build clean copies so the frozen compiler inputs stay byte-for-byte intact.
work=$(mktemp -d "${TMPDIR:-/tmp}/ucg-rc17-build.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
cp -R -- "$root/runtime" "$work/runtime"
cp -R -- "$root/editor-runtime" "$work/editor-runtime"
cp -R -- "$root/xmb" "$work/xmb"
mkdir -p -- "$work/runtime/build" "$work/runtime/PSP-GAME/UCG053END" "$work/editor-runtime/build"

# The preserved historical Makefiles have diagnostic/memory defaults.
# These explicit normal-release overrides are required for BOTH executables.
make -C "$work/runtime" -j2 all 'VISUAL_TEST=' PSP_LARGE_MEMORY=1 \
    XMB_MEDIA_DIR=../xmb 'PSP_EBOOT_TITLE=Uncanny Cat Golf'
make -C "$work/editor-runtime" -f EditorMakefile -j2 all 'EDITOR_TEST=' PSP_LARGE_MEMORY=1

output="$root/dist/UCG054"
mkdir -p -- "$output/data/editor"
cp -- "$work/runtime/PSP-GAME/UCG053END/EBOOT.PBP" "$output/EBOOT.PBP"
cp -- "$work/editor-runtime/build/EDITOR.PBP" "$output/data/editor/EBOOT.PBP"
printf 'Normal Main: %s\nEditor module: %s\n' "$output/EBOOT.PBP" "$output/data/editor/EBOOT.PBP"
printf '%s\n' 'Both use MEMSIZE=1. ISO packaging reuses the same normal Main; do not enable UCG_DISC_BUILD.'
printf '%s\n' 'Add the matching RC17 release data; this script does not regenerate asset banks or build an ISO.'
