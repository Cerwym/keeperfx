/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file bflib_network_stub.c
 *     No-op stubs for all networking functionality.
 * @par Purpose:
 *     Compiled instead of the real networking files when KEEPERFX_NETWORKING=OFF.
 *     Allows the game to run in single-player-only mode without any network
 *     library dependencies (SDL_net, enet, miniupnpc, libnatpmp).
 */
/******************************************************************************/
#include "pre_inc.h"

#include <stdint.h>
#include <stddef.h>
#include "bflib_basics.h"
#include "bflib_network.h"
#include "bflib_network_exchange.h"
#include "bflib_enet.h"
#include "bflib_coroutine.h"
#include "net_resync.h"
#include "net_game.h"
#include "net_input_lag.h"
#include "net_checksums.h"
#include "net_redundant_packets.h"
#include "net_received_packets.h"
#include "packets.h"

#include "post_inc.h"

/* net_game.c — multiplayer session data (UI still compiled, network disabled) */
long net_number_of_sessions = 0;
struct TbNetworkSessionNameEntry *net_session[32] = { NULL };
long net_session_index_active = -1;
char net_service[16][NET_SERVICE_LEN];
char net_player_name[20] = { 0 };
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

/* net_resync.cpp — called from main_game.c */
TbBool detailed_multiplayer_logging = 0;
TbBool LbNetwork_Resync(void *data_buffer, size_t buffer_length) { (void)data_buffer; (void)buffer_length; return 0; }
void LbNetwork_TimesyncBarrier(void) {}
void animate_resync_progress_bar(int current_phase, int total_phases) { (void)current_phase; (void)total_phases; }
void resync_game(void) {}

/* net_input_lag.c stubs */
void clear_input_lag_queue(void) {}
struct Packet* get_local_input_lag_packet_for_turn(GameTurn target_turn) { (void)target_turn; return NULL; }
void  store_local_packet_in_input_lag_queue(PlayerNumber my_packet_num) { (void)my_packet_num; }
TbBool input_lag_skips_initial_processing(void) { return 0; }
unsigned short calculate_skip_input(void) { return 0; }

/* net_checksums.c stubs */
void  update_turn_checksums(void) {}
short checksums_different(void) { return 0; }

/* net_redundant_packets.c stubs */
void initialize_redundant_packets(void) {}
void unbundle_packets(const char *bundled_buffer, PlayerNumber source_player) { (void)bundled_buffer; (void)source_player; }

/* net_received_packets.c stubs */
void initialize_packet_tracking(void) {}
const struct Packet* get_received_packets_for_turn(GameTurn turn) { (void)turn; return NULL; }

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
