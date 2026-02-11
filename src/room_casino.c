/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file room_casino.c
 *     Casino room implementation.
 * @par Purpose:
 *     Core casino room logic including wall detection, odds management,
 *     and gambling mechanics.
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
#include "room_casino.h"

#include "room_data.h"
#include "creature_control.h"
#include "slab_data.h"
#include "config_terrain.h"
#include "game_legacy.h"
#include "dungeon_data.h"
#include "player_utils.h"
#include "game_merge.h"
#include "thing_creature.h"

/******************************************************************************/

/**
 * Calculates the capacity of a casino room.
 * Each gambling device slab can accommodate one creature.
 */
long get_casino_capacity(const struct Room* room)
{
    if (room->kind != RoK_CASINO) {
        return 0;
    }
    
    // Each SlbT_CASINO slab fits 1 creature
    long capacity = 0;
    SlabCodedCoords slbnum = room->slabs_list;
    
    for (int i = 0; i < room->slabs_count; i++) {
        if (slbnum == 0) break;
        
        MapSlabCoord slb_x = slb_num_decode_x(slbnum);
        MapSlabCoord slb_y = slb_num_decode_y(slbnum);
        struct SlabMap* slb = get_slabmap_for_subtile(slab_subtile_center(slb_x), slab_subtile_center(slb_y));
        if (slb->kind == SlbT_CASINO) {
            capacity++;
        }
        slbnum = slb->next_in_room;
    }
    
    return capacity;
}

/**
 * Checks if a casino room has at least one reinforced wall slab.
 * Reinforced walls activate the odds controller.
 */
TbBool casino_has_reinforced_wall(const struct Room* room)
{
    if (room->kind != RoK_CASINO) {
        return false;
    }
    
    SlabCodedCoords slbnum = room->slabs_list;
    
    for (int i = 0; i < room->slabs_count; i++) {
        if (slbnum == 0) break;
        
        MapSlabCoord slb_x = slb_num_decode_x(slbnum);
        MapSlabCoord slb_y = slb_num_decode_y(slbnum);
        
        // Check 4 adjacent slabs (N, S, E, W)
        const int adj_offsets[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        
        for (int dir = 0; dir < 4; dir++) {
            MapSlabCoord adj_x = slb_x + adj_offsets[dir][0];
            MapSlabCoord adj_y = slb_y + adj_offsets[dir][1];
            struct SlabMap* adj_slb = get_slabmap_block(adj_x, adj_y);
            
            if (adj_slb->kind == SlbT_CASINO_WALL) {
                return true;
            }
        }
        
        slbnum = get_next_slab_number_in_room(slbnum);
    }
    
    return false;
}

/**
 * Updates the activation status of a casino room based on reinforced walls.
 * Called when room changes (slabs added/removed).
 */
void update_casino_activation(struct Room* room)
{
    if (room->kind != RoK_CASINO) {
        return;
    }
    
    TbBool has_wall = casino_has_reinforced_wall(room);
    room->casino.is_active = has_wall;
    
    if (!has_wall) {
        // Deactivate odds when walls are removed
        room->casino.house_odds_tier = 0;
    } else if (room->casino.house_odds_tier == 0) {
        // Activate with default odds when walls are first added
        // Default odds tier 1 (Fair) - will be configurable via rules.cfg
        room->casino.house_odds_tier = 1;
    }
}

/**
 * Gets the current odds tier for a casino room.
 * @return 0=Off/Inactive, 1=Fair, 2=Moderate, 3=Rigged
 */
unsigned char get_casino_odds_tier(const struct Room* room)
{
    if (room->kind != RoK_CASINO) {
        return 0;
    }
    return room->casino.house_odds_tier;
}

/**
 * Sets the odds tier for a casino room.
 * Only works if casino is active (has reinforced walls).
 */
void set_casino_odds_tier(struct Room* room, unsigned char tier)
{
    if (room->kind != RoK_CASINO) {
        return;
    }
    
    if (tier > 3) {
        tier = 3;
    }
    
    // Only allow setting odds if casino is active
    if (room->casino.is_active) {
        room->casino.house_odds_tier = tier;
    }
}

/**
 * Calculates the bet amount for a creature's gambling session.
 * Creatures bet 10-25% of their current wealth.
 */
GoldAmount calculate_gambling_bet(const struct Thing* creatng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    
    // Bet 10-25% of current wealth
    GoldAmount max_bet = cctrl->current_gold_held / 4;  // 25%
    GoldAmount min_bet = cctrl->current_gold_held / 10; // 10%
    
    if (max_bet <= min_bet) {
        return min_bet;
    }
    
    // Random amount between min and max
    return min_bet + GAME_RANDOM(max_bet - min_bet);
}

/**
 * Resolves a gambling session outcome (win/lose) and updates gold accordingly.
 * @return True if creature won, false if creature lost.
 */
TbBool resolve_gambling_outcome(struct Thing* creatng, struct Room* room, GoldAmount bet)
{
    if (room->kind != RoK_CASINO) {
        return false;
    }
    
    unsigned char odds_tier = room->casino.house_odds_tier;
    if (odds_tier == 0 || !room->casino.is_active) {
        return false; // Inactive casino
    }
    
    // Get win chance based on odds tier
    // Will use game.conf values when config system is added
    int win_chance;
    switch (odds_tier) {
        case 1: win_chance = 45; break; // Fair odds
        case 2: win_chance = 30; break; // Moderate house advantage
        case 3: win_chance = 15; break; // Rigged heavily
        default: win_chance = 0; break;
    }
    
    // Roll dice
    int roll = GAME_RANDOM(100);
    TbBool won = (roll < win_chance);
    
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    
    if (won) {
        // Calculate winnings (2x bet default)
        // Will use game.conf.casino_payout_multiplier when config is added
        const int payout_mult = 200; // 200% = 2x
        GoldAmount payout = (bet * payout_mult) / 100;
        
        // Pay winnings
        if (pay_casino_winnings(creatng, payout)) {
            cctrl->current_gold_held += payout;
            // House loses
            record_casino_profit(room, -payout);
        }
    } else {
        // Creature loses bet
        cctrl->current_gold_held -= bet;
        // House wins
        record_casino_profit(room, bet);
    }
    
    // Update cooldown
    cctrl->last_casino_visit_turn = game.play_gameturn;
    
    return won;
}

/**
 * Pays out casino winnings to a creature based on configured payout source.
 * @return True if payment successful, false if source is broke.
 */
TbBool pay_casino_winnings(struct Thing* creatng, GoldAmount amount)
{
    // Will use game.conf.casino_payout_source when config is added
    // For now, default to Magic (always pays)
    CasinoPayoutSource source = CasinoPayout_Magic;
    
    switch (source) {
        case CasinoPayout_Magic:
            // Gold from nowhere, always succeeds
            return true;
            
        case CasinoPayout_Treasury: {
            // From player treasury
            struct Dungeon* dungeon = get_dungeon(creatng->owner);
            if (dungeon->total_money_owned < amount) {
                return false; // Treasury broke
            }
            return take_money_from_dungeon(creatng->owner, amount, 0);
        }
            
        case CasinoPayout_HouseProfit: {
            // From room profit - not implemented yet (need to find room creature is on)
            // For now, treat as magic
            return true;
        }
            
        default:
            return false;
    }
}

/**
 * Records profit/loss for a casino room.
 * Positive values = house won, negative values = house lost.
 */
void record_casino_profit(struct Room* room, GoldAmount profit)
{
    if (room->kind != RoK_CASINO) {
        return;
    }
    
    room->casino.accumulated_profit += profit;
    
    // Clamp to prevent overflow
    if (room->casino.accumulated_profit < 0) {
        room->casino.accumulated_profit = 0;
    }
}

/******************************************************************************/
#include "post_inc.h"
