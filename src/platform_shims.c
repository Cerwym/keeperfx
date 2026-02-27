/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file platform_shims.c
 *     No-op stubs for desktop-only features excluded from homebrew platform builds.
 * @par Purpose:
 *     Provides empty implementations of functions whose full source files
 *     depend on libraries unavailable on Vita/3DS/Switch (SDL_net, ffmpeg,
 *     miniupnpc, Lua, spng, etc.).  Compiled only on homebrew platforms.
 */
/******************************************************************************/
#include "pre_inc.h"

#if defined(PLATFORM_VITA) || defined(PLATFORM_3DS) || defined(PLATFORM_SWITCH)

#include "api.h"
#include "sounds.h"
#include "net_input_lag.h"
#include "bflib_sndlib.h"
#include "bflib_coroutine.h"
#include "globals.h"
#include "lua_base.h"
#include "lua_triggers.h"
#include "moonphase.h"
#include "net_resync.h"

/* api.c stubs (SDL_net removed) */
int api_init_server(void) { return 0; }
void api_update_server(void) {}
void api_close_server(void) {}
void api_event(const char *event_name) { (void)event_name; }

/* sounds.h streaming stubs (SDL_mixer/bflib_sndlib removed) */
TbBool play_streamed_sample(const char *fname, SoundVolume vol) { (void)fname; (void)vol; return 0; }
void set_streamed_sample_volume(SoundVolume vol) { (void)vol; }
void stop_streamed_samples(void) {}

/* bflib_sndlib.cpp stubs */
void FreeAudio(void) {}
TbBool play_music(const char *fname) { (void)fname; return 0; }
TbBool play_music_track(int t) { (void)t; return 0; }
void resume_music(void) {}
void stop_music(void) {}

/* net_input_lag.c stubs (networking removed) */
void clear_input_lag_queue(void) {}

/* net_resync / globals: networking multiplayer logging flag */
TbBool detailed_multiplayer_logging = 0;

/* steam_api.cpp stubs — steam_api.hpp uses extern "C" so define in C */
int steam_api_init(void) { return 0; }
void steam_api_shutdown(void) {}

/* net_resync.cpp stubs — called from main_game.c, game networking removed */
TbBool LbNetwork_Resync(void *data_buffer, size_t buffer_length) { (void)data_buffer; (void)buffer_length; return 0; }
void LbNetwork_TimesyncBarrier(void) {}
void animate_resync_progress_bar(int current_phase, int total_phases) { (void)current_phase; (void)total_phases; }
void resync_game(void) {}

/* lua_base.c stubs — LuaJIT not available on homebrew platforms */
TbBool open_lua_script(LevelNumber lvnum) { (void)lvnum; return 0; }
void close_lua_script(void) {}
const char* get_lua_serialized_data(size_t *len) { if (len) *len = 0; return NULL; }
void set_lua_serialized_data(const char* data, size_t len) { (void)data; (void)len; }
TbBool execute_lua_code_from_console(const char* code) { (void)code; return 0; }
TbBool execute_lua_code_from_script(const char* code) { (void)code; return 0; }
const char* lua_get_serialised_data(size_t *len) { if (len) *len = 0; return NULL; }
void lua_set_serialised_data(const char *data, size_t len) { (void)data; (void)len; }
void cleanup_serialized_data(void) {}
void lua_set_random_seed(unsigned int seed) { (void)seed; }
void generate_lua_types_file(void) {}

/* lua_triggers.c stubs — Lua event hooks become no-ops on homebrew */
void lua_on_chatmsg(PlayerNumber plyr_idx, char *msg) { (void)plyr_idx; (void)msg; }
void lua_on_game_start(void) {}
void lua_on_game_tick(void) {}
void lua_on_power_cast(PlayerNumber plyr_idx, PowerKind pwkind, unsigned short splevel, MapSubtlCoord stl_x, MapSubtlCoord stl_y, struct Thing *thing) { (void)plyr_idx; (void)pwkind; (void)splevel; (void)stl_x; (void)stl_y; (void)thing; }
void lua_on_special_box_activate(PlayerNumber plyr_idx, struct Thing *cratetng) { (void)plyr_idx; (void)cratetng; }
void lua_on_dungeon_destroyed(PlayerNumber plyr_idx) { (void)plyr_idx; }
void lua_on_creature_death(struct Thing *crtng) { (void)crtng; }
void lua_on_creature_rebirth(struct Thing *crtng) { (void)crtng; }
void lua_on_trap_placed(struct Thing *traptng) { (void)traptng; }
void lua_on_apply_damage_to_thing(struct Thing *thing, HitPoints dmg, PlayerNumber dealing_plyr_idx) { (void)thing; (void)dmg; (void)dealing_plyr_idx; }
void lua_on_level_up(struct Thing *thing) { (void)thing; }

/* moonphase.c stubs — astronomy library not available on homebrew */
short is_full_moon = 0;
short is_near_full_moon = 0;
short is_new_moon = 0;
short is_near_new_moon = 0;
short calculate_moon_phase(short do_calculate, short add_to_log) { (void)do_calculate; (void)add_to_log; return 0; }

#endif // PLATFORM_VITA || PLATFORM_3DS || PLATFORM_SWITCH

#include "post_inc.h"
