# Save data / Сохранения

Progress is read and written through the native PSP Saved Data Utility:
`PSP/SAVEDATA/UCGP00054GAME`. This single XMB entry contains all eight game profiles and shared unlocks, rewards, quest state, costumes and Endless records. EBOOT and ISO share it.

The first launch imports the previous loose progress only when no native entry exists. An unreadable or corrupt native entry is reported, never replaced by empty progress. Once native data is valid, it is authoritative. Old loose files and per-profile XMB mirrors remain recovery copies; the new game does not update them. Keep a backup when upgrading or downgrading: RC17/RC18 cannot read the new shared native entry.

Settings, player artwork and custom levels remain ordinary configuration/user-content files in `PSP/GAME/UCG054`. They are not campaign progress. Copy them separately when moving the entire installation.

The optional native 100% download is separate from both game archives. It replaces the whole shared entry, including every profile. Its profile 07 and shared unlocks are generated; its scores are not claimed as natural records. See its included README for the disclosed completion markers.

## Русский

Прогресс читается и записывается через нативную систему сохранений PSP:
`PSP/SAVEDATA/UCGP00054GAME`. В одной записи меню PSP хранятся все восемь профилей и общие открытия, награды, задания, костюмы и результаты Endless. ISO и EBOOT используют эту запись совместно.

При первом запуске старый прогресс переносится только при отсутствии нативного сейва. Повреждённый или недоступный сейв не заменяется пустым. После успешной загрузки нативный сейв становится основным. Старые файлы прогресса и прежние записи отдельных профилей остаются резервными копиями и больше не обновляются. При обновлении или откате сохраняйте резервную копию: RC17/RC18 не читают новый общий сейв.

Настройки, рисунки игрока и созданные уровни остаются обычными файлами в `PSP/GAME/UCG054`. При переносе всей установки копируйте их отдельно.

Необязательный нативный сейв 100% скачивается отдельно. Он заменяет всю общую запись со всеми профилями. Профиль 07 и общие открытия сгенерированы; рекорды естественного прохождения не заявляются. Подробности о маркерах завершения указаны в его README.
