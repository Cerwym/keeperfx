/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file room_casino.h
 *     Header file for room_casino.c.
 * @par Purpose:
 *     Casino room support functions.
 * @par Comment:
 *     Casino provides gambling entertainment for idle creatures.
 * @author   GitHub Copilot (AI)
 * @date     10 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_ROOM_CASINO_H
#define DK_ROOM_CASINO_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#pragma pack(1)

struct Room;
struct Thing;

/** Casino payout source configuration */
typedef enum {
    CasinoPayout_Magic = 0,      /**< Gold materializes from thin air */
    CasinoPayout_Treasury = 1,   /**< Gold drawn from player treasury */
    CasinoPayout_HouseProfit = 2 /**< Gold drawn from accumulated room profit */
} CasinoPayoutSource;

#pragma pack()
/******************************************************************************/

// Capacity calculation
long get_casino_capacity(const struct Room* room);

// Wall detection and activation
TbBool casino_has_reinforced_wall(const struct Room* room);
void update_casino_activation(struct Room* room);

// Odds management
unsigned char get_casino_odds_tier(const struct Room* room);
void set_casino_odds_tier(struct Room* room, unsigned char tier);

// Gambling mechanics
TbBool can_creature_use_casino(const struct Thing* creatng);
GoldAmount calculate_gambling_bet(const struct Thing* creatng);
TbBool resolve_gambling_outcome(struct Thing* creatng, struct Room* room, GoldAmount bet);

// Payout system
TbBool pay_casino_winnings(struct Thing* creatng, GoldAmount amount);
void record_casino_profit(struct Room* room, GoldAmount profit);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
