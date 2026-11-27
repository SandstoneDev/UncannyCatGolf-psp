# RC21 validation

Original game 1.0.3b. PSP port 0.7.6 / RC21. Main and Editor are normal MEMSIZE=1 MIPS builds; no test, auto-win or Media Engine mode is enabled. ISO and EBOOT contain the same Main executable. The editor executable is unchanged from RC20.

## Save system

Campaign/profile/shared progress uses the PSP Saved Data Utility entry `PSP/SAVEDATA/UCGP00054GAME`. All eight profile slots share this entry between ISO and EBOOT. Legacy progress is imported only when native data is absent; valid native data always wins. Corrupt native data does not silently create empty progress. Settings, artwork and custom levels remain ordinary user files.

On a real PSP-3000, RC20 ISO imported old progress and securely saved generation 1. After restarting the process, it loaded generation 1 directly from native data and saved generation 2 after play. RC21 retains the exact same native controller, format and stable key. The owner's game and native-save files were hash-checked before and after RC21 installation and preserved. No bonus save was installed on the owner's card.

Host regression coverage includes the archive/router/controller, legacy save versions, multiple profile slots, shared state, close/retry/error paths, and PSP ABI compilation. A prior false pre-initialization busy check was corrected: SDK InitStart determines admission; controller lifetime and error checks remain enforced.

## Animation optimization

Only the Main animation decoder changed from RC20. Reference/delta tiles can reconstruct in existing cached scratch memory and then copy once to VRAM. Budget and overlap guards retain the prior path when staging is unsuitable. No extra heap buffer, frame skipping, content reduction or physics/RNG change was introduced.

The actual 6-09 and 6-08 fixture sets cover 1,102 behavioral cases per decoder build and 266 frozen frames. Optimized and baseline builds produced identical pixels, metadata and I/O results with O2 and ASAN/UBSAN. The extra uncached reconstruction work fails the baseline work gate and passes the candidate gate. The optimization affects reference/delta banks; keyframe-only paths retain their prior behavior.

These tests prove the tested decoder outputs and work routing. **A paired real-PSP RC21 speed/audio comparison is still pending. No numerical FPS gain or elimination of sound stalls is claimed.** The maintainer approved publishing this version with that limitation.

## Package scope

The Main and Editor source trees are bound to their compiled source archives. All game payload hashes and ISO members were read back. Public ZIPs are separately checked member by member. They exclude player progress, private manifests, test helpers and diagnostic logs. The optional generated native 100% save is a separate download with its own installation warning.

The main content inventory remains intact with English/Russian. This is a content inventory statement, not an exhaustive proof of every original behavior or natural unlock. See [known issues](KNOWN_ISSUES.md), particularly the editor's severe hardware slowdown.

## Binary SHA-256

| Artifact | SHA-256 |
|---|---|
| Main EBOOT.PBP | `55683652a1d0d00eb35f45714f61a8c706943f6b04749878d81965025a3ae1e0` |
| Editor EBOOT.PBP | `ce70b650ebecc93f0beb41961781eb566971bfefb9628a3150289bbfc21b50eb` |
| RC21 ISO | `0be6c2a2facba7bb6dccba5aea80d091defd32ceb948d9b440f1f35655bd0b29` |

## По-русски

На PSP-3000 подтверждены перенос старого прогресса, нативная запись, перезапуск игры, загрузка нативного сейва и повторная запись. В RC21 система сейвов не менялась. ISO и EBOOT используют один общий нативный сейв; пользовательский прогресс при установке сохранён.

Проверки декодера RC21 подтверждают одинаковые изображения и состояние на 1 102 сценариях для 266 реальных кадров. Прирост скорости и качество звука на консоли ещё предстоит сравнить в одинаковых условиях. Редактор остаётся очень медленным. Полный перечень контента сохранён, но каждая механика и естественное открытие полностью не проверены.
