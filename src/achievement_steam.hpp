/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file achievement_steam.hpp
 *     Header file for achievement_steam.cpp.
 * @par Purpose:
 *     Steam achievement backend implementation.
 * @par Comment:
 *     Provides Steam-specific achievement support.
 * @author   KeeperFX Team
 * @date     02 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef ACHIEVEMENT_STEAM_HPP
#define ACHIEVEMENT_STEAM_HPP

#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register Steam achievement backend.
 * @return True if successfully registered.
 */
TbBool steam_achievements_register(void);

/**
 * Check if Steam achievements are available.
 * @return True if Steam API is initialized and achievements are available.
 */
TbBool steam_achievements_is_available(void);

#ifdef __cplusplus
}
#endif

#endif // ACHIEVEMENT_STEAM_HPP
