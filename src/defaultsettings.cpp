/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <IrrCompileConfig.h>
#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "config.h"
#include "constants.h"
#include "mapgen/mapgen.h" // Mapgen::setDefaultSettings
#include "util/string.h"

#ifndef SERVER
#include "client/renderingengine.h"
#endif

#ifdef __IOS__
#import "wrapper.h"
#endif

void set_default_settings()
{
	Settings *settings = Settings::getLayer(SL_DEFAULTS);
	if (settings == nullptr)
		settings = Settings::createLayer(SL_DEFAULTS);

	// Client and server
	settings->setDefault("language", "");
	settings->setDefault("name", "");
	settings->setDefault("bind_address", "");
	settings->setDefault("serverlist_url", "servers.multicraft.world");

	// Client
	settings->setDefault("address", "");
	settings->setDefault("enable_sound", "true");
	settings->setDefault("sound_volume", "1.0");
	settings->setDefault("mute_sound", "false");
	settings->setDefault("enable_mesh_cache", "false");
	settings->setDefault("mesh_generation_interval", "0");
	settings->setDefault("meshgen_block_cache_size", "20");
	settings->setDefault("enable_vbo", "true");
	settings->setDefault("free_move", "false");
	settings->setDefault("pitch_move", "false");
	settings->setDefault("fast_move", "false");
	settings->setDefault("noclip", "false");
	settings->setDefault("screenshot_path", "screenshots");
	settings->setDefault("screenshot_format", "png");
	settings->setDefault("screenshot_quality", "0");
	settings->setDefault("client_unload_unused_data_timeout", "600");
	settings->setDefault("client_mapblock_limit", "7500");
	settings->setDefault("enable_build_where_you_stand", "false");
	settings->setDefault("curl_timeout", "5000");
	settings->setDefault("curl_parallel_limit", "8");
	settings->setDefault("curl_file_download_timeout", "300000");
	settings->setDefault("curl_verify_cert", "true");
	settings->setDefault("enable_remote_media_server", "true");
	settings->setDefault("enable_client_modding", "true");
	settings->setDefault("max_out_chat_queue_size", "20");
	settings->setDefault("pause_on_lost_focus", "true");
	settings->setDefault("enable_register_confirmation", "true");

	// Keymap
	settings->setDefault("remote_port", "30000");
	settings->setDefault("keymap_forward", "KEY_KEY_W");
	settings->setDefault("keymap_autoforward", "");
	settings->setDefault("keymap_backward", "KEY_KEY_S");
	settings->setDefault("keymap_left", "KEY_KEY_A");
	settings->setDefault("keymap_right", "KEY_KEY_D");
	settings->setDefault("keymap_jump", "KEY_SPACE");
	settings->setDefault("keymap_sneak", "KEY_LSHIFT");
	settings->setDefault("keymap_dig", "KEY_LBUTTON");
	settings->setDefault("keymap_place", "KEY_RBUTTON");
	settings->setDefault("keymap_drop", "KEY_KEY_Q");
	settings->setDefault("keymap_zoom", "KEY_KEY_Z");
	settings->setDefault("keymap_inventory", "KEY_KEY_I");
	settings->setDefault("keymap_special1", "KEY_KEY_E");
	settings->setDefault("keymap_chat", "KEY_KEY_T");
	settings->setDefault("keymap_cmd", "/");
	settings->setDefault("keymap_cmd_local", ".");
	settings->setDefault("keymap_minimap", "KEY_KEY_V");
	settings->setDefault("keymap_console", "KEY_F10");
	settings->setDefault("keymap_rangeselect", "KEY_KEY_R");
	settings->setDefault("keymap_freemove", "KEY_KEY_K");
	settings->setDefault("keymap_pitchmove", "KEY_KEY_P");
	settings->setDefault("keymap_fastmove", "KEY_KEY_J");
	settings->setDefault("keymap_noclip", "KEY_KEY_H");
	settings->setDefault("keymap_hotbar_next", "KEY_KEY_N");
	settings->setDefault("keymap_hotbar_previous", "KEY_KEY_B");
	settings->setDefault("keymap_mute", "KEY_KEY_M");
	settings->setDefault("keymap_increase_volume", "");
	settings->setDefault("keymap_decrease_volume", "");
	settings->setDefault("keymap_cinematic", "");
	settings->setDefault("keymap_toggle_hud", "KEY_F1");
	settings->setDefault("keymap_toggle_chat", "KEY_F2");
	settings->setDefault("keymap_toggle_fog", "KEY_F3");
#if DEBUG
	settings->setDefault("keymap_toggle_update_camera", "KEY_F4");
#else
	settings->setDefault("keymap_toggle_update_camera", "");
#endif
	settings->setDefault("keymap_toggle_debug", "KEY_F5");
	settings->setDefault("keymap_toggle_profiler", "KEY_F6");
	settings->setDefault("keymap_camera_mode", "KEY_KEY_C");
	settings->setDefault("keymap_screenshot", "KEY_F12");
	settings->setDefault("keymap_increase_viewing_range_min", "+");
	settings->setDefault("keymap_decrease_viewing_range_min", "-");
	settings->setDefault("keymap_slot1", "KEY_KEY_1");
	settings->setDefault("keymap_slot2", "KEY_KEY_2");
	settings->setDefault("keymap_slot3", "KEY_KEY_3");
	settings->setDefault("keymap_slot4", "KEY_KEY_4");
	settings->setDefault("keymap_slot5", "KEY_KEY_5");
	settings->setDefault("keymap_slot6", "KEY_KEY_6");
	settings->setDefault("keymap_slot7", "KEY_KEY_7");
	settings->setDefault("keymap_slot8", "KEY_KEY_8");
	settings->setDefault("keymap_slot9", "KEY_KEY_9");
	settings->setDefault("keymap_slot10", "KEY_KEY_0");
	settings->setDefault("keymap_slot11", "");
	settings->setDefault("keymap_slot12", "");
	settings->setDefault("keymap_slot13", "");
	settings->setDefault("keymap_slot14", "");
	settings->setDefault("keymap_slot15", "");
	settings->setDefault("keymap_slot16", "");
	settings->setDefault("keymap_slot17", "");
	settings->setDefault("keymap_slot18", "");
	settings->setDefault("keymap_slot19", "");
	settings->setDefault("keymap_slot20", "");
	settings->setDefault("keymap_slot21", "");
	settings->setDefault("keymap_slot22", "");
	settings->setDefault("keymap_slot23", "");
	settings->setDefault("keymap_slot24", "");
	settings->setDefault("keymap_slot25", "");
	settings->setDefault("keymap_slot26", "");
	settings->setDefault("keymap_slot27", "");
	settings->setDefault("keymap_slot28", "");
	settings->setDefault("keymap_slot29", "");
	settings->setDefault("keymap_slot30", "");
	settings->setDefault("keymap_slot31", "");
	settings->setDefault("keymap_slot32", "");

	// Some (temporary) keys for debugging
	settings->setDefault("keymap_quicktune_prev", "KEY_HOME");
	settings->setDefault("keymap_quicktune_next", "KEY_END");
	settings->setDefault("keymap_quicktune_dec", "KEY_NEXT");
	settings->setDefault("keymap_quicktune_inc", "KEY_PRIOR");

	// Visuals
#ifdef NDEBUG
	settings->setDefault("show_debug", "false");
#else
	settings->setDefault("show_debug", "true");
#endif
	settings->setDefault("fsaa", "0");
	settings->setDefault("undersampling", "0");
	settings->setDefault("world_aligned_mode", "enable");
	settings->setDefault("autoscale_mode", "disable");
	settings->setDefault("enable_fog", "true");
	settings->setDefault("fog_start", "0.4");
	settings->setDefault("3d_mode", "none");
	settings->setDefault("3d_paralax_strength", "0.025");
	settings->setDefault("tooltip_show_delay", "400");
	settings->setDefault("tooltip_append_itemname", "false");
	settings->setDefault("fps_max", "60");
	settings->setDefault("fps_max_unfocused", "20");
	settings->setDefault("viewing_range", "190");
#if ENABLE_GLES
	settings->setDefault("near_plane", "0.1");
#endif
	settings->setDefault("screen_w", "1024");
	settings->setDefault("screen_h", "768");
	settings->setDefault("autosave_screensize", "true");
	settings->setDefault("fullscreen", "false");
	settings->setDefault("fullscreen_bpp", "24");
	settings->setDefault("vsync", "false");
	settings->setDefault("fov", "72");
	settings->setDefault("leaves_style", "fancy");
	settings->setDefault("connected_glass", "false");
	settings->setDefault("smooth_lighting", "true");
	settings->setDefault("lighting_alpha", "0.0");
	settings->setDefault("lighting_beta", "1.5");
	settings->setDefault("display_gamma", "1.0");
	settings->setDefault("lighting_boost", "0.2");
	settings->setDefault("lighting_boost_center", "0.5");
	settings->setDefault("lighting_boost_spread", "0.2");
	settings->setDefault("texture_path", "");
	settings->setDefault("shader_path", "");
#if ENABLE_GLES
#ifdef _IRR_COMPILE_WITH_OGLES1_
	settings->setDefault("video_driver", "ogles1");
#else
	settings->setDefault("video_driver", "ogles2");
#endif
#else
	settings->setDefault("video_driver", "opengl");
#endif
	settings->setDefault("cinematic", "false");
	settings->setDefault("camera_smoothing", "0");
	settings->setDefault("cinematic_camera_smoothing", "0.7");
	settings->setDefault("enable_clouds", "true");
	settings->setDefault("view_bobbing_amount", "1.0");
	settings->setDefault("fall_bobbing_amount", "1.0");
	settings->setDefault("enable_3d_clouds", "true");
	settings->setDefault("cloud_radius", "12");
	settings->setDefault("menu_clouds", "true");
	settings->setDefault("opaque_water", "false");
	settings->setDefault("console_height", "0.6");
	settings->setDefault("console_message_height", "0.25");
	settings->setDefault("console_color", "(0,0,0)");
	settings->setDefault("console_alpha", "200");
	settings->setDefault("formspec_fullscreen_bg_color", "(0,0,0)");
	settings->setDefault("formspec_fullscreen_bg_opacity", "140");
	settings->setDefault("formspec_default_bg_color", "(0,0,0)");
	settings->setDefault("formspec_default_bg_opacity", "140");
	settings->setDefault("selectionbox_color", "(0,0,0)");
	settings->setDefault("selectionbox_width", "4");
	settings->setDefault("node_highlighting", "box");
	settings->setDefault("crosshair_color", "(255,255,255)");
	settings->setDefault("crosshair_alpha", "255");
	settings->setDefault("recent_chat_messages", "10");
	settings->setDefault("hud_scaling", "1.0");
	settings->setDefault("gui_scaling", "1.0");
	settings->setDefault("gui_scaling_filter", "false");
	settings->setDefault("gui_scaling_filter_txr2img", "true");
	settings->setDefault("desynchronize_mapblock_texture_animation", "false");
	settings->setDefault("hud_hotbar_max_width", "1.0");
	settings->setDefault("hud_move_upwards", "0");
	settings->setDefault("round_screen", "0");
	settings->setDefault("enable_local_map_saving", "false");
	settings->setDefault("show_entity_selectionbox", "false");
	settings->setDefault("transparency_sorting", "true");
	settings->setDefault("texture_clean_transparent", "false");
	settings->setDefault("texture_min_size", "0");
	settings->setDefault("ambient_occlusion_gamma", "1.8");
#if ENABLE_GLES
	settings->setDefault("enable_shaders", "false");
#else
	settings->setDefault("enable_shaders", "true");
#endif
	settings->setDefault("enable_particles", "true");
	settings->setDefault("arm_inertia", "true");
	settings->setDefault("show_nametag_backgrounds", "false");

	settings->setDefault("enable_minimap", "true");
	settings->setDefault("minimap_shape_round", "true");
	settings->setDefault("minimap_double_scan_height", "false");

	// Effects
	settings->setDefault("directional_colored_fog", "true");
	settings->setDefault("inventory_items_animations", "false");
	settings->setDefault("mip_map", "false");
	settings->setDefault("anisotropic_filter", "false");
	settings->setDefault("bilinear_filter", "false");
	settings->setDefault("trilinear_filter", "false");
	settings->setDefault("tone_mapping", "false");
	settings->setDefault("enable_waving_water", "false");
	settings->setDefault("water_wave_height", "1.0");
	settings->setDefault("water_wave_length", "20.0");
	settings->setDefault("water_wave_speed", "5.0");
	settings->setDefault("enable_waving_leaves", "false");
	settings->setDefault("enable_waving_plants", "false");


	// Input
	settings->setDefault("invert_mouse", "false");
	settings->setDefault("mouse_sensitivity", "0.2");
	settings->setDefault("repeat_place_time", "0.25");
	settings->setDefault("safe_dig_and_place", "false");
	settings->setDefault("random_input", "false");
	settings->setDefault("aux1_descends", "false");
	settings->setDefault("doubletap_jump", "false");
	settings->setDefault("always_fly_fast", "true");
	settings->setDefault("autojump", "false");
	settings->setDefault("continuous_forward", "false");
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	settings->setDefault("enable_joysticks", "true");
#else
	settings->setDefault("enable_joysticks", "false");
#endif
	settings->setDefault("joystick_id", "0");
	settings->setDefault("joystick_type", "");
	settings->setDefault("repeat_joystick_button_time", "0.17");
	settings->setDefault("joystick_frustum_sensitivity", "170");
	settings->setDefault("joystick_deadzone", "4096");

	// Main menu
	settings->setDefault("main_menu_path", "");
	settings->setDefault("serverlist_file", "favoriteservers.json");

#if USE_FREETYPE
	settings->setDefault("freetype", "true");
	std::string MultiCraftFont = porting::getDataPath("fonts" DIR_DELIM "MultiCraftFont.ttf");

#if !defined(__ANDROID__) && !defined(__APPLE__)
	settings->setDefault("font_path", porting::getDataPath("fonts" DIR_DELIM "Arimo-Regular.ttf"));
	settings->setDefault("font_path_italic", porting::getDataPath("fonts" DIR_DELIM "Arimo-Italic.ttf"));
	settings->setDefault("font_path_bold", porting::getDataPath("fonts" DIR_DELIM "Arimo-Bold.ttf"));
	settings->setDefault("font_path_bold_italic", porting::getDataPath("fonts" DIR_DELIM "Arimo-BoldItalic.ttf"));
#else
	settings->setDefault("font_path", MultiCraftFont);
	settings->setDefault("font_path_italic", MultiCraftFont);
	settings->setDefault("font_path_bold", MultiCraftFont);
	settings->setDefault("font_path_bold_italic", MultiCraftFont);
#endif

	settings->setDefault("font_bold", "false");
	settings->setDefault("font_italic", "false");
	settings->setDefault("font_shadow", "1");
	settings->setDefault("font_shadow_alpha", "127");

	settings->setDefault("mono_font_path", MultiCraftFont);
	settings->setDefault("mono_font_path_italic", MultiCraftFont);
	settings->setDefault("mono_font_path_bold", MultiCraftFont);
	settings->setDefault("mono_font_path_bold_italic", MultiCraftFont);

	settings->setDefault("emoji_font_path", porting::getDataPath("fonts" DIR_DELIM "NotoEmoji.ttf"));

#if !defined(__ANDROID__) && !defined(__APPLE__)
	settings->setDefault("fallback_font_path", porting::getDataPath("fonts" DIR_DELIM "DroidSansFallbackFull.ttf"));
#else
	settings->setDefault("fallback_font_path", MultiCraftFont);
#endif

	settings->setDefault("fallback_font_shadow", "1");
	settings->setDefault("fallback_font_shadow_alpha", "128");

	std::string font_size_str = std::to_string(TTF_DEFAULT_FONT_SIZE);

	settings->setDefault("font_size", font_size_str); // fallback_font_size
#else
	settings->setDefault("freetype", "false");
	settings->setDefault("font_path", porting::getDataPath("fonts" DIR_DELIM "mono_dejavu_sans"));
	settings->setDefault("mono_font_path", porting::getDataPath("fonts" DIR_DELIM "mono_dejavu_sans"));

	std::string font_size_str = std::to_string(DEFAULT_FONT_SIZE);
#endif
	// General font settings
	settings->setDefault("font_size", font_size_str);
	settings->setDefault("mono_font_size", font_size_str);
	settings->setDefault("chat_font_size", "0"); // Default "font_size"

	// ContentDB
	settings->setDefault("contentdb_url", "https://content.multicraft.world");
	settings->setDefault("contentdb_max_concurrent_downloads", "3");

#ifdef __ANDROID__
	settings->setDefault("contentdb_flag_blacklist", "nonfree, android_default");
#else
	settings->setDefault("contentdb_flag_blacklist", "nonfree, desktop_default");
#endif

	settings->setDefault("update_information_url", "https://updates.multicraft.world/app.json");
	#if ENABLE_UPDATE_CHECKER
		settings->setDefault("update_last_checked", "");
	#else
		settings->setDefault("update_last_checked", "disabled");
	#endif

	// Server
	settings->setDefault("disable_texture_packs", "false");
	settings->setDefault("disable_escape_sequences", "false");
	settings->setDefault("strip_color_codes", "true");
	settings->setDefault("announce_mt", "true");
#if USE_PROMETHEUS
	settings->setDefault("prometheus_listener_address", "127.0.0.1:30000");
#endif

	// Network
	settings->setDefault("enable_ipv6", "true");
	settings->setDefault("ipv6_server", "false");
	settings->setDefault("max_packets_per_iteration","1024");
	settings->setDefault("port", "40000");
	settings->setDefault("strict_protocol_version_checking", "false");
	settings->setDefault("player_transfer_distance", "0");
	settings->setDefault("max_simultaneous_block_sends_per_client", "40");
	settings->setDefault("time_send_interval", "5");

	settings->setDefault("default_game", "default");
	settings->setDefault("motd", "");
	settings->setDefault("max_users", "15");
	settings->setDefault("creative_mode", "false");
	settings->setDefault("enable_damage", "true");
	settings->setDefault("default_password", "");
	settings->setDefault("default_privs", "interact, shout");
	settings->setDefault("enable_pvp", "true");
	settings->setDefault("enable_mod_channels", "true");
	settings->setDefault("disallow_empty_password", "false");
	settings->setDefault("disable_anticheat", "false");
	settings->setDefault("enable_rollback_recording", "false");
	settings->setDefault("deprecated_lua_api_handling", "log");

	settings->setDefault("kick_msg_shutdown", "Server shutting down.");
	settings->setDefault("kick_msg_crash", "This server has experienced an internal error. You will now be disconnected.");
	settings->setDefault("ask_reconnect_on_crash", "false");

	settings->setDefault("chat_message_format", "@name: @message");
	settings->setDefault("profiler_print_interval", "0");
	settings->setDefault("active_object_send_range_blocks", "8");
	settings->setDefault("active_block_range", "4");
	//settings->setDefault("max_simultaneous_block_sends_per_client", "1");
	// This causes frametime jitter on client side, or does it?
	settings->setDefault("max_block_send_distance", "12");
	settings->setDefault("block_send_optimize_distance", "4");
	settings->setDefault("server_side_occlusion_culling", "true");
	settings->setDefault("csm_restriction_flags", "60");
	settings->setDefault("csm_restriction_noderange", "0");
	settings->setDefault("max_clearobjects_extra_loaded_blocks", "4096");
	settings->setDefault("time_speed", "72");
	settings->setDefault("world_start_time", "6125");
	settings->setDefault("server_unload_unused_data_timeout", "29");
	settings->setDefault("max_objects_per_block", "64");
	settings->setDefault("server_map_save_interval", "5.3");
	settings->setDefault("chat_message_max_size", "500");
	settings->setDefault("chat_message_limit_per_10sec", "5.0");
	settings->setDefault("chat_message_limit_trigger_kick", "50");
	settings->setDefault("sqlite_synchronous", "2");
	settings->setDefault("map_compression_level_disk", "3");
	settings->setDefault("map_compression_level_net", "-1");
	settings->setDefault("full_block_send_enable_min_time_from_building", "2.0");
	settings->setDefault("dedicated_server_step", "0.09");
	settings->setDefault("active_block_mgmt_interval", "2.0");
	settings->setDefault("abm_interval", "1.0");
	settings->setDefault("abm_time_budget", "0.2");
	settings->setDefault("nodetimer_interval", "0.2");
	settings->setDefault("ignore_world_load_errors", "false");
	settings->setDefault("remote_media", "");
	settings->setDefault("debug_log_level", "warning");
	settings->setDefault("debug_log_size_max", "50");
	settings->setDefault("chat_log_level", "error");
	settings->setDefault("emergequeue_limit_total", "1024");
	settings->setDefault("emergequeue_limit_diskonly", "128");
	settings->setDefault("emergequeue_limit_generate", "128");
	settings->setDefault("num_emerge_threads", "1");
	settings->setDefault("log_mod_memory_usage_on_load", "false");
	settings->setDefault("secure.enable_security", "true");
	settings->setDefault("secure.trusted_mods", "");
	settings->setDefault("secure.http_mods", "");

	// Physics
	settings->setDefault("movement_acceleration_default", "3");
	settings->setDefault("movement_acceleration_air", "2");
	settings->setDefault("movement_acceleration_fast", "20");
	settings->setDefault("movement_speed_walk", "4");
	settings->setDefault("movement_speed_crouch", "1.35");
	settings->setDefault("movement_speed_fast", "20");
	settings->setDefault("movement_speed_climb", "3");
	settings->setDefault("movement_speed_jump", "6.5");
	settings->setDefault("movement_liquid_fluidity", "1");
	settings->setDefault("movement_liquid_fluidity_smooth", "0.5");
	settings->setDefault("movement_liquid_sink", "20");
	settings->setDefault("movement_gravity", "9.81");

	// Liquids
	settings->setDefault("liquid_loop_max", "100000");
	settings->setDefault("liquid_queue_purge_time", "0");
	settings->setDefault("liquid_update", "1.0");

	// Mapgen
	settings->setDefault("mg_name", "v7p");
	settings->setDefault("water_level", "1");
	settings->setDefault("mapgen_limit", "31000");
	settings->setDefault("chunksize", "5");
	settings->setDefault("fixed_map_seed", "");
	settings->setDefault("max_block_generate_distance", "10");
	settings->setDefault("enable_mapgen_debug_info", "false");
	Mapgen::setDefaultSettings(settings);

	// Server list announcing
	settings->setDefault("server_announce", "false");
	settings->setDefault("server_url", "");
	settings->setDefault("server_address", "");
	settings->setDefault("server_name", "");
	settings->setDefault("server_description", "");

	settings->setDefault("high_precision_fpu", "true");
	settings->setDefault("enable_console", "false");
	settings->setDefault("screen_dpi", "72");
	settings->setDefault("display_density_factor", "1");

#ifndef SERVER
	settings->setDefault("device_is_tablet", std::to_string(RenderingEngine::isTablet()));
#endif

	// Altered settings for macOS
#if defined(__MACH__) && defined(__APPLE__) && !defined(__IOS__)
	settings->setDefault("screen_w", "0");
	settings->setDefault("screen_h", "0");
	settings->setDefault("keymap_camera_mode", "KEY_KEY_C");
	settings->setDefault("vsync", "true");

	int ScaleFactor = porting::getScreenScale();
	settings->setDefault("screen_dpi", std::to_string(ScaleFactor * 72));
	if (ScaleFactor >= 2) {
		settings->setDefault("hud_scaling", "1.5");
	} else {
		settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE - 2));
		settings->setDefault("hud_scaling", "1.25");
		settings->setDefault("gui_scaling", "1.5");
	}

	// Shaders work but may reduce performance on iGPU
	settings->setDefault("enable_shaders", "false");
#endif

#ifdef HAVE_TOUCHSCREENGUI
	settings->setDefault("touchtarget", "true");
	settings->setDefault("touchscreen_threshold", "20");
	settings->setDefault("touch_sensitivity", "0.2");
	settings->setDefault("fixed_virtual_joystick", "true");
	settings->setDefault("virtual_joystick_triggers_aux", "false");
	settings->setDefault("fast_move", "true");
#endif

	// Mobile Platform
#if defined(__ANDROID__) || defined(__IOS__)
	settings->setDefault("fullscreen", "true");
	settings->setDefault("emergequeue_limit_diskonly", "16");
	settings->setDefault("emergequeue_limit_generate", "16");
	settings->setDefault("curl_verify_cert", "false");
	settings->setDefault("doubletap_jump", "true");
	settings->setDefault("gui_scaling_filter_txr2img", "false");
	settings->setDefault("autosave_screensize", "false");
	settings->setDefault("recent_chat_messages", "6");

	if (RenderingEngine::isTablet()) {
		settings->setDefault("recent_chat_messages", "8");
		settings->setDefault("console_message_height", "0.4");
	}

	// Set the optimal settings depending on the memory size [Android] | model [iOS]
#ifdef __ANDROID__
	float memoryMax = porting::getTotalSystemMemory() / 1024;

	if (memoryMax < 2) {
		// minimal settings for less than 2GB RAM
#elif __IOS__
	if (false) {
		// obsolete
#endif
		settings->setDefault("client_unload_unused_data_timeout", "60");
		settings->setDefault("client_mapblock_limit", "50");
		settings->setDefault("fps_max", "35");
		settings->setDefault("fps_max_unfocused", "10");
		settings->setDefault("viewing_range", "30");
		settings->setDefault("smooth_lighting", "false");
		settings->setDefault("enable_3d_clouds", "false");
		settings->setDefault("active_object_send_range_blocks", "1");
		settings->setDefault("active_block_range", "1");
		settings->setDefault("max_objects_per_block", "16");
		settings->setDefault("dedicated_server_step", "0.2");
		settings->setDefault("abm_interval", "3.0");
		settings->setDefault("chunksize", "3");
		settings->setDefault("max_block_generate_distance", "1");
		settings->setDefault("arm_inertia", "false");
#ifdef __ANDROID__
	} else if (memoryMax >= 2 && memoryMax < 4) {
		// low settings for 2-4GB RAM
#elif __IOS__
	} else if (!IOS_VERSION_AVAILABLE("13.0")) {
		// low settings
#endif
		settings->setDefault("client_unload_unused_data_timeout", "120");
		settings->setDefault("client_mapblock_limit", "200");
		settings->setDefault("fps_max", "35");
		settings->setDefault("fps_max_unfocused", "10");
		settings->setDefault("viewing_range", "40");
		settings->setDefault("smooth_lighting", "false");
		settings->setDefault("active_object_send_range_blocks", "1");
		settings->setDefault("active_block_range", "2");
		settings->setDefault("max_objects_per_block", "16");
		settings->setDefault("dedicated_server_step", "0.2");
		settings->setDefault("abm_interval", "2.0");
		settings->setDefault("chunksize", "3");
		settings->setDefault("max_block_generate_distance", "2");
		settings->setDefault("arm_inertia", "false");
#ifdef __ANDROID__
	} else if (memoryMax >= 4 && memoryMax <= 5) {
		// medium settings for 4.1-5GB RAM
#elif __IOS__
	} else if (([SDVersion deviceVersion] == iPhone6S) || ([SDVersion deviceVersion] == iPhone6SPlus) || ([SDVersion deviceVersion] == iPhoneSE) ||
			   ([SDVersion deviceVersion] == iPhone7) || ([SDVersion deviceVersion] == iPhone7Plus) ||
			   ([SDVersion deviceVersion] == iPadMini4) || ([SDVersion deviceVersion] == iPadAir2) || ([SDVersion deviceVersion] == iPad5))
	{
		// medium settings
#endif
		settings->setDefault("client_unload_unused_data_timeout", "180");
		settings->setDefault("client_mapblock_limit", "300");
		settings->setDefault("fps_max", "35");
		settings->setDefault("viewing_range", "60");
		settings->setDefault("active_object_send_range_blocks", "2");
		settings->setDefault("active_block_range", "2");
		settings->setDefault("max_objects_per_block", "32");
		settings->setDefault("max_block_generate_distance", "3");
	} else {
		// high settings
		settings->setDefault("client_mapblock_limit", "500");
		settings->setDefault("viewing_range", "120");
		settings->setDefault("active_object_send_range_blocks", "4");
		settings->setDefault("max_block_generate_distance", "5");

		// enable visual shader effects
		settings->setDefault("enable_waving_water", "true");
		settings->setDefault("enable_waving_leaves", "true");
		settings->setDefault("enable_waving_plants", "true");
	}

	// Android Settings
#ifdef __ANDROID__
	// Switch to olges2 with shaders on powerful Android devices
	if (memoryMax > 5) {
		settings->setDefault("video_driver", "ogles2");
		settings->setDefault("enable_shaders", "true");
	} else {
		settings->setDefault("video_driver", "ogles1");
		settings->setDefault("enable_shaders", "false");
	}

	// Prefer ogles2 on an Intel platform
	std::string arch = porting::getCpuArchitecture();
	if (arch == "x86" || arch == "x86_64") {
		settings->setDefault("video_driver", "ogles2");
		settings->setDefault("enable_shaders", "true");
	}

	v2u32 window_size = RenderingEngine::getDisplaySize();
	if (window_size.X > 0) {
		float x_inches = window_size.X / (160.f * RenderingEngine::getDisplayDensity());
		if (x_inches <= 3.7) {
			// small 4" phones
			g_settings->setDefault("hud_scaling", "0.55");
			g_settings->setDefault("touch_sensitivity", "0.3");
		} else if (x_inches > 3.7 && x_inches <= 4.5) {
			// medium phones
			g_settings->setDefault("hud_scaling", "0.6");
			g_settings->setDefault("selectionbox_width", "6");
		} else if (x_inches > 4.5 && x_inches <= 5.5) {
			// large 6" phones
			g_settings->setDefault("hud_scaling", "0.7");
			g_settings->setDefault("selectionbox_width", "6");
		} else if (x_inches > 5.5 && x_inches <= 6.5) {
			// 7" tablets
			g_settings->setDefault("hud_scaling", "0.9");
			g_settings->setDefault("selectionbox_width", "6");
		}

		if (x_inches <= 4.5) {
			settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE - 1));
		} else if (x_inches >= 7.0) {
			settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE + 1));
		}

		// Settings for the Rounded or Cutout Screen
		int RoundScreen = porting::getRoundScreen();
		if (RoundScreen > 0)
			settings->setDefault("round_screen", std::to_string(RoundScreen));
	}
#endif // Android

	// iOS Settings
#ifdef __IOS__
	// Switch to olges2 with shaders in new iOS versions
	if (IOS_VERSION_AVAILABLE("14.0")) {
		settings->setDefault("video_driver", "ogles2");
		settings->setDefault("enable_shaders", "true");
	} else {
		settings->setDefault("video_driver", "ogles1");
		settings->setDefault("enable_shaders", "false");
	}

	settings->setDefault("debug_log_level", "none");

	// Set the size of the elements depending on the screen size
	if SDVersion4Inch {
		// 4" iPhone and iPod Touch
		settings->setDefault("hud_scaling", "0.55");
		settings->setDefault("touch_sensitivity", "0.33");
	} else if SDVersion4and7Inch {
		// 4.7" iPhone
		settings->setDefault("hud_scaling", "0.6");
		settings->setDefault("touch_sensitivity", "0.27");
	} else if SDVersion5and5Inch {
		// 5.5" iPhone Plus
		settings->setDefault("hud_scaling", "0.6");
		settings->setDefault("touch_sensitivity", "0.3");
	} else if (SDVersion5and8Inch || SDVersion6and1Inch) {
		// 5.8" and 6.1" iPhones
		settings->setDefault("hud_scaling", "0.8");
		settings->setDefault("touch_sensitivity", "0.35");
		settings->setDefault("selectionbox_width", "6");
	} else if SDVersion6and5Inch {
		// 6.5" iPhone
		settings->setDefault("hud_scaling", "0.85");
		settings->setDefault("touch_sensitivity", "0.35");
		settings->setDefault("selectionbox_width", "6");
	} else if SDVersion7and9Inch {
		// iPad mini
		settings->setDefault("hud_scaling", "0.9");
		settings->setDefault("touch_sensitivity", "0.25");
		settings->setDefault("selectionbox_width", "6");
	} else if SDVersion8and3Inch {
		settings->setDefault("touch_sensitivity", "0.25");
		settings->setDefault("selectionbox_width", "6");
	} else {
		// iPad
		settings->setDefault("touch_sensitivity", "0.3");
		settings->setDefault("selectionbox_width", "6");
	}

	if SDVersion4Inch {
		settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE - 2));
	} else if (SDVersion4and7Inch || SDVersion5and5Inch) {
		settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE - 1));
	} else if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad && !SDVersion7and9Inch) {
		settings->setDefault("font_size", std::to_string(TTF_DEFAULT_FONT_SIZE + 1));
	}

	// Settings for the Rounded Screen and Home Bar
	int RoundScreen = porting::getRoundScreen();
	if (RoundScreen > 0) {
		int upwards = 25, round = 40;
		if SDVersioniPhone12Series {
			upwards = 20, round = 90;
		} else if SDVersion8and3Inch {
			upwards = 15, round = 20;
		}

		settings->setDefault("hud_move_upwards", std::to_string(upwards));
		settings->setDefault("round_screen", std::to_string(round));
	}
#endif // iOS
#endif
}
