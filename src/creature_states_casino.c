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
#include "creature_states_casino.h"

#include "creature_states.h"
#include "creature_control.h"
#include "room_casino.h"
#include "room_data.h"
#include "thing_navigate.h"
#include "config_creature.h"
#include "slab_data.h"
#include "game_legacy.h"

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
        cctrl->field_82 = 0;
        return CrStRet_ResetFail;
    }
    
    switch (cctrl->field_82) {
        case 0:  // Navigate to device
            if (!setup_gambling_device_search(creatng, room)) {
                return CrStRet_ResetFail;
            }
            cctrl->field_82 = 1;
            return CrStRet_Modified;
            
        case 1:  // Approach device
            if (creature_arrived_at_pos(creatng)) {
                cctrl->field_82 = 2;
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
                    cctrl->field_82 = 3;
                } else {
                    // Loss - creature is disappointed
                    cctrl->field_82 = 4;
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
    // field_AI_val represents "need for lair" - 10 is threshold for seeking lair
    if (cctrl->field_AI_val >= 10) {
        return false;
    }
    
    // Check eligibility (cooldown, minimum gold, etc.)
    if (!can_creature_use_casino(creatng)) {
        return false;
    }
    
    // Random chance based on boredom level
    // More bored creatures are more likely to gamble
    int base_chance = 5;
    int boredom_bonus = (10 - cctrl->field_AI_val); // Higher when less need for lair
    int total_chance = base_chance + boredom_bonus;
    
    return CREATURE_RANDOM(creatng, 100) < total_chance;
}

/**
 * Finds an available casino room for a creature to visit.
 * Prefers casinos that are active and have spare capacity.
 */
struct Room* find_casino_for_creature(struct Thing* creatng)
{
    struct Dungeon* dungeon = get_dungeon(creatng->owner);
    
    // Iterate through player's casino rooms
    long i = dungeon->room_kind[RoK_CASINO];
    unsigned long k = 0;
    
    while (i != 0) {
        struct Room* room = room_get(i);
        if (room_is_invalid(room)) {
            break;
        }
        
        i = room->next_of_owner;
        
        // Check if casino is active and has capacity
        if (room->casino.is_active) {
            long capacity = get_casino_capacity(room);
            if (room->used_capacity < capacity) {
                return room;
            }
        }
        
        k++;
        if (k >= ROOMS_COUNT) {
            break;
        }
    }
    
    return INVALID_ROOM;
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
    // Note: CrSt_CreatureGambling needs to be added to creature state enum
    // For now, this will cause a compile error until state is added
    // internal_set_thing_state(creatng, CrSt_CreatureGambling);
    // creatng->continue_state = CrSt_CreatureGambling;
    
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
    long n = CREATURE_RANDOM(creatng, room->slabs_count);
    SlabCodedCoords slbnum = room->slabs_list;
    
    // Skip to random starting point
    for (long k = n; k > 0; k--) {
        if (slbnum == 0) break;
        slbnum = get_next_slab_number_in_room(slbnum);
    }
    
    // Search for gambling device slab
    for (int i = 0; i < room->slabs_count; i++) {
        struct SlabMap* slb = get_slabmap_for_subtile(slbnum);
        
        if (slb->kind == SlbT_CASINO) {
            // Found device slab, navigate to its center
            MapSlabCoord slb_x = slb_num_decode_x(slbnum);
            MapSlabCoord slb_y = slb_num_decode_y(slbnum);
            
            // Center of slab is subtile 4 (middle of 0-8 grid)
            MapSubtlCoord stl_x = slab_subtile(slb_x, 4);
            MapSubtlCoord stl_y = slab_subtile(slb_y, 4);
            
            // Set up navigation
            setup_person_move_to_position(creatng, stl_x, stl_y, 0);
            return true;
        }
        
        slbnum = get_next_slab_number_in_room(slbnum);
        if (slbnum == 0) break;
    }
    
    return false;
}

/******************************************************************************/
