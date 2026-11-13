#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ -n ${PSPDEV:-} ]]; then
    export PATH="$PSPDEV/bin:$PATH"
fi
command -v psp-config >/dev/null || { echo 'Install PSPSDK or set PSPDEV.' >&2; exit 1; }
cd "$root/runtime"
mkdir -p build PSP-GAME/UCG053
make -j2 all XMB_MEDIA_DIR=../xmb
make -j2 all OBJS=build/main-disc.o TARGET=build/ucg_053_disc VISUAL_TEST=-DUCG_DISC_BUILD \
    PSP_EBOOT=build/EBOOT-disc.PBP PSP_EBOOT_SFO=build/DISC-PARAM.SFO XMB_MEDIA_DIR=../xmb
