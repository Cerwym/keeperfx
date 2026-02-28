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

#include <stdint.h>
#include "api.h"
#include "sounds.h"
#include "net_input_lag.h"
#include "bflib_sndlib.h"
#include "bflib_sound.h"
#include "bflib_fmvids.h"
#include "bflib_coroutine.h"
#include "globals.h"
#include "lua_base.h"
#include "lua_triggers.h"
#include "config.h"
#include "lua_cfg_funcs.h"
#include "moonphase.h"
#include "net_resync.h"
#include "net_game.h"
#include "bflib_network.h"
#include "bflib_network_exchange.h"
#include "custom_sprites.h"
#include "creature_graphics.h"
#include "engine_render.h"
#include "engine_arrays.h"
#include "scrcapt.h"
#include "audio/audio_interface.h"
#include "input/input_interface.h"
#include "bflib_enet.h"
#include "console_cmd.h"
#include "net_checksums.h"
#include "net_redundant_packets.h"
#include "net_received_packets.h"
#include "packets.h"

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

/* audio_openal.c / input_sdl.c global interface pointers */
AudioInterface* g_audio = NULL;
InputInterface* g_input = NULL;

/* bflib_sndlib.cpp — additional sound engine stubs */
void SetSoundMasterVolume(SoundVolume v) { (void)v; }
TbBool GetSoundInstalled(void) { return 0; }
void MonitorStreamedSoundTrack(void) {}
void *GetSoundDriver(void) { return NULL; }
void StopAllSamples(void) {}
TbBool InitAudio(const struct SoundSettings *s) { (void)s; return 0; }
TbBool IsSamplePlaying(SoundMilesID id) { (void)id; return 0; }
void SetSampleVolume(SoundEmitterID e, SoundSmplTblID t, SoundVolume v) { (void)e; (void)t; (void)v; }
void SetSamplePan(SoundEmitterID e, SoundSmplTblID t, SoundPan p) { (void)e; (void)t; (void)p; }
void SetSamplePitch(SoundEmitterID e, SoundSmplTblID t, SoundPitch p) { (void)e; (void)t; (void)p; }
void toggle_bbking_mode(void) {}
void set_music_volume(SoundVolume v) { (void)v; }
void pause_music(void) {}

/* bflib_sound.h — sample playback stubs (defined in bflib_sndlib.cpp) */
SoundMilesID play_sample(SoundEmitterID e, SoundSmplTblID t, SoundVolume v, SoundPan p, SoundPitch pi, char rep, unsigned char ctype, SoundBankID b) { (void)e; (void)t; (void)v; (void)p; (void)pi; (void)rep; (void)ctype; (void)b; return 0; }
void stop_sample(SoundEmitterID e, SoundSmplTblID t, SoundBankID b) { (void)e; (void)t; (void)b; }
SoundSFXID get_sample_sfxid(SoundSmplTblID t, SoundBankID b) { (void)t; (void)b; return 0; }

/* bflib_fmvids.cpp — video playback stub */
TbBool play_smk(const char *filename, int flags) { (void)filename; (void)flags; return 0; }

/* scrcapt.c — screenshot / movie stubs */
unsigned char screenshot_format = 1;
TbBool movie_record_start(void) { return 0; }
TbBool movie_record_stop(void) { return 0; }
TbBool take_screenshot(char *fname) { (void)fname; return 0; }
TbBool perform_any_screen_capturing(void) { return 0; }
TbBool cumulative_screen_shot(void) { return 0; }

/* custom_sprites.c — sprite globals and lookup functions */
struct TbSpriteSheet *gui_panel_sprites = NULL;
unsigned char *big_scratch = NULL;
short bad_icon_id = INT16_MAX;
int total_sprite_zip_count = 0;
short iso_td_add[KEEPERSPRITE_ADD_NUM];
short td_iso_add[KEEPERSPRITE_ADD_NUM];
TbSpriteData keepersprite_add[KEEPERSPRITE_ADD_NUM] = { 0 };
struct KeeperSprite creature_table_add[KEEPERSPRITE_ADD_NUM] = { {0} };

const struct TbSprite *get_panel_sprite(short i) { (void)i; return NULL; }
const struct TbSprite *get_button_sprite(short i) { (void)i; return NULL; }
const struct TbSprite *get_button_sprite_for_player(short i, PlayerNumber p) { (void)i; (void)p; return NULL; }
const struct TbSprite *get_frontend_sprite(short i) { (void)i; return NULL; }
const struct TbSprite *get_new_icon_sprite(short i) { (void)i; return NULL; }
short get_icon_id(const char *name) { (void)name; return 0; }
short get_anim_id_(const char *name) { (void)name; return 0; }
short get_anim_id(const char *name, struct ObjectConfigStats *objst) { (void)name; (void)objst; return 0; }
const struct LensOverlayData *get_lens_overlay_data(const char *name) { (void)name; return NULL; }
const struct LensMistData *get_lens_mist_data(const char *name) { (void)name; return NULL; }

/* lua_cfg_funcs.c — Lua-driven creature-state / update functions */
short luafunc_crstate_func(FuncIdx func_idx, struct Thing *thing) { (void)func_idx; (void)thing; return 0; }
short luafunc_thing_update_func(FuncIdx func_idx, struct Thing *thing) { (void)func_idx; (void)thing; return 0; }
TbResult luafunc_magic_use_power(FuncIdx func_idx, PlayerNumber plyr_idx, PowerKind pwkind,
    unsigned short splevel, MapSubtlCoord stl_x, MapSubtlCoord stl_y, struct Thing *thing, unsigned long allow_flags)
    { (void)func_idx; (void)plyr_idx; (void)pwkind; (void)splevel; (void)stl_x; (void)stl_y; (void)thing; (void)allow_flags; return Lb_FAIL; }
FuncIdx get_function_idx(const char *func_name, const struct NamedCommand *Cfuncs) { (void)func_name; (void)Cfuncs; return 0; }

/* net_game.c — multiplayer session data (UI still compiled, network disabled) */
long net_number_of_sessions = 0;
struct TbNetworkSessionNameEntry *net_session[32] = { NULL };
long net_session_index_active = -1;
char net_service[16][NET_SERVICE_LEN];
char net_player_name[20] = { 0 };

/* bflib_network.cpp — LbNetwork API stubs */
void    LbNetwork_SetServerPort(int port) { (void)port; }
void    LbNetwork_InitSessionsFromCmdLine(const char *str) { (void)str; }
TbError LbNetwork_Init(unsigned long srvcindex, unsigned long maxplayrs, struct TbNetworkPlayerInfo *locplayr, struct ServiceInitData *init_data) { (void)srvcindex; (void)maxplayrs; (void)locplayr; (void)init_data; return Lb_FAIL; }
TbError LbNetwork_Join(struct TbNetworkSessionNameEntry *nsname, char *playr_name, int32_t *playr_num, void *optns) { (void)nsname; (void)playr_name; (void)playr_num; (void)optns; return Lb_FAIL; }
TbError LbNetwork_Create(char *nsname_str, char *plyr_name, uint32_t *plyr_num, void *optns) { (void)nsname_str; (void)plyr_name; (void)plyr_num; (void)optns; return Lb_FAIL; }
TbError LbNetwork_EnableNewPlayers(TbBool allow) { (void)allow; return Lb_FAIL; }
TbError LbNetwork_EnumerateServices(TbNetworkCallbackFunc callback, void *user_data) { (void)callback; (void)user_data; return Lb_FAIL; }
TbError LbNetwork_EnumeratePlayers(struct TbNetworkSessionNameEntry *sesn, TbNetworkCallbackFunc callback, void *user_data) { (void)sesn; (void)callback; (void)user_data; return Lb_FAIL; }
TbError LbNetwork_EnumerateSessions(TbNetworkCallbackFunc callback, void *ptr) { (void)callback; (void)ptr; return Lb_FAIL; }
TbError LbNetwork_Stop(void) { return Lb_FAIL; }
void    LbNetwork_UpdateInputLagIfHost(void) {}

/* bflib_network_exchange.cpp — packet exchange stubs */
TbError LbNetwork_Exchange(enum NetMessageType msg_type, void *send_buf, void *server_buf, size_t buf_size) { (void)msg_type; (void)send_buf; (void)server_buf; (void)buf_size; return Lb_FAIL; }
TbError LbNetwork_ExchangeLogin(char *plyr_name) { (void)plyr_name; return Lb_FAIL; }
void    LbNetwork_WaitForMissingPackets(void *server_buf, size_t client_frame_size) { (void)server_buf; (void)client_frame_size; }
void    LbNetwork_SendChatMessageImmediate(int player_id, const char *message) { (void)player_id; (void)message; }
void    LbNetwork_BroadcastUnpauseTimesync(void) {}

/* input_sdl.c / audio_openal.c — init functions */
void input_sdl_initialize(void) {}
void audio_openal_initialize(void) {}

/* bflib_sndlib.cpp — missing volume getter */
SoundVolume GetCurrentSoundMasterVolume(void) { return 0; }

/* bflib_enet.cpp — network stats (enet excluded, stubs return zero) */
unsigned long GetPing(int id) { (void)id; return 0; }
unsigned long GetPingVariance(int id) { (void)id; return 0; }
unsigned int  GetPacketLoss(int id) { (void)id; return 0; }
unsigned int  GetClientDataInTransit(void) { return 0; }
unsigned int  GetIncomingPacketQueueSize(void) { return 0; }
unsigned int  GetClientPacketsLost(void) { return 0; }
unsigned int  GetClientOutgoingDataTotal(void) { return 0; }
unsigned int  GetClientIncomingDataTotal(void) { return 0; }
unsigned int  GetClientReliableCommandsInFlight(void) { return 0; }

/* console_cmd.c — developer console (Lua excluded) */
void   cmd_auto_completion(PlayerNumber plyr_idx, char *cmd_str, size_t cmd_size) { (void)plyr_idx; (void)cmd_str; (void)cmd_size; }
TbBool cmd_exec(PlayerNumber plyr_idx, char *msg) { (void)plyr_idx; (void)msg; return 0; }

/* net_game.c — additional multiplayer globals and helpers */
struct TbNetworkPlayerName net_player[NET_PLAYERS_COUNT];
struct ConfigInfo net_config_info;
struct TbNetworkPlayerInfo net_player_info[NET_PLAYERS_COUNT];

TbBool network_player_active(int plyr_idx) { (void)plyr_idx; return 0; }
const char *network_player_name(int plyr_idx) { (void)plyr_idx; return ""; }
void sync_various_data(void) {}
unsigned long get_host_player_id(void) { return 0; }
short setup_network_service(int srvidx) { (void)srvidx; return 0; }
int   setup_old_network_service(void) { return 0; }
void  init_players_network_game(CoroutineLoop *context) { (void)context; }
void  setup_count_players(void) {}
long  network_session_join(void) { return 0; }

/* custom_sprites.c — sprite system init */
void init_custom_sprites(LevelNumber level_no) { (void)level_no; }
int  is_custom_icon(short icon_idx) { (void)icon_idx; return 0; }

/* net_input_lag.c — missing lag queue accessors */
struct Packet* get_local_input_lag_packet_for_turn(GameTurn target_turn) { (void)target_turn; return NULL; }
void  store_local_packet_in_input_lag_queue(PlayerNumber my_packet_num) { (void)my_packet_num; }
TbBool input_lag_skips_initial_processing(void) { return 0; }
unsigned short calculate_skip_input(void) { return 0; }

/* net_checksums.c — checksum verification stubs */
void  update_turn_checksums(void) {}
short checksums_different(void) { return 0; }

/* net_redundant_packets.c — packet bundling stubs */
void initialize_redundant_packets(void) {}
void unbundle_packets(const char *bundled_buffer, PlayerNumber source_player) { (void)bundled_buffer; (void)source_player; }

/* net_received_packets.c — received packet tracking stubs */
void initialize_packet_tracking(void) {}
const struct Packet* get_received_packets_for_turn(GameTurn turn) { (void)turn; return NULL; }

#endif // PLATFORM_VITA || PLATFORM_3DS || PLATFORM_SWITCH

#include "post_inc.h"
