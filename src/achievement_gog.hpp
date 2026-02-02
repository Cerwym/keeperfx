/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file achievement_gog.hpp
 *     Header file for achievement_gog.cpp.
 * @par Purpose:
 *     GOG Galaxy achievement backend implementation.
 * @par Comment:
 *     Provides GOG Galaxy-specific achievement support.
 * @author   KeeperFX Team
 * @date     02 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef ACHIEVEMENT_GOG_HPP
#define ACHIEVEMENT_GOG_HPP

#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register GOG Galaxy achievement backend.
 * @return True if successfully registered.
 */
TbBool gog_achievements_register(void);

/**
 * Check if GOG Galaxy achievements are available.
 * @return True if Galaxy API is initialized and achievements are available.
 */
TbBool gog_achievements_is_available(void);

#ifdef __cplusplus
}
#endif

#endif // ACHIEVEMENT_GOG_HPP
