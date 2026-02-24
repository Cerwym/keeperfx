/******************************************************************************/
/** @file gog_galaxy_api.h
 *     Header file for GOG Galaxy achievement backend.
 * @par Purpose:
 *     Provides GOG Galaxy SDK integration for achievements.
 * @par Comment:
 *     Dynamically loads Galaxy.dll at runtime via LoadLibrary/GetProcAddress.
 *     No link-time dependency on Galaxy.lib.
 * @author   Peter Lockett & KeeperFX Team
 * @date     24 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef GOG_GALAXY_API_H
#define GOG_GALAXY_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the GOG Galaxy achievement backend.
 * Loads Galaxy.dll, authenticates, and requests user achievements.
 * @return 0 on success, 1 on failure, -1 if not on Windows.
 */
int gog_galaxy_init(void);

/**
 * Shutdown the GOG Galaxy backend.
 * Releases the Galaxy SDK and unloads the DLL.
 */
void gog_galaxy_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // GOG_GALAXY_API_H
