/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_states_casino.c
 *     Creature gambling behavior state machine implementation.
 * @par Purpose:
 *     Handles creatures visiting casino, selecting devices, and gambling.
 * @par Comment:
 *     None.
 * @author   GitHub Copilot (AI)
 * @date     10 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "creature_states_casino.h"

#include "creature_states.h"
#include "creature_control.h"
#include "room_casino.h"
#include "room_data.h"
#include "thing_navigate.h"
#include "config_creature.h"
#include "slab_data.h"
#include "game_legacy.h"
#include "room_list.h"
#include "game_merge.h"

/******************************************************************************/

/**
 * Main state handler for creature gambling behavior.
 * Manages the full gambling session from device selection to outcome.
 * 
 * State progression (field_82):
 * 0 - Navigate to gambling device
 * 1 - Approach device (walking)
 * 2 - Play gambling animation
 * 3 - Victory celebration
 * 4 - Disappointed reaction
 */
short creature_gambling(struct Thing* creatng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    struct Room* room = get_room_thing_is_on(creatng);
    
    // Validate still in casino
    if (!room || room->kind != RoK_CASINO) {
        cctrl->casino.state = 0;
        return CrStRet_ResetFail;
    }
    
    switch (cctrl->casino.state) {
        case 0:  // Navigate to device
            if (!setup_gambling_device_search(creatng, room)) {
                return CrStRet_ResetFail;
            }
            cctrl->casino.state = 1;
            cctrl->casino.device_use_time = 0;
            return CrStRet_Modified;
            
        case 1:  // Approach device
            // Wait a few turns for creature to reach device
            cctrl->casino.device_use_time++;
            if (cctrl->casino.device_use_time >= 16) {
                cctrl->casino.state = 2;
                // Start gambling animation (will be implemented with sprite system)
                // For now, just use generic activity
            }
            return CrStRet_Modified;
            
        case 2:  // Play animation (gambling in progress)
            // Wait for animation to complete
            // In full implementation, this would check animation state
            // For now, use a simple timer
            if (cctrl->instance_id == CrInst_NULL) {
                // Animation complete, resolve outcome
                GoldAmount bet = calculate_gambling_bet(creatng);
                TbBool won = resolve_gambling_outcome(creatng, room, bet);
                
                // React based on outcome
                if (won) {
                    // Victory - creature is happy
                    cctrl->casino.state = 3;
                } else {
                    // Loss - creature is disappointed
                    cctrl->casino.state = 4;
                }
            }
            return CrStRet_Modified;
            
        case 3:  // Victory celebration
            // Play happy animation/sound
            // For now, just finish
            return CrStRet_ResetOk; // Done, return to idle
            
        case 4:  // Disappointed reaction
            // Play sad animation/sound
            // For now, just finish
            return CrStRet_ResetOk;
            
        default:
            return CrStRet_ResetFail;
    }
}

/**
 * Checks if a creature should visit the casino based on mood and state.
 * Creatures gamble when idle/bored, not when seeking comfort in lair.
 */
TbBool creature_should_visit_casino(struct Thing* creatng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    
    // Don't gamble if going to lair for comfort
    // annoyance_level[AngR_NoLair] represents "need for lair" - 10 is threshold for seeking lair
    if (cctrl->annoyance_level[AngR_NoLair] >= 10) {
        return false;
    }
    
    // Check eligibility (cooldown, minimum gold, etc.)
    if (!can_creature_use_casino(creatng)) {
        return false;
    }
    
    // Random chance based on boredom level
    // More bored creatures are more likely to gamble
    int base_chance = 5;
    int boredom_bonus = (10 - cctrl->annoyance_level[AngR_NoLair]); // Higher when less need for lair
    int total_chance = base_chance + boredom_bonus;
    
    return GAME_RANDOM(100) < total_chance;
}

/**
 * Checks if a creature is eligible to use the casino.
 * Verifies cooldown, minimum gold, and basic requirements.
 */
TbBool can_creature_use_casino(struct Thing* creatng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    
    // Minimum gold requirement
    if (cctrl->current_gold_held < 100) {
        return false;
    }
    
    // Cooldown check - don't visit too frequently (2000 turns = ~40 seconds)
    if ((game.play_gameturn - cctrl->last_casino_visit_turn) < 2000) {
        return false;
    }
    
    // Mood check - angry/livid creatures don't gamble
    if ((cctrl->mood_flags & (CCMoo_Angry | CCMoo_Livid)) != 0) {
        return false;
    }
    
    return true;
}

/**
 * Finds an available casino room for a creature to visit.
 * Prefers casinos that are active and have spare capacity.
 */
struct Room* find_casino_for_creature(struct Thing* creatng)
{
    // Find nearest casino room using proper API
    struct Room *room = get_player_room_of_kind_nearest_to(
        creatng->owner,
        RoK_CASINO,
        creatng->mappos.x.stl.num,
        creatng->mappos.y.stl.num,
        NULL
    );
    
    if (room_is_invalid(room)) {
        return INVALID_ROOM;
    }
    
    // Check if casino is active and has capacity
    if (!room->casino.is_active) {
        return INVALID_ROOM;
    }
    
    long capacity = get_casino_capacity(room);
    if (room->used_capacity >= capacity) {
        return INVALID_ROOM;
    }
    
    return room;
}

/**
 * Sets up a creature to start gambling.
 * Transitions creature to gambling state.
 */
TbBool setup_creature_gambling(struct Thing* creatng)
{
    struct Room* room = find_casino_for_creature(creatng);
    if (!room_exists(room)) {
        return false;
    }
    
    // Transition to gambling state
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    cctrl->target_room_id = room->index;
    cctrl->last_casino_visit_turn = game.play_gameturn;
    
    internal_set_thing_state(creatng, CrSt_CreatureGambling);
    creatng->continue_state = CrSt_CreatureGambling;
    
    return true;
}

/**
 * Searches for an available gambling device in the casino.
 * Uses random starting point (like lair flower search) for variety.
 */
TbBool setup_gambling_device_search(struct Thing* creatng, struct Room* room)
{
    if (room->kind != RoK_CASINO) {
        return false;
    }
    
    // Start at random slab in room (prevents all creatures going to same device)
    long n = GAME_RANDOM(room->slabs_count);
    SlabCodedCoords slbnum = room->slabs_list;
    
    // Skip to random starting point
    for (long k = n; k > 0; k--) {
        if (slbnum == 0) break;
        MapSlabCoord slb_x = slb_num_decode_x(slbnum);
        MapSlabCoord slb_y = slb_num_decode_y(slbnum);
        struct SlabMap* slb = get_slabmap_for_subtile(slab_subtile_center(slb_x), slab_subtile_center(slb_y));
        slbnum = slb->next_in_room;
    }
    
    // Search for gambling device slab
    for (int i = 0; i < room->slabs_count; i++) {
        MapSlabCoord slb_x = slb_num_decode_x(slbnum);
        MapSlabCoord slb_y = slb_num_decode_y(slbnum);
        struct SlabMap* slb = get_slabmap_for_subtile(slab_subtile_center(slb_x), slab_subtile_center(slb_y));
        
        if (slb->kind == SlbT_CASINO) {
            // Found device slab, navigate to its center
            // (slb_x and slb_y already set from loop iteration)
            
            // Center of slab is subtile 4 (middle of 0-8 grid)
            MapSubtlCoord stl_x = slab_subtile(slb_x, 4);
            MapSubtlCoord stl_y = slab_subtile(slb_y, 4);
            
            // Set up navigation
            setup_person_move_to_position(creatng, stl_x, stl_y, 0);
            return true;
        }
        
        slbnum = slb->next_in_room;
        if (slbnum == 0) break;
    }
    
    return false;
}

/******************************************************************************/
#include "post_inc.h"
