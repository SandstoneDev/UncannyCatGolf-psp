# Uncanny Cat Golf for PSP

![Uncanny Cat Golf logo](docs/media/logo.png)

[Русский](README.ru.md) · [053 R9 Hotfix](https://github.com/SandstoneDev/UncannyCatGolf-psp/releases/tag/v1.0.3b-psp.053-r9)

An unofficial PSP port by **SandstoneDev**, based on the original **1.0.3b**. The current PSP release is **port build 053 R9 Hotfix**.

## What's playable

- All **113 campaign holes** across seven worlds, including the tutorial, world results, and campaign endings.
- Campaign map, scores, ranks, records, and unlockable rewards used by the campaign.
- English and Russian menus and text, PSP controls, multiple profiles, and save entries in the PSP's Saved Data Utility.
- Hold SELECT to skip eligible holes with the original progress bar animation.

## Install

Choose one archive. On PSP, use a system set up to run homebrew in the format you choose.

**[Download ISO ZIP](https://github.com/SandstoneDev/UncannyCatGolf-psp/releases/download/v1.0.3b-psp.053-r9/Uncanny-Cat-Golf-1.0.3b-PSP-053-R9-ISO.zip)**

1. Extract `Uncanny-Cat-Golf.iso` from the ZIP.
2. Copy the ISO to `ISO` at the root of the memory card. In PPSSPP, open the extracted ISO directly.

**[Download EBOOT ZIP](https://github.com/SandstoneDev/UncannyCatGolf-psp/releases/download/v1.0.3b-psp.053-r9/Uncanny-Cat-Golf-1.0.3b-PSP-053-R9-EBOOT.zip)**

1. Extract the included `PSP` folder to the root of the memory card.
2. Keep `EBOOT.PBP` and the `data` folder together inside `PSP/GAME/UCG053`. In PPSSPP, open the extracted `EBOOT.PBP`.

On PSP, launch **Uncanny Cat Golf** from **Game → Memory Stick**. Keep your existing saves when updating. The ISO keeps progress in `PSP/UCG053-ISO` and the EBOOT keeps it in `PSP/GAME/UCG053`.

[SHA-256 checksums for both archives](https://github.com/SandstoneDev/UncannyCatGolf-psp/releases/download/v1.0.3b-psp.053-r9/SHA256SUMS.txt).

The port supports **PSP-1000 (Fat)**. Both formats were tested in PPSSPP. The R9 ISO also launched on a physical PSP without overclocking.

## Screenshots

| Title screen | Campaign map | Tutorial |
| --- | --- | --- |
| ![Title screen on PSP](docs/media/title.png) | ![Campaign map in Russian](docs/media/campaign-map.png) | ![Playing a tutorial hole](docs/media/tutorial.png) |

## Known issues and roadmap

R9 adjusts the ISO boot layout, caches the animated fields on 2_06 and 2_08, and preloads confetti on holes that share the same victory animation. There can be short frame or audio stalls, particularly around the title screen, some transitions, and victory animations. Further releases will improve performance and bring the port closer to the PC version.

- Finish and integrate Summer Villa, costumes, Club Canny, Mystery Phone, and the secret quest. Work on these features is underway in a separate development build.
- Add standalone minigames, Sound Test, Endless Shuffle, remaining modifiers, and the level editor with custom worlds.
- Continue checking UI, input, and effects against the PC release.

[Building the PSP executable](src/README.md)

## Credits

Original game by SH2K (SlappyHappy2000), Haze Weathers, mbx8rt, Penny Rigate, greenhead352, and bibizu. PSP port by SandstoneDev.

[Original game and PC downloads](https://slappyhappy2000.itch.io/uncanny-cat-golf) · [Original game Discord](https://discord.gg/bkZM7cUwNq)
