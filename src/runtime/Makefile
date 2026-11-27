TARGET = build/ucg_053_cosmic
OBJS = build/main.o build/storage_hooks.o
CFLAGS = -O2 -G0 -Wall -Wextra -Werror -fno-align-functions -fno-align-loops -fno-align-jumps -fno-align-labels -ffunction-sections -fdata-sections
VISUAL_TEST ?= -DUCG_VICTORY_PROBE
CXXFLAGS = -std=gnu++17 -fno-exceptions -fno-rtti $(VISUAL_TEST)
LDFLAGS += -Wl,--gc-sections -Wl,--wrap=fopen -Wl,--wrap=rename -Wl,--wrap=unlink -Wl,--wrap=remove -Wl,--wrap=stat
IO_AUDIT_WRAPS = sceIoOpen sceIoClose sceIoRead sceIoWrite sceIoLseek sceIoLseek32 sceIoOpenAsync sceIoCloseAsync sceIoReadAsync sceIoLseek32Async sceIoPollAsync sceIoWaitAsync sceIoWaitAsyncCB
LDFLAGS += $(foreach symbol,$(IO_AUDIT_WRAPS),-Wl,--wrap=$(symbol))
build/main.o build/main-disc.o: loss_probe.hpp loss_audio_stats.hpp io_audit.hpp transition_trace.hpp park_resource_io.hpp
LIBS = -lpspsystemctrl_user -lpsprtc -lpspgu -lpspaudio -lz -lvorbisfile -lvorbis -logg -lstdc++ -lm
BUILD_PRX = 1
PSP_FW_VERSION = 660
# Preserve original Main84 SFO MEMSIZE=2 explicitly.
PSP_LARGE_MEMORY = 2
EXTRA_TARGETS = $(PSP_EBOOT)
PSP_EBOOT_TITLE = Uncanny Cat Golf - 053 Endless
PSP_EBOOT_SFO = build/PARAM.SFO
PSP_EBOOT = PSP-GAME/UCG053END/EBOOT.PBP

# The staged Windows build runs two directories deeper than the prototype.
XMB_MEDIA_DIR ?= $(if $(wildcard ../xmb/ICON0.PNG),../xmb,../../../xmb)
XMB_MEDIA = $(addprefix $(XMB_MEDIA_DIR)/,ICON0.PNG PIC1.PNG SND0.AT3)
ifneq ($(words $(wildcard $(XMB_MEDIA))),3)
$(error Missing XMB media in $(XMB_MEDIA_DIR))
endif
PSP_EBOOT_ICON = $(XMB_MEDIA_DIR)/ICON0.PNG
PSP_EBOOT_PIC1 = $(XMB_MEDIA_DIR)/PIC1.PNG
PSP_EBOOT_SND0 = $(XMB_MEDIA_DIR)/SND0.AT3

PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PSP_EBOOT): $(XMB_MEDIA)

build/main.o: main.cpp asset_layout.hpp asset_layout.inc elysian_horse040.hpp elysian_horse050809.hpp elysian039.hpp elysian_rocks039.hpp frontier_night_draw037.hpp frontier_night_art037.hpp frontier_titles037.hpp world4_mechanics037.hpp frontier1415_data037.hpp frontier1011_data.hpp frontier_lighting.hpp frontier0607_data.hpp frontier_dog_tiles035.hpp frontier_draw0607.hpp nuke.hpp portal.hpp frontier_courses.hpp frontier_draw.hpp frontier_atlases.hpp selector_frontier_titles.hpp actor_atlas.hpp menu_scroll.hpp menu_scroll_draw.hpp controls.hpp game.hpp tile_collision.hpp visual.hpp media.hpp compact_audio.hpp audio_src_48k.inc world_transition.hpp campaign.hpp course_objects.hpp results.hpp golf.hpp locale.hpp key_doors.hpp pool.hpp gameplay_view.hpp glowstick.hpp stop_tokens.hpp billboards.hpp par_ball.hpp level_select.hpp level_select_art.hpp profiles.hpp profile_art.hpp profile_keyboard.hpp profile_ui.hpp selector_details.hpp selector_details_draw.hpp gift_ui.hpp selector_gifts.hpp selector_transition.hpp baddie_run.hpp hardware_probe.hpp asset_file.hpp jump_pad.hpp police_event.hpp secret_runner.hpp news.hpp pause_menu.hpp pause_art.hpp goal_flag.hpp gift_atlas.hpp slot_machine.hpp slot_machine_art.hpp slot_machine_draw.hpp park_art.hpp park_stage_art.hpp switch_courses.hpp switch_blocks.hpp smiley_ring.hpp park_video.hpp selector_park_titles.hpp chuckle_run.hpp selector_next_titles.hpp selector_dog_titles.hpp dog_courses032.hpp multi_dog032.hpp park_late032_data.hpp selector-extra-park-titles.hpp park_events031.hpp ring_timer_audio.hpp park_late031.hpp park_late031_data.hpp selector-final-park-titles.hpp park_dog_screen.hpp park_switch_patches031.hpp chuckle_art.hpp coaster_art.hpp selector_factory_titles.hpp marcy_run.hpp marcy_reveal.hpp dense_blocks.hpp coaster_spring_art.hpp selector_late_park_titles.hpp selector_burst_gift.hpp marcy_reveal.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/main.o build/main-disc.o: endless_runtime.hpp endless_run.hpp endless_geometry.hpp \
    endless_objects.hpp endless_objects_art.hpp endless_modifiers.hpp endless_modifier_art.hpp
build/main.o build/main-disc.o: editor_audio.hpp editor_builder_audio.hpp editor_builder_gift.hpp editor_builder_gift_data.hpp editor_builder_gift_raster.hpp editor_builder_gift_source.hpp editor_builder_gift_source_data.hpp editor_builder_main.hpp editor_builder_present.hpp editor_builder_reward.hpp

build/main.o: thought_bank.hpp selector_transition_compact.hpp
build/main.o: boot_splash041.inc

build/main-disc.o: main.cpp boot_splash041.inc iso_assets052.inc
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/main.o: elysian_geometry041.hpp frontier051011_visual.hpp

build/main.o: elysian_geometry042.hpp elysian_traffic042.hpp frontier051213_visual.hpp

build/main.o: elysian_geometry043.hpp elysian_threats043.hpp elysian_mist043_draw.hpp frontier051415_visual.hpp

build/main.o: elysian_geometry044.hpp elysian_wizard044.hpp elysian_wizard044_draw.hpp frontier051617_visual.hpp elysian_wind044_audio.hpp

build/main.o: elysian_geometry045.hpp elysian_finale045.hpp elysian_finale045_runtime.hpp elysian_finale045_draw.hpp elysian_ending045.hpp frontier05180601_visual.hpp

build/main.o: cosmic_geometry046.hpp cosmic060203_visual.hpp cosmic_assets046.hpp cosmic_draw046.hpp cosmic_draw_extra046.hpp cosmic_selector046.hpp

build/main.o: cosmic_geometry047.hpp cosmic060405_visual.hpp cosmic_assets047.hpp starstorm047.hpp starstorm_draw047.hpp starstorm_ring047.hpp

build/main.o: cosmic_geometry048.hpp cosmic060607_visual.hpp metronome048.hpp cosmic_assets048.hpp cosmic_draw048.hpp

build/main.o: cosmic_geometry049.hpp cosmic0809_visual.hpp polyrhythm049.hpp cosmic_assets049.hpp cosmic_draw049.hpp

build/main.o: cosmic_mechanics050.hpp cosmic1011_visual.hpp cosmic_assets050.hpp cosmic_draw050.hpp

build/main.o: cosmic_geometry051.hpp cosmic_goal_path051.hpp cosmic1213_visual.hpp moving_goal051.hpp cosmic_assets051.hpp cosmic_draw051.hpp disco_light051.hpp

build/main.o: cosmic_geometry052.hpp cosmic1415_visual.hpp cosmic_assets052.hpp cosmic_draw052.hpp santa052.hpp

build/main.o: club_geometry054.hpp club_physics054.hpp club_runtime054.hpp club_text054.hpp costume_runtime054.hpp focused054.hpp phone_art054.hpp phone_runtime054.hpp phone_text054.hpp robert-animation054.hpp villa054.hpp villa_art054.hpp villa_costume_art054.hpp villa_costume_rows054.inc villa_costumes054.hpp villa_residents054.hpp villa_unlocks054.hpp
build/main.o: cat_artist_art.hpp cat_artist_authored.hpp cat_artist_book.hpp cat_artist_global.hpp cat_artist_copycat.hpp cat_artist_dialogue.hpp cat_artist_palette.hpp cat_artist_runtime.hpp

build/main.o: editor_entry.hpp editor_entry_data.hpp

build/storage_hooks.o: storage_hooks.cpp storage_hooks.hpp storage_reader.hpp storage_stdio.hpp storage_pool.hpp storage_file_pool.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/main.o: storage_hooks.hpp

build/main.o: editor_legacy_gift_blast.hpp editor_legacy_gift_blast_data.hpp

build/main.o build/main-disc.o: media.hpp compact_audio.hpp editor_audio.hpp bort_voice.hpp sound_test_spectrum.hpp

build/main.o: bort_art.hpp bort_art_data.hpp bort_runtime.hpp bort_sim.hpp dash_art.hpp dash_art_data.hpp dash_runtime.hpp dash_sim.hpp meowls_art.hpp meowls_art_data.hpp meowls_presentation.hpp meowls_runtime.hpp meowls_schedule.hpp meowls_sim.hpp sound_test_art.hpp sound_test_art_data.hpp sound_test_audio.hpp sound_test_runtime.hpp sound_test_state.hpp

build/main.o build/main-disc.o: campaign_modifiers.hpp campaign_mod_options.hpp campaign_modifier_art_data.hpp editor_costume_sounds.hpp

build/main.o build/main-disc.o: startup_settings.hpp startup_intro_state.hpp startup_intro.hpp startup_intro_art.hpp startup_intro_art_data.hpp user_audio_buses.hpp
build/main.o build/main-disc.o: attract_data.hpp attract_demo.hpp attract_runtime.hpp

build/main.o build/main-disc.o: startup_commit.hpp startup_editor_bus.hpp
build/main.o build/main-disc.o: actor_pronouns054.hpp phone_pronouns054.hpp actor_pronoun_clut_draw054.hpp actor_pronoun_clut054.inc
build/main.o build/main-disc.o: phone_settings054.hpp scene_clock.hpp world_state.hpp world_awards.hpp world_progress054.hpp world_runtime054.hpp world_legacy054.hpp world_gift_ui.hpp world_gift_art.hpp world_gift_sequence.hpp club_events054.hpp club_npc_draw054.hpp club_tracks054.hpp club_visit054.hpp

build/main.o build/main-disc.o: shot_settings.hpp

build/main.o build/main-disc.o: swarm_peer_grid066.hpp frame_profile066.hpp endless_hud_layout066.hpp chapter_loading066.inc

build/main.o build/main-disc.o: home_exit067.hpp

build/main.o build/main-disc.o: endless_ui067.hpp endless_ui067_data.hpp

build/main.o build/main-disc.o: theme067.hpp theme067_data.hpp
