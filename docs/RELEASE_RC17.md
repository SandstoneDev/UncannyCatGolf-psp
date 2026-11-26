# Uncanny Cat Golf 1.0.3b for PSP — 0.6.9 / RC17

**Prerelease content update by SandstoneDev. The level editor is included, but suffers from severe slowdown on PSP and is currently barely usable. Editor optimization is deferred.**

This release brings together the main content developed after the public053R15 build: all113campaign courses, Summer Villa, costumes, Club Canny, Mystery Phone and the friend/secret quest; Endless Shuffle; Cat Artist/Copycat, Meowls, Uncanny Dash and Bort Bash; Sound Test58, campaign modifiers/Peak Hunter, first-start introduction and25demos. Experimental custom-world editing includes all14original ExampleWorld courses. Main game: English/Russian; editor: English.

RC17 defers the custom-level catalogue until requested, reduces small font reads, restores the original timer/restraint in HUD text sizing, and fixes native aiming in custom-level play/playtest. Integrated smiley-group optimizations retain actors, spawns and simulation order. No content was cut to conceal slowdown.

## Downloads

- **EBOOT ZIP**: extract the included `PSP` folder to the card root. Keep the complete `PSP/GAME/UCG054` folder, including `storage.ucd` and `data`.
- **ISO ZIP**: extract the included `ISO` and `PSP` folders to the card root. Launch the ISO; `PSP` supplies the optional offline manual.
- **100-Percent-Save ZIP**: OPTIONAL SEPARATE DOWNLOAD. Generated developer unlock preset, not natural completion or a speed/leaderboard record. Uses slot07 and replaces shared unlock/quest state; first back up the entire `PSP/GAME/UCG054`. Both game archives exclude it. Exact113PEAK/565prism and global-unlock definition, hidden-course counter behavior and limitations are in its bilingual README.
- Verify the archives with **SHA256SUMS.txt**. **ISO_SHA256.txt** covers the unchanged extracted ISO; **BUILD_INFO.json** records the frozen game identities.

Current ISO and standard EBOOT share saves/config/custom worlds at `PSP/GAME/UCG054`. Keep it on updates. Old053directories are not automatically migrated; keep them backed up and see the repository's save instructions.

## Verification and limitations

Fresh675source-input checks, a clean public-source build,1012payload files and1019actual ISO members passed; both compiled PBP files match the tested RC17 binaries. Limited EN EBOOT/RU ISO real-input workflows and private save roundtrips were checked. This is not exhaustive natural progression or physical qualification.

On the owner's PSP-3000, editor-module entry→ready changed15.77→6.80s in single uncontrolled runs, but the first Editing frame still blocked13.86s. There is no fresh whole-game FPS or audio-underrun measurement. Main-game frame/audio stalls remain possible, and stable30/60FPS, PSP-1000 compatibility, complete HOME firmware behavior and exactPCparity are not claimed. PC Godot mods and locales outside EN/RU are not supported.

---

## По-русски

**Предрелиз0.6.9/RC17. Редактор включён, но на PSP очень сильно тормозит и практически непригоден для комфортной работы; дальнейшая его оптимизация отложена.**

Объединён основной контент после публичного053R15:113лунок кампании, Summer Villa, костюмы, Club Canny, Mystery Phone, квест друга и секретные маршруты, Endless, четыре мини-игры, Sound Test58, модификаторы/Peak Hunter, первое вступление и25демо. Включены14оригинальных ExampleWorld. Основная играEN/RU, редактор английский.

Два архива игры — **EBOOT** и **ISO** — содержат необходимые ассеты. Необязательный **сейв100%скачивается отдельно** и не устанавливается обычными архивами. Это сгенерированный сейв открытий: слот07 и общие награды/квест. Перед установкой скопируйте всю `PSP/GAME/UCG054`; точный состав и исключения указаны в егоREADME. Рекорды естественного прохождения не заявляются.

Проверены целостность пакетов, соответствие исходников бинарникам и ограниченные функциональные сценарии. Загрузка модуля редактора в аппаратном прогоне стала короче15,77→6,80с, но затем первый кадр редактирования занял13,86с. ПриростFPS всей игры и улучшение звука по свежим логам не доказаны. В тяжёлых сценах/переходах остаются задержки. Основной контент перечисленных режимов перенесён; полное совпадение каждой механики/эффекта и всех естественных открытий не подтверждено.

[Changelog](https://github.com/SandstoneDev/UncannyCatGolf-psp/blob/v1.0.3b-psp.069-rc17/CHANGELOG.md) · [Список изменений](https://github.com/SandstoneDev/UncannyCatGolf-psp/blob/v1.0.3b-psp.069-rc17/CHANGELOG.ru.md) · [Original game / Оригинал](https://slappyhappy2000.itch.io/uncanny-cat-golf)
