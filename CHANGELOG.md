# Changelog

## 0.6.9 / RC17 — 2026-11-26 (release)

Published as the regular latest release. The same game archives and checksums are retained; their enclosed documentation records the earlier prerelease audit.

Original game version: 1.0.3b. This update integrates the main content line developed after the public 053 R15 hotfix.

### Added since the public 053 release

- Summer Villa, costumes, Club Canny, Mystery Phone, the friend quest and secret/story routes.
- Cat Artist/Copycat, Meowls, Uncanny Dash and Bort Bash; Sound Test with58entries.
- Endless Shuffle with82positions (10unique courses plus72campaign replays).
- Eleven profile modifiers and the separate Peak Hunter mode.
- First-start introduction,25attract demos and the remaining original settings.
- Experimental level editor,14original ExampleWorld courses, custom-world save/load and playtest.
- Offline manual sidecars; English/Russian main game support retained.
- Optional generated100%all-unlocks save, distributed separately from both game archives.

### Improved across the integrated content builds

- Conservative collision candidate filtering and spawn-owner counting for large moving smiley groups, including4-8. Actor counts, spawn rules, update order and game simulation were retained.
- Load content when its mode needs it; improve resource/font cache access and avoid frequent diagnostic writes during play.
- D-pad navigation for ordinary menus, pauses and Villa; original pointer for precise actions. Editor/Villa switch between stick pointer and D-pad navigation.
- Original loading prism, selected side borders, transparent text rendering and targeted readability fixes.
- Costume propagation, reward/HUD presentation and remaining original dialogue paths.

### RC17 changes from RC16

- Defer the custom-world catalogue until it is requested instead of parsing it when the editor module starts.
- Use bounded optional metadata/compressed-input windows for glyph reads while retaining the original decoded pixels and glyph-cache policy.
- Use native aiming in custom-level play and editor playtest; shots no longer depend on a clamped mouse pointer. Keep the original terminal-UI pointer behavior.
- Restore original timer size/position and restrained HUD text bounds; modestly enlarge only the smallest labels.
- Persist a navigation checkpoint for both Editing and Playing, with the correct context label.

### Known limitations

**Editor performance on PSP remains severe; it is currently barely usable. Further editor optimization is deferred.** Its inclusion and successful limited functional tests do not imply comfortable hardware use.

Frame/audio stalls remain possible in dense levels, loading/menu transitions and some effects. Fresh RC17 logs confirm a shorter editor-module startup in one uncontrolled comparison, not a whole-game FPS or audio-quality gain. PSP-3000 is the hardware reference; PSP-1000 and every firmware/model combination are not qualified. Firmware HOME behavior and full natural progression coverage remain open.

All113original campaign IDs and the listed major content sets are included. Exact Godot behavior/pixel parity and every natural unlock branch have not been fully verified. PC Godot mods and locales outside EN/RU are not supported. The editor defaults to English.
