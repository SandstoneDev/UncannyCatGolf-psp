# Saves and updates / Сохранения и обновление

## Current RC17

Both the standard EBOOT and ISO use `PSP/GAME/UCG054` for mutable game state. Keep the entire folder when updating: profile files, shared world/config files, custom levels and any art/editor files. Do not delete it just because you launch the ISO. An EBOOT in a renamed folder uses that folder instead; the ISO always uses `UCG054`.

The `PSP/SAVEDATA/UCGP00054PROFILE##` XMB entries are mirrors; do not treat them alone as a verified recovery backup. Copy the game-state folder too.

ISO и стандартный EBOOT RC17 используют общую `PSP/GAME/UCG054`. При обновлении сохраните всю папку: профили, общие открытия/настройки, свои уровни, рисунки и редакторские файлы. ISO тоже использует её. EBOOT в переименованной папке хранит данные рядом с собой; ISO всегда использует `UCG054`.

Записи `PSP/SAVEDATA/UCGP00054PROFILE##` — зеркало XMB, а не отдельно проверенный способ восстановления. Основную папку игры тоже нужно копировать.

## Old public053 builds

Keep `PSP/GAME/UCG053`, `PSP/UCG053-ISO` and old PSP saved-data entries intact. RC17 does not automatically move those old directories. Back them up before experimenting; do not assume that blindly copying old shared configuration or XMB mirrors is a supported migration. Keep the old build side by side until your progress is confirmed. A missing profile in the new directory does not mean the old profile was deleted.

Сохраните `PSP/GAME/UCG053`, `PSP/UCG053-ISO` и старые записи PSP. RC17 не переносит эти папки автоматически. Сделайте резервную копию; слепое копирование старых общих настроек/зеркал XMB не считается проверенной миграцией. Оставьте старую версию рядом, пока не убедитесь в наличии нужного прогресса. Отсутствие профиля в новой папке не означает удаление старого.

## Optional100%bonus save

The game archives contain no personal saves and do not install the bonus save. Download it separately only if wanted. Back up the complete `PSP/GAME/UCG054` first. Its profile slot and shared-state replacement affect different scopes: profile07 replaces that slot; shared unlocks/quests apply to every profile. Restore your full backup to undo the shared-state replacement. Follow the exact file list/definition in the archive; it is a developer-generated unlock save, not a natural leaderboard record. It is intended for this RC17 state format, not the old053 release.

Обычные архивы игры не содержат личных сейвов и не устанавливают бонусный. Он скачивается отдельно, только по желанию. Сначала скопируйте всю `PSP/GAME/UCG054`. Профиль07 заменяет этот слот; общие открытия/квесты влияют на все профили. Чтобы отменить замену общих данных, восстановите полную резервную копию. Точный список файлов и определение100%смотрите в архиве. Это сгенерированный сейв открытий, не рекорд естественного прохождения; формат рассчитан на RC17, не старый053.

## Native PSP saves / Нативные сохранения PSP

Both current EBOOT and ISO call the PSP savedata AUTOSAVE API to write a profile mirror under `PSP/SAVEDATA/UCGP00054PROFILE##`. This is not an ISO-only feature. The current runtime does not load profiles back through AUTOLOAD/LOAD: authoritative state is read from `PSP/GAME/UCG054`. The bonus archive therefore supplies those game-state files, including shared unlocks that a profile mirror alone would not restore.

Нативная запись через PSP savedata работает у текущих EBOOT и ISO. Она создаёт зеркало профиля в `PSP/SAVEDATA/UCGP00054PROFILE##`; это не функция только для ISO. Обратная загрузка профиля через AUTOLOAD/LOAD пока не подключена. Основные данные игра читает из `PSP/GAME/UCG054`, поэтому бонусный архив содержит именно эти файлы, включая общие открытия, которые одним зеркалом профиля не восстановить. Это ограничение текущей интеграции, а не ограничение PSP.
