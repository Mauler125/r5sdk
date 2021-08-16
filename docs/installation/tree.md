# **File structure of an apex install**

## Example of the file structure of an apex install.

```
|   amd_ags_x64.dll
|   bink2w64.dll
|   binkawin64.dll
|   build.txt
|   gameinfo.txt
|   gameversion.txt
|   mileswin64.dll
|   r5apex.exe
|   r5apexdata.bin
|   
+---audio
|   \---ship
|           audio.mprj
|           general.mbnk
|           general.mbnk_digest
|           general_english.mstr
|           general_english_patch_1.mstr
|           general_stream.mstr
|           general_stream_patch_1.mstr
|           
+---bin
|       crashmsg.exe
|       dxsupport.cfg
|       
+---cfg
|   |   config_default_pc.cfg
|   |   cpu_level_0_pc.ekv
|   |   cpu_level_1_pc.ekv
|   |   cpu_level_2_pc.ekv
|   |   cpu_level_3_pc.ekv
|   |   gpu_level_0_pc.ekv
|   |   gpu_level_1_pc.ekv
|   |   gpu_level_2_pc.ekv
|   |   gpu_level_3_pc.ekv
|   |   gpu_mem_level_0_pc.ekv
|   |   gpu_mem_level_1_pc.ekv
|   |   gpu_mem_level_2_pc.ekv
|   |   gpu_mem_level_3_pc.ekv
|   |   gpu_mem_level_4_pc.ekv
|   |   mem_level_0_pc.ekv
|   |   mem_level_1_pc.ekv
|   |   mem_level_2_pc.ekv
|   |   mem_level_3_pc.ekv
|   |   video_settings_changed_quit.cfg
|   |   
|   +---client
|   |       st_data.bin
|   |       
|   \---server
|           persistent_player_data_manifest.rson
|           persistent_player_data_version_512.pdef
|           
+---materials
|   \---correction
|       |   ability_hunt_mode.raw_hdr
|       |   adrenaline_passive.raw_hdr
|       |   area_sonar_scan.raw_hdr
|       |   blank.raw_hdr
|       |   fx_phase_shift.raw_hdr
|       |   gas_cloud.raw_hdr
|       |   hueshift_example.raw_hdr
|       |   menu.raw_hdr
|       |   menu_test.raw_hdr
|       |   mp_canyonlands_hdr.raw_hdr
|       |   mp_canyonlands_night_hdr.raw_hdr
|       |   mp_rr_desertlands_hdr.raw_hdr
|       |   mp_rr_desertlands_mu1_hdr.raw_hdr
|       |   outside_ring.raw_hdr
|       |   shell_shock.raw_hdr
|       |   smoke_cloud.raw_hdr
|       |   UndoACESToe.raw_hdr
|       |   __master_color_grade.raw_hdr
|       |   
|       \---gcards
|           |   psplus04_rare_01.raw_hdr
|           |   
|           +---bangalore
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       psplus04_rare_01.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_epic_01.raw_hdr
|           |       
|           +---bloodhound
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       retail02_epic_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_event01_epic_01.raw_hdr
|           |       season02_rare_01.raw_hdr
|           |       
|           +---caustic
|           |       beasthunter_legendary_01.raw_hdr
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       psplus03_rare_01.raw_hdr
|           |       psplus04_rare_01.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_epic_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_rare_01.raw_hdr
|           |       
|           +---crypto
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_epic_01.raw_hdr
|           |       
|           +---gibraltar
|           |       beasthunter_legendary_01.raw_hdr
|           |       black_and_white.raw_hdr
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_epic_01.raw_hdr
|           |       season02_event02_legendary_01.raw_hdr
|           |       
|           +---lifeline
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       psplus02_rare_01.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       retail01_epic_01.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_rare_01.raw_hdr
|           |       
|           +---mirage
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       psplus02_rare_01.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_epic_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_rare_01.raw_hdr
|           |       
|           +---octane
|           |       beasthunter_legendary_01.raw_hdr
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_event01_epic_01.raw_hdr
|           |       season02_rare_01.raw_hdr
|           |       
|           +---pathfinder
|           |       beasthunter_legendary_01.raw_hdr
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_2019valentines.raw_hdr
|           |       psplus04_rare_01.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season01_rare_01.raw_hdr
|           |       season02_epic_01.raw_hdr
|           |       
|           +---template
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       
|           +---wattson
|           |       common_01.raw_hdr
|           |       common_02.raw_hdr
|           |       common_03.raw_hdr
|           |       common_04.raw_hdr
|           |       common_05.raw_hdr
|           |       legendary_01.raw_hdr
|           |       legendary_02.raw_hdr
|           |       legendary_03.raw_hdr
|           |       legendary_04.raw_hdr
|           |       legendary_05.raw_hdr
|           |       rare_01.raw_hdr
|           |       rare_02.raw_hdr
|           |       rare_03.raw_hdr
|           |       rare_04.raw_hdr
|           |       rare_05.raw_hdr
|           |       s03bp_rare_01.raw_hdr
|           |       season02_epic_01.raw_hdr
|           |       season02_event01_rare_01.raw_hdr
|           |       
|           \---wraith
|                   common_01.raw_hdr
|                   common_02.raw_hdr
|                   common_03.raw_hdr
|                   common_04.raw_hdr
|                   common_05.raw_hdr
|                   common_06.raw_hdr
|                   legendary_01.raw_hdr
|                   legendary_02.raw_hdr
|                   legendary_03.raw_hdr
|                   legendary_04.raw_hdr
|                   legendary_05.raw_hdr
|                   psplus03_rare_01.raw_hdr
|                   rare_01.raw_hdr
|                   rare_02.raw_hdr
|                   rare_03.raw_hdr
|                   rare_04.raw_hdr
|                   rare_05.raw_hdr
|                   s03bp_epic_01.raw_hdr
|                   season01_rare_01.raw_hdr
|                   season02_event02_legendary_01.raw_hdr
|                   season02_rare_01.raw_hdr
|                   
+---media
|   |   frontend.bik
|   |   intro.bik
|   |   intro_captions.txt
|   |   legal_english.bik
|   |   rating_GRAC.bik
|   |   respawn.bik
|   |   startupvids.txt
|   |   
|   +---battle_pass
|   |       havoc_skin_legendary_04.bik
|   |       havoc_skin_legendary_05.bik
|   |       peacekeeper_skin_s03bp_legendary_01.bik
|   |       peacekeeper_skin_s03bp_legendary_02.bik
|   |       r301_skin_season02_legendary_01.bik
|   |       r301_skin_season02_legendary_02.bik
|   |       season_rewards.bik
|   |       
|   +---character_executions
|   |       bangalore_execution_default.bik
|   |       bangalore_execution_weapon_steal.bik
|   |       bloodhound_execution_default.bik
|   |       bloodhound_execution_respect.bik
|   |       bloodhound_execution_the_raven.bik
|   |       caustic_execution_brutal.bik
|   |       caustic_execution_default.bik
|   |       caustic_execution_lastbreath.bik
|   |       crypto_execution_default.bik
|   |       crypto_execution_dronescan.bik
|   |       execution_downed_default.bik
|   |       gibraltar_execution_default.bik
|   |       gibraltar_execution_shield_bash.bik
|   |       gibraltar_execution_slam.bik
|   |       lifeline_execution_default.bik
|   |       lifeline_execution_drone_shock.bik
|   |       lifeline_execution_syringe.bik
|   |       mirage_execution_default.bik
|   |       mirage_execution_joker.bik
|   |       mirage_execution_trickster.bik
|   |       octane_execution_default.bik
|   |       octane_execution_facekick.bik
|   |       pathfinder_execution_default.bik
|   |       pathfinder_execution_highfive.bik
|   |       pathfinder_execution_punch.bik
|   |       wattson_execution_default.bik
|   |       wattson_execution_zap.bik
|   |       wraith_execution_default.bik
|   |       wraith_execution_ninja.bik
|   |       wraith_execution_portal.bik
|   |       
|   +---gamemodes
|   |       apex_elite.bik
|   |       duos.bik
|   |       generic_01.bik
|   |       generic_02.bik
|   |       hunt_the_beast.bik
|   |       play_apex.bik
|   |       ranked_1.bik
|   |       ranked_2.bik
|   |       shadow_squad.bik
|   |       shotguns_and_snipers.bik
|   |       solo_iron_crown.bik
|   |       training.bik
|   |       Worlds_Edge.bik
|   |       _temp.bik
|   |       
|   +---skydive_emotes
|   |       bangalore_backflip.bik
|   |       bloodhound_backflip.bik
|   |       caustic_backflip.bik
|   |       gibraltar_backflip.bik
|   |       gibraltar_surf.bik
|   |       lifeline_backflip.bik
|   |       lifeline_windmill.bik
|   |       mirage_backflip.bik
|   |       mirage_rodeo.bik
|   |       octane_backflip.bik
|   |       pathfinder_backflip.bik
|   |       pathfinder_birdattack.bik
|   |       wattson_backflip.bik
|   |       wattson_explorer.bik
|   |       wraith_backflip.bik
|   |       
|   +---skydive_trails
|   |       skydive_trail_default.bik
|   |       skydive_trail_rankedperiod_01_apex.bik
|   |       skydive_trail_rankedperiod_01_diamond.bik
|   |       
|   \---ultimate_states
|           ultimate_bloodhound_active.bik
|           ultimate_bloodhound_intro.bik
|           ultimate_bloodhound_ready.bik
|           ultimate_bloodhound_use.bik
|           
+---paks
|   \---Win64
|           common(01).rpak
|           common(02).rpak
|           common.rpak
|           common_early.rpak
|           common_mp(01).rpak
|           common_mp(02).rpak
|           common_mp.rpak
|           effects(01).rpak
|           effects.rpak
|           gcard_frame_bangalore_common_01.opt.starpak
|           gcard_frame_bangalore_common_01.rpak
|           gcard_frame_bangalore_common_01.starpak
|           gcard_frame_bangalore_common_02.opt.starpak
|           gcard_frame_bangalore_common_02.rpak
|           gcard_frame_bangalore_common_02.starpak
|           gcard_frame_bangalore_common_03.opt.starpak
|           gcard_frame_bangalore_common_03.rpak
|           gcard_frame_bangalore_common_03.starpak
|           gcard_frame_bangalore_common_04.opt.starpak
|           gcard_frame_bangalore_common_04.rpak
|           gcard_frame_bangalore_common_04.starpak
|           gcard_frame_bangalore_common_05.opt.starpak
|           gcard_frame_bangalore_common_05.rpak
|           gcard_frame_bangalore_common_05.starpak
|           gcard_frame_bangalore_legendary_01.opt.starpak
|           gcard_frame_bangalore_legendary_01.rpak
|           gcard_frame_bangalore_legendary_01.starpak
|           gcard_frame_bangalore_legendary_02.opt.starpak
|           gcard_frame_bangalore_legendary_02.rpak
|           gcard_frame_bangalore_legendary_02.starpak
|           gcard_frame_bangalore_legendary_03.opt.starpak
|           gcard_frame_bangalore_legendary_03.rpak
|           gcard_frame_bangalore_legendary_03.starpak
|           gcard_frame_bangalore_legendary_04.opt.starpak
|           gcard_frame_bangalore_legendary_04.rpak
|           gcard_frame_bangalore_legendary_04.starpak
|           gcard_frame_bangalore_legendary_05.opt.starpak
|           gcard_frame_bangalore_legendary_05.rpak
|           gcard_frame_bangalore_legendary_05.starpak
|           gcard_frame_bangalore_psplus04_rare_01.rpak
|           gcard_frame_bangalore_rare_01.opt.starpak
|           gcard_frame_bangalore_rare_01.rpak
|           gcard_frame_bangalore_rare_01.starpak
|           gcard_frame_bangalore_rare_02.opt.starpak
|           gcard_frame_bangalore_rare_02.rpak
|           gcard_frame_bangalore_rare_02.starpak
|           gcard_frame_bangalore_rare_03.opt.starpak
|           gcard_frame_bangalore_rare_03.rpak
|           gcard_frame_bangalore_rare_03.starpak
|           gcard_frame_bangalore_rare_04.opt.starpak
|           gcard_frame_bangalore_rare_04.rpak
|           gcard_frame_bangalore_rare_04.starpak
|           gcard_frame_bangalore_rare_05.opt.starpak
|           gcard_frame_bangalore_rare_05.rpak
|           gcard_frame_bangalore_rare_05.starpak
|           gcard_frame_bangalore_s03bp_rare_01.rpak
|           gcard_frame_bangalore_season01_rare_01.opt.starpak
|           gcard_frame_bangalore_season01_rare_01.rpak
|           gcard_frame_bangalore_season01_rare_01.starpak
|           gcard_frame_bangalore_season02_epic_01(01).rpak
|           gcard_frame_bangalore_season02_epic_01.rpak
|           gcard_frame_bloodhound_common_01.opt.starpak
|           gcard_frame_bloodhound_common_01.rpak
|           gcard_frame_bloodhound_common_01.starpak
|           gcard_frame_bloodhound_common_02.opt.starpak
|           gcard_frame_bloodhound_common_02.rpak
|           gcard_frame_bloodhound_common_02.starpak
|           gcard_frame_bloodhound_common_03.opt.starpak
|           gcard_frame_bloodhound_common_03.rpak
|           gcard_frame_bloodhound_common_03.starpak
|           gcard_frame_bloodhound_common_04.opt.starpak
|           gcard_frame_bloodhound_common_04.rpak
|           gcard_frame_bloodhound_common_04.starpak
|           gcard_frame_bloodhound_common_05.opt.starpak
|           gcard_frame_bloodhound_common_05.rpak
|           gcard_frame_bloodhound_common_05.starpak
|           gcard_frame_bloodhound_legendary_01.opt.starpak
|           gcard_frame_bloodhound_legendary_01.rpak
|           gcard_frame_bloodhound_legendary_01.starpak
|           gcard_frame_bloodhound_legendary_02.opt.starpak
|           gcard_frame_bloodhound_legendary_02.rpak
|           gcard_frame_bloodhound_legendary_02.starpak
|           gcard_frame_bloodhound_legendary_03.opt.starpak
|           gcard_frame_bloodhound_legendary_03.rpak
|           gcard_frame_bloodhound_legendary_03.starpak
|           gcard_frame_bloodhound_legendary_04.opt.starpak
|           gcard_frame_bloodhound_legendary_04.rpak
|           gcard_frame_bloodhound_legendary_04.starpak
|           gcard_frame_bloodhound_legendary_05.opt.starpak
|           gcard_frame_bloodhound_legendary_05.rpak
|           gcard_frame_bloodhound_legendary_05.starpak
|           gcard_frame_bloodhound_rare_01.opt.starpak
|           gcard_frame_bloodhound_rare_01.rpak
|           gcard_frame_bloodhound_rare_01.starpak
|           gcard_frame_bloodhound_rare_02.opt.starpak
|           gcard_frame_bloodhound_rare_02.rpak
|           gcard_frame_bloodhound_rare_02.starpak
|           gcard_frame_bloodhound_rare_03.opt.starpak
|           gcard_frame_bloodhound_rare_03.rpak
|           gcard_frame_bloodhound_rare_03.starpak
|           gcard_frame_bloodhound_rare_04.opt.starpak
|           gcard_frame_bloodhound_rare_04.rpak
|           gcard_frame_bloodhound_rare_04.starpak
|           gcard_frame_bloodhound_rare_05.opt.starpak
|           gcard_frame_bloodhound_rare_05.rpak
|           gcard_frame_bloodhound_rare_05.starpak
|           gcard_frame_bloodhound_rare_founder.opt.starpak
|           gcard_frame_bloodhound_rare_founder.rpak
|           gcard_frame_bloodhound_rare_founder.starpak
|           gcard_frame_bloodhound_rare_ps4.opt.starpak
|           gcard_frame_bloodhound_rare_ps4.rpak
|           gcard_frame_bloodhound_rare_ps4.starpak
|           gcard_frame_bloodhound_retail02_epic_01.rpak
|           gcard_frame_bloodhound_season01_rare_01.opt.starpak
|           gcard_frame_bloodhound_season01_rare_01.rpak
|           gcard_frame_bloodhound_season01_rare_01.starpak
|           gcard_frame_bloodhound_season02_event01_epic_01(02).opt.starpak
|           gcard_frame_bloodhound_season02_event01_epic_01(02).rpak
|           gcard_frame_bloodhound_season02_event01_epic_01(02).starpak
|           gcard_frame_bloodhound_season02_rare_01.opt.starpak
|           gcard_frame_bloodhound_season02_rare_01.rpak
|           gcard_frame_bloodhound_season02_rare_01.starpak
|           gcard_frame_caustic_beasthunter_legendary_01.opt.starpak
|           gcard_frame_caustic_beasthunter_legendary_01.rpak
|           gcard_frame_caustic_beasthunter_legendary_01.starpak
|           gcard_frame_caustic_common_01.opt.starpak
|           gcard_frame_caustic_common_01.rpak
|           gcard_frame_caustic_common_01.starpak
|           gcard_frame_caustic_common_02.opt.starpak
|           gcard_frame_caustic_common_02.rpak
|           gcard_frame_caustic_common_02.starpak
|           gcard_frame_caustic_common_03.opt.starpak
|           gcard_frame_caustic_common_03.rpak
|           gcard_frame_caustic_common_03.starpak
|           gcard_frame_caustic_common_04.opt.starpak
|           gcard_frame_caustic_common_04.rpak
|           gcard_frame_caustic_common_04.starpak
|           gcard_frame_caustic_common_05.opt.starpak
|           gcard_frame_caustic_common_05.rpak
|           gcard_frame_caustic_common_05.starpak
|           gcard_frame_caustic_legendary_01.opt.starpak
|           gcard_frame_caustic_legendary_01.rpak
|           gcard_frame_caustic_legendary_01.starpak
|           gcard_frame_caustic_legendary_02.opt.starpak
|           gcard_frame_caustic_legendary_02.rpak
|           gcard_frame_caustic_legendary_02.starpak
|           gcard_frame_caustic_legendary_03.opt.starpak
|           gcard_frame_caustic_legendary_03.rpak
|           gcard_frame_caustic_legendary_03.starpak
|           gcard_frame_caustic_legendary_04.opt.starpak
|           gcard_frame_caustic_legendary_04.rpak
|           gcard_frame_caustic_legendary_04.starpak
|           gcard_frame_caustic_legendary_05.opt.starpak
|           gcard_frame_caustic_legendary_05.rpak
|           gcard_frame_caustic_legendary_05.starpak
|           gcard_frame_caustic_psplus03_rare_01.opt.starpak
|           gcard_frame_caustic_psplus03_rare_01.rpak
|           gcard_frame_caustic_psplus03_rare_01.starpak
|           gcard_frame_caustic_rare_01.opt.starpak
|           gcard_frame_caustic_rare_01.rpak
|           gcard_frame_caustic_rare_01.starpak
|           gcard_frame_caustic_rare_02.opt.starpak
|           gcard_frame_caustic_rare_02.rpak
|           gcard_frame_caustic_rare_02.starpak
|           gcard_frame_caustic_rare_03.opt.starpak
|           gcard_frame_caustic_rare_03.rpak
|           gcard_frame_caustic_rare_03.starpak
|           gcard_frame_caustic_rare_04.opt.starpak
|           gcard_frame_caustic_rare_04.rpak
|           gcard_frame_caustic_rare_04.starpak
|           gcard_frame_caustic_rare_05.opt.starpak
|           gcard_frame_caustic_rare_05.rpak
|           gcard_frame_caustic_rare_05.starpak
|           gcard_frame_caustic_s03bp_epic_01.rpak
|           gcard_frame_caustic_season01_rare_01.opt.starpak
|           gcard_frame_caustic_season01_rare_01.rpak
|           gcard_frame_caustic_season01_rare_01.starpak
|           gcard_frame_caustic_season02_rare_01.opt.starpak
|           gcard_frame_caustic_season02_rare_01.rpak
|           gcard_frame_caustic_season02_rare_01.starpak
|           gcard_frame_crypto_common_01.rpak
|           gcard_frame_crypto_common_02.rpak
|           gcard_frame_crypto_common_03.rpak
|           gcard_frame_crypto_common_04.rpak
|           gcard_frame_crypto_common_05.rpak
|           gcard_frame_crypto_legendary_01.rpak
|           gcard_frame_crypto_legendary_02.rpak
|           gcard_frame_crypto_legendary_03.rpak
|           gcard_frame_crypto_legendary_04.rpak
|           gcard_frame_crypto_legendary_05.rpak
|           gcard_frame_crypto_rare_01.rpak
|           gcard_frame_crypto_rare_02.rpak
|           gcard_frame_crypto_rare_03.rpak
|           gcard_frame_crypto_rare_04.rpak
|           gcard_frame_crypto_rare_05.rpak
|           gcard_frame_crypto_s03bp_epic_01(01).rpak
|           gcard_frame_crypto_s03bp_epic_01.rpak
|           gcard_frame_gibraltar_beasthunter_legendary_01.opt.starpak
|           gcard_frame_gibraltar_beasthunter_legendary_01.rpak
|           gcard_frame_gibraltar_beasthunter_legendary_01.starpak
|           gcard_frame_gibraltar_common_01.opt.starpak
|           gcard_frame_gibraltar_common_01.rpak
|           gcard_frame_gibraltar_common_01.starpak
|           gcard_frame_gibraltar_common_02.opt.starpak
|           gcard_frame_gibraltar_common_02.rpak
|           gcard_frame_gibraltar_common_02.starpak
|           gcard_frame_gibraltar_common_03.opt.starpak
|           gcard_frame_gibraltar_common_03.rpak
|           gcard_frame_gibraltar_common_03.starpak
|           gcard_frame_gibraltar_common_04.opt.starpak
|           gcard_frame_gibraltar_common_04.rpak
|           gcard_frame_gibraltar_common_04.starpak
|           gcard_frame_gibraltar_common_05.opt.starpak
|           gcard_frame_gibraltar_common_05.rpak
|           gcard_frame_gibraltar_common_05.starpak
|           gcard_frame_gibraltar_legendary_01.opt.starpak
|           gcard_frame_gibraltar_legendary_01.rpak
|           gcard_frame_gibraltar_legendary_01.starpak
|           gcard_frame_gibraltar_legendary_02.opt.starpak
|           gcard_frame_gibraltar_legendary_02.rpak
|           gcard_frame_gibraltar_legendary_02.starpak
|           gcard_frame_gibraltar_legendary_03.opt.starpak
|           gcard_frame_gibraltar_legendary_03.rpak
|           gcard_frame_gibraltar_legendary_03.starpak
|           gcard_frame_gibraltar_legendary_04.opt.starpak
|           gcard_frame_gibraltar_legendary_04.rpak
|           gcard_frame_gibraltar_legendary_04.starpak
|           gcard_frame_gibraltar_legendary_05.opt.starpak
|           gcard_frame_gibraltar_legendary_05.rpak
|           gcard_frame_gibraltar_legendary_05.starpak
|           gcard_frame_gibraltar_rare_01.opt.starpak
|           gcard_frame_gibraltar_rare_01.rpak
|           gcard_frame_gibraltar_rare_01.starpak
|           gcard_frame_gibraltar_rare_02.opt.starpak
|           gcard_frame_gibraltar_rare_02.rpak
|           gcard_frame_gibraltar_rare_02.starpak
|           gcard_frame_gibraltar_rare_03.opt.starpak
|           gcard_frame_gibraltar_rare_03.rpak
|           gcard_frame_gibraltar_rare_03.starpak
|           gcard_frame_gibraltar_rare_04.opt.starpak
|           gcard_frame_gibraltar_rare_04.rpak
|           gcard_frame_gibraltar_rare_04.starpak
|           gcard_frame_gibraltar_rare_05.opt.starpak
|           gcard_frame_gibraltar_rare_05.rpak
|           gcard_frame_gibraltar_rare_05.starpak
|           gcard_frame_gibraltar_rare_founder.opt.starpak
|           gcard_frame_gibraltar_rare_founder.rpak
|           gcard_frame_gibraltar_rare_founder.starpak
|           gcard_frame_gibraltar_rare_ps4.opt.starpak
|           gcard_frame_gibraltar_rare_ps4.rpak
|           gcard_frame_gibraltar_rare_ps4.starpak
|           gcard_frame_gibraltar_s03bp_rare_01.rpak
|           gcard_frame_gibraltar_season01_rare_01.opt.starpak
|           gcard_frame_gibraltar_season01_rare_01.rpak
|           gcard_frame_gibraltar_season01_rare_01.starpak
|           gcard_frame_gibraltar_season02_epic_01(02).opt.starpak
|           gcard_frame_gibraltar_season02_epic_01(02).rpak
|           gcard_frame_gibraltar_season02_epic_01(02).starpak
|           gcard_frame_gibraltar_season02_epic_01.opt.starpak
|           gcard_frame_gibraltar_season02_epic_01.rpak
|           gcard_frame_gibraltar_season02_epic_01.starpak
|           gcard_frame_gibraltar_season02_event02_legendary_01.rpak
|           gcard_frame_lifeline_common_01.opt.starpak
|           gcard_frame_lifeline_common_01.rpak
|           gcard_frame_lifeline_common_01.starpak
|           gcard_frame_lifeline_common_02.opt.starpak
|           gcard_frame_lifeline_common_02.rpak
|           gcard_frame_lifeline_common_02.starpak
|           gcard_frame_lifeline_common_03.opt.starpak
|           gcard_frame_lifeline_common_03.rpak
|           gcard_frame_lifeline_common_03.starpak
|           gcard_frame_lifeline_common_04.opt.starpak
|           gcard_frame_lifeline_common_04.rpak
|           gcard_frame_lifeline_common_04.starpak
|           gcard_frame_lifeline_common_05.opt.starpak
|           gcard_frame_lifeline_common_05.rpak
|           gcard_frame_lifeline_common_05.starpak
|           gcard_frame_lifeline_legendary_01.opt.starpak
|           gcard_frame_lifeline_legendary_01.rpak
|           gcard_frame_lifeline_legendary_01.starpak
|           gcard_frame_lifeline_legendary_02.opt.starpak
|           gcard_frame_lifeline_legendary_02.rpak
|           gcard_frame_lifeline_legendary_02.starpak
|           gcard_frame_lifeline_legendary_03.opt.starpak
|           gcard_frame_lifeline_legendary_03.rpak
|           gcard_frame_lifeline_legendary_03.starpak
|           gcard_frame_lifeline_legendary_04.opt.starpak
|           gcard_frame_lifeline_legendary_04.rpak
|           gcard_frame_lifeline_legendary_04.starpak
|           gcard_frame_lifeline_legendary_05.opt.starpak
|           gcard_frame_lifeline_legendary_05.rpak
|           gcard_frame_lifeline_legendary_05.starpak
|           gcard_frame_lifeline_psplus02_rare_01.opt.starpak
|           gcard_frame_lifeline_psplus02_rare_01.rpak
|           gcard_frame_lifeline_psplus02_rare_01.starpak
|           gcard_frame_lifeline_rare_01.opt.starpak
|           gcard_frame_lifeline_rare_01.rpak
|           gcard_frame_lifeline_rare_01.starpak
|           gcard_frame_lifeline_rare_02.opt.starpak
|           gcard_frame_lifeline_rare_02.rpak
|           gcard_frame_lifeline_rare_02.starpak
|           gcard_frame_lifeline_rare_03.opt.starpak
|           gcard_frame_lifeline_rare_03.rpak
|           gcard_frame_lifeline_rare_03.starpak
|           gcard_frame_lifeline_rare_04.opt.starpak
|           gcard_frame_lifeline_rare_04.rpak
|           gcard_frame_lifeline_rare_04.starpak
|           gcard_frame_lifeline_rare_05.opt.starpak
|           gcard_frame_lifeline_rare_05.rpak
|           gcard_frame_lifeline_rare_05.starpak
|           gcard_frame_lifeline_retail01_epic_01.rpak
|           gcard_frame_lifeline_s03bp_rare_01.rpak
|           gcard_frame_lifeline_season01_rare_01.opt.starpak
|           gcard_frame_lifeline_season01_rare_01.rpak
|           gcard_frame_lifeline_season01_rare_01.starpak
|           gcard_frame_lifeline_season02_rare_01.opt.starpak
|           gcard_frame_lifeline_season02_rare_01.rpak
|           gcard_frame_lifeline_season02_rare_01.starpak
|           gcard_frame_mirage_common_01.opt.starpak
|           gcard_frame_mirage_common_01.rpak
|           gcard_frame_mirage_common_01.starpak
|           gcard_frame_mirage_common_02.opt.starpak
|           gcard_frame_mirage_common_02.rpak
|           gcard_frame_mirage_common_02.starpak
|           gcard_frame_mirage_common_03.opt.starpak
|           gcard_frame_mirage_common_03.rpak
|           gcard_frame_mirage_common_03.starpak
|           gcard_frame_mirage_common_04.opt.starpak
|           gcard_frame_mirage_common_04.rpak
|           gcard_frame_mirage_common_04.starpak
|           gcard_frame_mirage_common_05.opt.starpak
|           gcard_frame_mirage_common_05.rpak
|           gcard_frame_mirage_common_05.starpak
|           gcard_frame_mirage_legendary_01.opt.starpak
|           gcard_frame_mirage_legendary_01.rpak
|           gcard_frame_mirage_legendary_01.starpak
|           gcard_frame_mirage_legendary_02.opt.starpak
|           gcard_frame_mirage_legendary_02.rpak
|           gcard_frame_mirage_legendary_02.starpak
|           gcard_frame_mirage_legendary_03.opt.starpak
|           gcard_frame_mirage_legendary_03.rpak
|           gcard_frame_mirage_legendary_03.starpak
|           gcard_frame_mirage_legendary_04.opt.starpak
|           gcard_frame_mirage_legendary_04.rpak
|           gcard_frame_mirage_legendary_04.starpak
|           gcard_frame_mirage_legendary_05.opt.starpak
|           gcard_frame_mirage_legendary_05.rpak
|           gcard_frame_mirage_legendary_05.starpak
|           gcard_frame_mirage_psplus02_rare_01.opt.starpak
|           gcard_frame_mirage_psplus02_rare_01.rpak
|           gcard_frame_mirage_psplus02_rare_01.starpak
|           gcard_frame_mirage_rare_01.opt.starpak
|           gcard_frame_mirage_rare_01.rpak
|           gcard_frame_mirage_rare_01.starpak
|           gcard_frame_mirage_rare_02.opt.starpak
|           gcard_frame_mirage_rare_02.rpak
|           gcard_frame_mirage_rare_02.starpak
|           gcard_frame_mirage_rare_03.opt.starpak
|           gcard_frame_mirage_rare_03.rpak
|           gcard_frame_mirage_rare_03.starpak
|           gcard_frame_mirage_rare_04.opt.starpak
|           gcard_frame_mirage_rare_04.rpak
|           gcard_frame_mirage_rare_04.starpak
|           gcard_frame_mirage_rare_05.opt.starpak
|           gcard_frame_mirage_rare_05.rpak
|           gcard_frame_mirage_rare_05.starpak
|           gcard_frame_mirage_s03bp_epic_01.rpak
|           gcard_frame_mirage_season01_rare_01.opt.starpak
|           gcard_frame_mirage_season01_rare_01.rpak
|           gcard_frame_mirage_season01_rare_01.starpak
|           gcard_frame_mirage_season02_rare_01.opt.starpak
|           gcard_frame_mirage_season02_rare_01.rpak
|           gcard_frame_mirage_season02_rare_01.starpak
|           gcard_frame_octane_beasthunter_legendary_01.opt.starpak
|           gcard_frame_octane_beasthunter_legendary_01.rpak
|           gcard_frame_octane_beasthunter_legendary_01.starpak
|           gcard_frame_octane_common_01.opt.starpak
|           gcard_frame_octane_common_01.rpak
|           gcard_frame_octane_common_01.starpak
|           gcard_frame_octane_common_02.opt.starpak
|           gcard_frame_octane_common_02.rpak
|           gcard_frame_octane_common_02.starpak
|           gcard_frame_octane_common_03.opt.starpak
|           gcard_frame_octane_common_03.rpak
|           gcard_frame_octane_common_03.starpak
|           gcard_frame_octane_common_04.opt.starpak
|           gcard_frame_octane_common_04.rpak
|           gcard_frame_octane_common_04.starpak
|           gcard_frame_octane_common_05.opt.starpak
|           gcard_frame_octane_common_05.rpak
|           gcard_frame_octane_common_05.starpak
|           gcard_frame_octane_legendary_01.opt.starpak
|           gcard_frame_octane_legendary_01.rpak
|           gcard_frame_octane_legendary_01.starpak
|           gcard_frame_octane_legendary_02.opt.starpak
|           gcard_frame_octane_legendary_02.rpak
|           gcard_frame_octane_legendary_02.starpak
|           gcard_frame_octane_legendary_03.opt.starpak
|           gcard_frame_octane_legendary_03.rpak
|           gcard_frame_octane_legendary_03.starpak
|           gcard_frame_octane_legendary_04.opt.starpak
|           gcard_frame_octane_legendary_04.rpak
|           gcard_frame_octane_legendary_04.starpak
|           gcard_frame_octane_legendary_05.opt.starpak
|           gcard_frame_octane_legendary_05.rpak
|           gcard_frame_octane_legendary_05.starpak
|           gcard_frame_octane_rare_01.opt.starpak
|           gcard_frame_octane_rare_01.rpak
|           gcard_frame_octane_rare_01.starpak
|           gcard_frame_octane_rare_02.opt.starpak
|           gcard_frame_octane_rare_02.rpak
|           gcard_frame_octane_rare_02.starpak
|           gcard_frame_octane_rare_03.opt.starpak
|           gcard_frame_octane_rare_03.rpak
|           gcard_frame_octane_rare_03.starpak
|           gcard_frame_octane_rare_04.opt.starpak
|           gcard_frame_octane_rare_04.rpak
|           gcard_frame_octane_rare_04.starpak
|           gcard_frame_octane_rare_05.opt.starpak
|           gcard_frame_octane_rare_05.rpak
|           gcard_frame_octane_rare_05.starpak
|           gcard_frame_octane_s03bp_rare_01.rpak
|           gcard_frame_octane_season01_rare_01.opt.starpak
|           gcard_frame_octane_season01_rare_01.rpak
|           gcard_frame_octane_season01_rare_01.starpak
|           gcard_frame_octane_season02_event01_epic_01(02).opt.starpak
|           gcard_frame_octane_season02_event01_epic_01(02).rpak
|           gcard_frame_octane_season02_event01_epic_01(02).starpak
|           gcard_frame_octane_season02_rare_01.opt.starpak
|           gcard_frame_octane_season02_rare_01.rpak
|           gcard_frame_octane_season02_rare_01.starpak
|           gcard_frame_pathfinder_beasthunter_legendary_01.opt.starpak
|           gcard_frame_pathfinder_beasthunter_legendary_01.rpak
|           gcard_frame_pathfinder_beasthunter_legendary_01.starpak
|           gcard_frame_pathfinder_common_01.opt.starpak
|           gcard_frame_pathfinder_common_01.rpak
|           gcard_frame_pathfinder_common_01.starpak
|           gcard_frame_pathfinder_common_02.opt.starpak
|           gcard_frame_pathfinder_common_02.rpak
|           gcard_frame_pathfinder_common_02.starpak
|           gcard_frame_pathfinder_common_03.opt.starpak
|           gcard_frame_pathfinder_common_03.rpak
|           gcard_frame_pathfinder_common_03.starpak
|           gcard_frame_pathfinder_common_04.opt.starpak
|           gcard_frame_pathfinder_common_04.rpak
|           gcard_frame_pathfinder_common_04.starpak
|           gcard_frame_pathfinder_common_05.opt.starpak
|           gcard_frame_pathfinder_common_05.rpak
|           gcard_frame_pathfinder_common_05.starpak
|           gcard_frame_pathfinder_legendary_01.opt.starpak
|           gcard_frame_pathfinder_legendary_01.rpak
|           gcard_frame_pathfinder_legendary_01.starpak
|           gcard_frame_pathfinder_legendary_02.opt.starpak
|           gcard_frame_pathfinder_legendary_02.rpak
|           gcard_frame_pathfinder_legendary_02.starpak
|           gcard_frame_pathfinder_legendary_03.opt.starpak
|           gcard_frame_pathfinder_legendary_03.rpak
|           gcard_frame_pathfinder_legendary_03.starpak
|           gcard_frame_pathfinder_legendary_04.opt.starpak
|           gcard_frame_pathfinder_legendary_04.rpak
|           gcard_frame_pathfinder_legendary_04.starpak
|           gcard_frame_pathfinder_legendary_05.opt.starpak
|           gcard_frame_pathfinder_legendary_05.rpak
|           gcard_frame_pathfinder_legendary_05.starpak
|           gcard_frame_pathfinder_legendary_2019valentines.opt.starpak
|           gcard_frame_pathfinder_legendary_2019valentines.rpak
|           gcard_frame_pathfinder_legendary_2019valentines.starpak
|           gcard_frame_pathfinder_psplus04_rare_01.rpak
|           gcard_frame_pathfinder_rare_01.opt.starpak
|           gcard_frame_pathfinder_rare_01.rpak
|           gcard_frame_pathfinder_rare_01.starpak
|           gcard_frame_pathfinder_rare_02.opt.starpak
|           gcard_frame_pathfinder_rare_02.rpak
|           gcard_frame_pathfinder_rare_02.starpak
|           gcard_frame_pathfinder_rare_03.opt.starpak
|           gcard_frame_pathfinder_rare_03.rpak
|           gcard_frame_pathfinder_rare_03.starpak
|           gcard_frame_pathfinder_rare_04.opt.starpak
|           gcard_frame_pathfinder_rare_04.rpak
|           gcard_frame_pathfinder_rare_04.starpak
|           gcard_frame_pathfinder_rare_05.opt.starpak
|           gcard_frame_pathfinder_rare_05.rpak
|           gcard_frame_pathfinder_rare_05.starpak
|           gcard_frame_pathfinder_s03bp_rare_01.rpak
|           gcard_frame_pathfinder_season01_rare_01.opt.starpak
|           gcard_frame_pathfinder_season01_rare_01.rpak
|           gcard_frame_pathfinder_season01_rare_01.starpak
|           gcard_frame_pathfinder_season02_epic_01(02).opt.starpak
|           gcard_frame_pathfinder_season02_epic_01(02).rpak
|           gcard_frame_pathfinder_season02_epic_01(02).starpak
|           gcard_frame_pathfinder_season02_epic_01.opt.starpak
|           gcard_frame_pathfinder_season02_epic_01.rpak
|           gcard_frame_pathfinder_season02_epic_01.starpak
|           gcard_frame_template_common_01.opt.starpak
|           gcard_frame_template_common_01.rpak
|           gcard_frame_template_common_01.starpak
|           gcard_frame_template_common_02.opt.starpak
|           gcard_frame_template_common_02.rpak
|           gcard_frame_template_common_02.starpak
|           gcard_frame_template_common_03.opt.starpak
|           gcard_frame_template_common_03.rpak
|           gcard_frame_template_common_03.starpak
|           gcard_frame_template_common_04.opt.starpak
|           gcard_frame_template_common_04.rpak
|           gcard_frame_template_common_04.starpak
|           gcard_frame_template_common_05.opt.starpak
|           gcard_frame_template_common_05.rpak
|           gcard_frame_template_common_05.starpak
|           gcard_frame_template_legendary_01(01).rpak
|           gcard_frame_template_legendary_01.rpak
|           gcard_frame_template_legendary_02(01).rpak
|           gcard_frame_template_legendary_02.rpak
|           gcard_frame_template_legendary_03(01).rpak
|           gcard_frame_template_legendary_03.rpak
|           gcard_frame_template_legendary_04(01).rpak
|           gcard_frame_template_legendary_04.rpak
|           gcard_frame_template_legendary_05(01).rpak
|           gcard_frame_template_legendary_05.rpak
|           gcard_frame_template_rare_01(01).rpak
|           gcard_frame_template_rare_01.rpak
|           gcard_frame_template_rare_02(01).rpak
|           gcard_frame_template_rare_02.rpak
|           gcard_frame_template_rare_03(01).rpak
|           gcard_frame_template_rare_03.rpak
|           gcard_frame_template_rare_04(01).rpak
|           gcard_frame_template_rare_04.rpak
|           gcard_frame_template_rare_05(01).rpak
|           gcard_frame_template_rare_05.rpak
|           gcard_frame_wattson_common_01.opt.starpak
|           gcard_frame_wattson_common_01.rpak
|           gcard_frame_wattson_common_01.starpak
|           gcard_frame_wattson_common_02.opt.starpak
|           gcard_frame_wattson_common_02.rpak
|           gcard_frame_wattson_common_02.starpak
|           gcard_frame_wattson_common_03.opt.starpak
|           gcard_frame_wattson_common_03.rpak
|           gcard_frame_wattson_common_03.starpak
|           gcard_frame_wattson_common_04.opt.starpak
|           gcard_frame_wattson_common_04.rpak
|           gcard_frame_wattson_common_04.starpak
|           gcard_frame_wattson_common_05.opt.starpak
|           gcard_frame_wattson_common_05.rpak
|           gcard_frame_wattson_common_05.starpak
|           gcard_frame_wattson_legendary_01.opt.starpak
|           gcard_frame_wattson_legendary_01.rpak
|           gcard_frame_wattson_legendary_01.starpak
|           gcard_frame_wattson_legendary_02.opt.starpak
|           gcard_frame_wattson_legendary_02.rpak
|           gcard_frame_wattson_legendary_02.starpak
|           gcard_frame_wattson_legendary_03.opt.starpak
|           gcard_frame_wattson_legendary_03.rpak
|           gcard_frame_wattson_legendary_03.starpak
|           gcard_frame_wattson_legendary_04.opt.starpak
|           gcard_frame_wattson_legendary_04.rpak
|           gcard_frame_wattson_legendary_04.starpak
|           gcard_frame_wattson_legendary_05.opt.starpak
|           gcard_frame_wattson_legendary_05.rpak
|           gcard_frame_wattson_legendary_05.starpak
|           gcard_frame_wattson_rare_01.opt.starpak
|           gcard_frame_wattson_rare_01.rpak
|           gcard_frame_wattson_rare_01.starpak
|           gcard_frame_wattson_rare_02.opt.starpak
|           gcard_frame_wattson_rare_02.rpak
|           gcard_frame_wattson_rare_02.starpak
|           gcard_frame_wattson_rare_03.opt.starpak
|           gcard_frame_wattson_rare_03.rpak
|           gcard_frame_wattson_rare_03.starpak
|           gcard_frame_wattson_rare_04.opt.starpak
|           gcard_frame_wattson_rare_04.rpak
|           gcard_frame_wattson_rare_04.starpak
|           gcard_frame_wattson_rare_05.opt.starpak
|           gcard_frame_wattson_rare_05.rpak
|           gcard_frame_wattson_rare_05.starpak
|           gcard_frame_wattson_s03bp_rare_01.rpak
|           gcard_frame_wattson_season02_epic_01(02).opt.starpak
|           gcard_frame_wattson_season02_epic_01(02).rpak
|           gcard_frame_wattson_season02_epic_01(02).starpak
|           gcard_frame_wattson_season02_epic_01.opt.starpak
|           gcard_frame_wattson_season02_epic_01.rpak
|           gcard_frame_wattson_season02_epic_01.starpak
|           gcard_frame_wattson_season02_event01_rare_01(02).rpak
|           gcard_frame_wattson_season02_event01_rare_01.rpak
|           gcard_frame_wraith_common_01.opt.starpak
|           gcard_frame_wraith_common_01.rpak
|           gcard_frame_wraith_common_01.starpak
|           gcard_frame_wraith_common_02.opt.starpak
|           gcard_frame_wraith_common_02.rpak
|           gcard_frame_wraith_common_02.starpak
|           gcard_frame_wraith_common_03.opt.starpak
|           gcard_frame_wraith_common_03.rpak
|           gcard_frame_wraith_common_03.starpak
|           gcard_frame_wraith_common_04.opt.starpak
|           gcard_frame_wraith_common_04.rpak
|           gcard_frame_wraith_common_04.starpak
|           gcard_frame_wraith_common_05.opt.starpak
|           gcard_frame_wraith_common_05.rpak
|           gcard_frame_wraith_common_05.starpak
|           gcard_frame_wraith_legendary_01.opt.starpak
|           gcard_frame_wraith_legendary_01.rpak
|           gcard_frame_wraith_legendary_01.starpak
|           gcard_frame_wraith_legendary_02.opt.starpak
|           gcard_frame_wraith_legendary_02.rpak
|           gcard_frame_wraith_legendary_02.starpak
|           gcard_frame_wraith_legendary_03.opt.starpak
|           gcard_frame_wraith_legendary_03.rpak
|           gcard_frame_wraith_legendary_03.starpak
|           gcard_frame_wraith_legendary_04.opt.starpak
|           gcard_frame_wraith_legendary_04.rpak
|           gcard_frame_wraith_legendary_04.starpak
|           gcard_frame_wraith_legendary_05.opt.starpak
|           gcard_frame_wraith_legendary_05.rpak
|           gcard_frame_wraith_legendary_05.starpak
|           gcard_frame_wraith_psplus03_rare_01(01).rpak
|           gcard_frame_wraith_psplus03_rare_01.rpak
|           gcard_frame_wraith_rare_01.opt.starpak
|           gcard_frame_wraith_rare_01.rpak
|           gcard_frame_wraith_rare_01.starpak
|           gcard_frame_wraith_rare_02.opt.starpak
|           gcard_frame_wraith_rare_02.rpak
|           gcard_frame_wraith_rare_02.starpak
|           gcard_frame_wraith_rare_03.opt.starpak
|           gcard_frame_wraith_rare_03.rpak
|           gcard_frame_wraith_rare_03.starpak
|           gcard_frame_wraith_rare_04.opt.starpak
|           gcard_frame_wraith_rare_04.rpak
|           gcard_frame_wraith_rare_04.starpak
|           gcard_frame_wraith_rare_05.opt.starpak
|           gcard_frame_wraith_rare_05.rpak
|           gcard_frame_wraith_rare_05.starpak
|           gcard_frame_wraith_rare_founder.opt.starpak
|           gcard_frame_wraith_rare_founder.rpak
|           gcard_frame_wraith_rare_founder.starpak
|           gcard_frame_wraith_s03bp_epic_01.rpak
|           gcard_frame_wraith_season01_rare_01.opt.starpak
|           gcard_frame_wraith_season01_rare_01.rpak
|           gcard_frame_wraith_season01_rare_01.starpak
|           gcard_frame_wraith_season02_event02_legendary_01.rpak
|           gcard_frame_wraith_season02_rare_01.opt.starpak
|           gcard_frame_wraith_season02_rare_01.rpak
|           gcard_frame_wraith_season02_rare_01.starpak
|           loadscreen_custom_01(02).opt.starpak
|           loadscreen_custom_01(02).rpak
|           loadscreen_custom_01(02).starpak
|           loadscreen_custom_01.opt.starpak
|           loadscreen_custom_01.rpak
|           loadscreen_custom_01.starpak
|           loadscreen_custom_02(02).opt.starpak
|           loadscreen_custom_02(02).rpak
|           loadscreen_custom_02(02).starpak
|           loadscreen_custom_02.opt.starpak
|           loadscreen_custom_02.rpak
|           loadscreen_custom_02.starpak
|           loadscreen_custom_03(02).opt.starpak
|           loadscreen_custom_03(02).rpak
|           loadscreen_custom_03(02).starpak
|           loadscreen_custom_03.opt.starpak
|           loadscreen_custom_03.rpak
|           loadscreen_custom_03.starpak
|           loadscreen_custom_04(02).opt.starpak
|           loadscreen_custom_04(02).rpak
|           loadscreen_custom_04(02).starpak
|           loadscreen_custom_04.opt.starpak
|           loadscreen_custom_04.rpak
|           loadscreen_custom_04.starpak
|           loadscreen_custom_05(02).opt.starpak
|           loadscreen_custom_05(02).rpak
|           loadscreen_custom_05(02).starpak
|           loadscreen_custom_05.opt.starpak
|           loadscreen_custom_05.rpak
|           loadscreen_custom_05.starpak
|           loadscreen_custom_06(02).opt.starpak
|           loadscreen_custom_06(02).rpak
|           loadscreen_custom_06(02).starpak
|           loadscreen_custom_06.opt.starpak
|           loadscreen_custom_06.rpak
|           loadscreen_custom_06.starpak
|           loadscreen_custom_07(02).opt.starpak
|           loadscreen_custom_07(02).rpak
|           loadscreen_custom_07(02).starpak
|           loadscreen_custom_07.opt.starpak
|           loadscreen_custom_07.rpak
|           loadscreen_custom_07.starpak
|           loadscreen_custom_08(02).opt.starpak
|           loadscreen_custom_08(02).rpak
|           loadscreen_custom_08(02).starpak
|           loadscreen_custom_08.opt.starpak
|           loadscreen_custom_08.rpak
|           loadscreen_custom_08.starpak
|           loadscreen_custom_09(02).opt.starpak
|           loadscreen_custom_09(02).rpak
|           loadscreen_custom_09(02).starpak
|           loadscreen_custom_09.opt.starpak
|           loadscreen_custom_09.rpak
|           loadscreen_custom_09.starpak
|           loadscreen_custom_10(02).opt.starpak
|           loadscreen_custom_10(02).rpak
|           loadscreen_custom_10(02).starpak
|           loadscreen_custom_10.opt.starpak
|           loadscreen_custom_10.rpak
|           loadscreen_custom_10.starpak
|           loadscreen_custom_11.rpak
|           loadscreen_custom_kings_canyon_mu1(02).opt.starpak
|           loadscreen_custom_kings_canyon_mu1(02).rpak
|           loadscreen_custom_kings_canyon_mu1(02).starpak
|           loadscreen_custom_kings_canyon_mu1.opt.starpak
|           loadscreen_custom_kings_canyon_mu1.rpak
|           loadscreen_custom_kings_canyon_mu1.starpak
|           loadscreen_custom_kings_canyon_og(02).opt.starpak
|           loadscreen_custom_kings_canyon_og(02).rpak
|           loadscreen_custom_kings_canyon_og(02).starpak
|           loadscreen_custom_kings_canyon_og.opt.starpak
|           loadscreen_custom_kings_canyon_og.rpak
|           loadscreen_custom_kings_canyon_og.starpak
|           loadscreen_custom_s03bp_01.rpak
|           loadscreen_custom_s03bp_02.rpak
|           loadscreen_custom_s03bp_03.rpak
|           loadscreen_custom_s03bp_04.rpak
|           loadscreen_custom_s03bp_05.rpak
|           loadscreen_custom_s03bp_06(01).rpak
|           loadscreen_custom_s03bp_06.rpak
|           loadscreen_custom_s03bp_07.rpak
|           loadscreen_custom_s03bp_08.rpak
|           loadscreen_custom_s03bp_09.rpak
|           loadscreen_custom_s03bp_10(01).rpak
|           loadscreen_custom_s03bp_10.rpak
|           loadscreen_custom_s03bp_11.rpak
|           loadscreen_custom_worlds_end_og.rpak
|           lobby.rpak
|           mp_lobby.rpak
|           mp_lobby_loadscreen.rpak
|           mp_rr_canyonlands_64k_x_64k(01).rpak
|           mp_rr_canyonlands_64k_x_64k(02).rpak
|           mp_rr_canyonlands_64k_x_64k.rpak
|           mp_rr_canyonlands_64k_x_64k_loadscreen.rpak
|           mp_rr_canyonlands_mu1(01).rpak
|           mp_rr_canyonlands_mu1(02).rpak
|           mp_rr_canyonlands_mu1.rpak
|           mp_rr_canyonlands_mu1_loadscreen.rpak
|           mp_rr_canyonlands_mu1_night.rpak
|           mp_rr_canyonlands_mu1_night_loadscreen.rpak
|           mp_rr_canyonlands_staging(01).rpak
|           mp_rr_canyonlands_staging(02).rpak
|           mp_rr_canyonlands_staging.rpak
|           mp_rr_canyonlands_staging_loadscreen.rpak
|           mp_rr_desertlands_64k_x_64k.rpak
|           mp_rr_desertlands_64k_x_64k_loadscreen.rpak
|           patch_master.rpak
|           pc_all(01).opt.starpak
|           pc_all(01).starpak
|           pc_all(02).opt.starpak
|           pc_all(02).starpak
|           pc_all(03).opt.starpak
|           pc_all(03).starpak
|           pc_all.opt.starpak
|           pc_all.starpak
|           startup.rpak
|           subtitles_english(01).rpak
|           subtitles_english(02).opt.starpak
|           subtitles_english.opt.starpak
|           subtitles_english.rpak
|           subtitles_english.starpak
|           subtitles_french(01).rpak
|           subtitles_french.rpak
|           subtitles_german(01).rpak
|           subtitles_german.rpak
|           subtitles_italian(01).rpak
|           subtitles_italian.rpak
|           subtitles_japanese(03).rpak
|           subtitles_japanese.rpak
|           subtitles_korean(01).rpak
|           subtitles_korean.rpak
|           subtitles_polish(01).rpak
|           subtitles_polish.rpak
|           subtitles_portuguese(03).rpak
|           subtitles_portuguese.rpak
|           subtitles_russian(01).rpak
|           subtitles_russian.rpak
|           subtitles_schinese(01).rpak
|           subtitles_schinese.rpak
|           subtitles_spanish(01).rpak
|           subtitles_spanish.rpak
|           subtitles_tchinese(01).rpak
|           subtitles_tchinese.rpak
|           ui.dll
|           ui.rpak
|           ui_mp.rpak
|           
+---platform
|   |   imgui.ini
|   |   playlists_r5_patch.txt
|   |   
|   +---cfg
|   |   |   autoexec.cfg
|   |   |   autoexec_dev.cfg
|   |   |   autoexec_rexx.cfg
|   |   |   connect5_dev.cfg
|   |   |   netchan.cfg
|   |   |   noclip.cfg
|   |   |   noclip_disable.cfg
|   |   |   noclip_enable.cfg
|   |   |   startup_debug.cfg
|   |   |   startup_dedi.cfg
|   |   |   startup_retail.cfg
|   |   |   
|   |   +---aimassist
|   |   |       adspull_classes.txt
|   |   |       
|   |   \---levels
|   |           mp_lobby.cfg
|   |           mp_rr_canyonlands_64k_x_64k.cfg
|   |           
|   +---log
|   |       default_r5.log
|   |       netchan_r5.log
|   |       
|   +---maps
|   |   |   mp_rr_desertlands_64k_x_64k_env.ent
|   |   |   mp_rr_desertlands_64k_x_64k_fx.ent
|   |   |   mp_rr_desertlands_64k_x_64k_script.ent
|   |   |   mp_rr_desertlands_64k_x_64k_snd.ent
|   |   |   mp_rr_desertlands_64k_x_64k_spawn.ent
|   |   |   
|   |   +---graphs
|   |   |       mp_lobby.ain
|   |   |       mp_rr_canyonlands_64k_x_64k.ain
|   |   |       mp_rr_canyonlands_mu1.ain
|   |   |       mp_rr_canyonlands_mu1_night.ain
|   |   |       mp_rr_canyonlands_staging.ain
|   |   |       mp_rr_desertlands_64k_x_64k.ain
|   |   |       mp_rr_desertlands_64k_x_64k_nx.ain
|   |   |       
|   |   \---navmesh
|   |           mp_lobby_extra_large.nm
|   |           mp_lobby_large.nm
|   |           mp_lobby_medium.nm
|   |           mp_lobby_med_short.nm
|   |           mp_lobby_small.nm
|   |           mp_rr_canyonlands_64k_x_64k_extra_large.nm
|   |           mp_rr_canyonlands_64k_x_64k_large.nm
|   |           mp_rr_canyonlands_64k_x_64k_medium.nm
|   |           mp_rr_canyonlands_64k_x_64k_med_short.nm
|   |           mp_rr_canyonlands_64k_x_64k_small.nm
|   |           mp_rr_canyonlands_mu1_extra_large.nm
|   |           mp_rr_canyonlands_mu1_large.nm
|   |           mp_rr_canyonlands_mu1_medium.nm
|   |           mp_rr_canyonlands_mu1_med_short.nm
|   |           mp_rr_canyonlands_mu1_night_extra_large.nm
|   |           mp_rr_canyonlands_mu1_night_large.nm
|   |           mp_rr_canyonlands_mu1_night_medium.nm
|   |           mp_rr_canyonlands_mu1_night_med_short.nm
|   |           mp_rr_canyonlands_mu1_night_small.nm
|   |           mp_rr_canyonlands_mu1_small.nm
|   |           mp_rr_canyonlands_staging_extra_large.nm
|   |           mp_rr_canyonlands_staging_large.nm
|   |           mp_rr_canyonlands_staging_medium.nm
|   |           mp_rr_canyonlands_staging_med_short.nm
|   |           mp_rr_canyonlands_staging_small.nm
|   |           mp_rr_desertlands_64k_x_64k_extra_large.nm
|   |           mp_rr_desertlands_64k_x_64k_large.nm
|   |           mp_rr_desertlands_64k_x_64k_medium.nm
|   |           mp_rr_desertlands_64k_x_64k_med_short.nm
|   |           mp_rr_desertlands_64k_x_64k_nx_extra_large.nm
|   |           mp_rr_desertlands_64k_x_64k_nx_large.nm
|   |           mp_rr_desertlands_64k_x_64k_nx_medium.nm
|   |           mp_rr_desertlands_64k_x_64k_nx_med_short.nm
|   |           mp_rr_desertlands_64k_x_64k_nx_small.nm
|   |           mp_rr_desertlands_64k_x_64k_small.nm
|   |               
|   +---shaders
|   |   \---fxc
|   |           boxfilter_cs50.vcs
|   |           visquery_ps40.vcs
|   |           
|   \---support
|       +---EA Help
|       |       Assistance technique.fr_FR.rtf
|       |       Kundendienst.rtf
|       |       Pomoc techniczna.rtf
|       |       Servicio t�cnico.rtf
|       |       Suporte T�cnico.rtf
|       |       Supporto tecnico.rtf
|       |       Technical Support.en_US.rtf
|       |       Technical Support.ja_JP.rtf
|       |       Technical Support.ko_KR.rtf
|       |       Technical Support.ru_RU.rtf
|       |       Technical Support.zh_CN.rtf
|       |       Technical Support.zh_TW.rtf
|       |       
|       +---Privacy and Cookie Policy
|       |       de_DE.html
|       |       en_US.html
|       |       es_ES.html
|       |       fr_FR.html
|       |       it_IT.html
|       |       ja_JP.html
|       |       ko_KR.html
|       |       pl_PL.html
|       |       pt_BR.html
|       |       ru_RU.html
|       |       zh_CN.html
|       |       zh_TW.html
|       |       
|       \---User Agreement
|               de_DE.html
|               en_US.html
|               es_ES.html
|               fr_FR.html
|               it_IT.html
|               ja_JP.html
|               ko_KR.html
|               pl_PL.html
|               pt_BR.html
|               ru_RU.html
|               zh_CN.html
|               zh_TW.html
|               
+---r2
|   \---maps
|       \---graphs
+---stbsp
|       mp_lobby.stbsp
|       mp_rr_canyonlands_64k_x_64k.stbsp
|       mp_rr_canyonlands_mu1.stbsp
|       mp_rr_canyonlands_mu1_night.stbsp
|       mp_rr_canyonlands_staging.stbsp
|       mp_rr_desertlands_1.stbsp
|       mp_rr_desertlands_64k_x_64k.stbsp
|       
\---vpk
        client_frontend.bsp.pak000_000.vpk
        client_frontend.bsp.pak000_001.vpk
        client_frontend.bsp.pak000_002.vpk
        client_frontend.bsp.pak000_003.vpk
        client_frontend.bsp.pak000_004.vpk
        client_frontend.bsp.pak000_005.vpk
        client_frontend.bsp.pak000_006.vpk
        client_frontend.bsp.pak000_007.vpk
        client_frontend.bsp.pak000_008.vpk
        client_mp_common.bsp.pak000_000.vpk
        client_mp_common.bsp.pak000_001.vpk
        client_mp_common.bsp.pak000_002.vpk
        client_mp_common.bsp.pak000_003.vpk
        client_mp_common.bsp.pak000_004.vpk
        client_mp_common.bsp.pak000_005.vpk
        client_mp_common.bsp.pak000_006.vpk
        client_mp_common.bsp.pak000_007.vpk
        client_mp_rr_canyonlands_64k_x_64k.bsp.pak000_000.vpk
        client_mp_rr_canyonlands_64k_x_64k.bsp.pak000_001.vpk
        client_mp_rr_canyonlands_64k_x_64k.bsp.pak000_002.vpk
        client_mp_rr_canyonlands_64k_x_64k.bsp.pak000_003.vpk
        client_mp_rr_canyonlands_mu1.bsp.pak000_000.vpk
        client_mp_rr_canyonlands_mu1.bsp.pak000_001.vpk
        client_mp_rr_canyonlands_mu1.bsp.pak000_002.vpk
        client_mp_rr_canyonlands_mu1.bsp.pak000_003.vpk
        client_mp_rr_canyonlands_mu1.bsp.pak000_004.vpk
        client_mp_rr_canyonlands_mu1_night.bsp.pak000_000.vpk
        client_mp_rr_canyonlands_mu1_night.bsp.pak000_001.vpk
        client_mp_rr_canyonlands_mu1_night.bsp.pak000_002.vpk
        client_mp_rr_canyonlands_staging.bsp.pak000_000.vpk
        client_mp_rr_canyonlands_staging.bsp.pak000_001.vpk
        client_mp_rr_canyonlands_staging.bsp.pak000_002.vpk
        client_mp_rr_canyonlands_staging.bsp.pak000_003.vpk
        client_mp_rr_canyonlands_staging.bsp.pak000_004.vpk
        client_mp_rr_desertlands_64k_x_64k.bsp.pak000_000.vpk
        client_mp_rr_desertlands_64k_x_64k.bsp.pak000_001.vpk
        enable.txt
        englishclient_frontend.bsp.pak000_dir.vpk
        englishclient_frontend.bsp.pak000_dir.vpk.bak
        englishclient_mp_common.bsp.pak000_dir.vpk
        englishclient_mp_common.bsp.pak000_dir.vpk.bak
        englishclient_mp_rr_canyonlands_64k_x_64k.bsp.pak000_dir.vpk
        englishclient_mp_rr_canyonlands_mu1.bsp.pak000_dir.vpk
        englishclient_mp_rr_canyonlands_mu1_night.bsp.pak000_dir.vpk
        englishclient_mp_rr_canyonlands_staging.bsp.pak000_dir.vpk
        englishclient_mp_rr_desertlands_64k_x_64k.bsp.pak000_dir.vpk
        englishserver_mp_common.bsp.pak000_dir.vpk
        server_mp_common.bsp.pak000_000.vpk

```