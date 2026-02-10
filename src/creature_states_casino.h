/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_states_casino.h
 *     Header file for creature_states_casino.c.
 * @par Purpose:
 *     Creature gambling behavior state machine.
 * @par Comment:
 *     Handles creature interaction with casino rooms.
 * @author   GitHub Copilot (AI)
 * @date     10 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_CRTR_STATES_CASINO_H
#define DK_CRTR_STATES_CASINO_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#pragma pack(1)

struct Thing;
struct Room;

#pragma pack()
/******************************************************************************/

// State handler (will be added to creature state enum)
short creature_gambling(struct Thing* creatng);

// Decision logic
TbBool creature_should_visit_casino(struct Thing* creatng);

// Navigation
struct Room* find_casino_for_creature(struct Thing* creatng);
TbBool setup_creature_gambling(struct Thing* creatng);

// Gambling device interaction
TbBool setup_gambling_device_search(struct Thing* creatng, struct Room* room);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
