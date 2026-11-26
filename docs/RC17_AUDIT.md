# RC17 prerelease audit / Предрелизный аудит

Audit date:2026-11-26. Source baseline: original1.0.3b; PSP0.6.9/RC17. The source tree and game payload remain frozen; this audit updates publication/documentation and adds a separately generated optional save.

## Content inventory

113original campaign IDs match the original roster exactly. Endless has82positions (72campaign replays+10unique), and ExampleWorld has14original courses:137unique golf maps in total. Four minigames/activities,58Sound Test entries,11profile modifiers+Peak Hunter,24costume preference IDs,48global award keys and25attract demos are represented. This is content-presence coverage, not113natural wins or exhaustive event parity. PC mods and four locales outside EN/RU are excluded.

## Package and functional verification

Fresh checks bind453Main inputs and222Editor inputs to compiler archives and normal executable payloads; all1012game files and all1019actual ISO members/extents match. Normal MEMSIZE1, no autoplay/autowin/private or Media Engine experiment. The ISO is1,077,872,640bytes; the EBOOT payload plus its internal build manifest is1,036,951,740bytes. No content was removed to hit a size cap.

Existing final EN EBOOT/RU ISO real-input test chains were revalidated: native shots at the pointer edge, private edit→undo/redo→save→reload→playtest/restart→Main, preserved Main language and original timer layout. Those limited emulator checks do not establish physical performance or full natural progression. The previous owner-card installation checked all1012game files, ISO and56owner-file backups; the audit itself did not alter the card or owner data.

## Hardware comparison: RC16→RC17 on PSP-3000

| Measurement | RC16 | RC17 |
|---|---:|---:|
| Editor module entry→ready |15.768558s|6.798689s|
| Audio initialization |2.918413s|2.909367s|

These are single uncontrolled runs, not a paired benchmark. Module readiness is its menu, not the first editing canvas. RC17 still has a13.863816s blocking first Editing frame. There is no fresh steady Editor window or persisted audio-underrun checkpoint. The main-game frame profile is byte-identical to the older RC16 file; no new gameplay FPS can be claimed from it.

**Release limitation: Editor remains very slow and barely usable on PSP.** Main-game stutters, audio quality, firmware HOME, other PSP models and complete long-play coverage remain qualification work. No stable30/60FPS or100%PC parity claim is made.

The internal historical build manifest still has an inherited RC15/RC16 hardware-status string. It is not RC17 evidence; this dated audit and the current release notes supersede that stale text. The published game payload and ISO are the tested RC17 binaries/assets; public archive metadata is supplied separately.

## Кратко по-русски

Состав113лунок кампании точно совпадает с оригиналом; основные режимы и каталоги контента включены. Целостность исходников, нормальных бинарников,1012файлов и1019файловISO проверена заново. Это не полный аппаратный/естественный прогон всех маршрутов.

Модуль редактора в одном прогоне открылся быстрее:15,77→6,80с. Затем первый кадр редактирования занял13,86с; редактор остаётся почти непригодным для комфортной работы. Инициализация звука почти прежняя; качество музыки/прерывания по новым логам не измерены. Свежего профиля основной игры нет. EN/RU и файлы владельца сохранены; дальнейшая оптимизация Editor отложена.
