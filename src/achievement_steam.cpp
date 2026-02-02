/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file achievement_steam.cpp
 *     Steam achievement backend implementation.
 * @par Purpose:
 *     Implements achievement support using Steam/Steamworks API.
 * @par Comment:
 *     Provides Steam-specific achievement unlock and tracking.
 * @author   KeeperFX Team
 * @date     02 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "achievement_steam.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include "achievement_api.h"
#include "bflib_fileio.h"
#include "keeperfx.hpp"
#include "post_inc.h"

/******************************************************************************/
// Steam API type definitions
// These match the Steamworks SDK but are defined here to avoid requiring the full SDK

typedef uint64_t uint64;
typedef uint32_t uint32;

// Minimal Steam API interface definitions
class ISteamUserStats
{
public:
    virtual bool RequestCurrentStats() = 0;
    virtual bool GetStat(const char* pchName, int32_t* pData) = 0;
    virtual bool GetStat(const char* pchName, float* pData) = 0;
    virtual bool SetStat(const char* pchName, int32_t nData) = 0;
    virtual bool SetStat(const char* pchName, float fData) = 0;
    virtual bool UpdateAvgRateStat(const char* pchName, float flCountThisSession, double dSessionLength) = 0;
    virtual bool GetAchievement(const char* pchName, bool* pbAchieved) = 0;
    virtual bool SetAchievement(const char* pchName) = 0;
    virtual bool ClearAchievement(const char* pchName) = 0;
    virtual bool GetAchievementAndUnlockTime(const char* pchName, bool* pbAchieved, uint32* punUnlockTime) = 0;
    virtual bool StoreStats() = 0;
};

/******************************************************************************/
// Function pointer types for dynamically loaded Steam API functions
typedef void* (*SteamInternal_ContextInit_t)(void* pContextInitData);
typedef void* (*SteamInternal_FindOrCreateUserInterface_t)(int hSteamUser, const char* pszVersion);
typedef void (*SteamAPI_RunCallbacks_t)();

// Global Steam API function pointers
static SteamInternal_ContextInit_t SteamInternal_ContextInit = NULL;
static SteamInternal_FindOrCreateUserInterface_t SteamInternal_FindOrCreateUserInterface = NULL;
static SteamAPI_RunCallbacks_t SteamAPI_RunCallbacks = NULL;

// Steam UserStats interface
static ISteamUserStats* g_pSteamUserStats = NULL;

#ifdef _WIN32
static HMODULE steam_achievement_lib = NULL;
#endif

static TbBool steam_achievements_available = false;

/******************************************************************************/
// Steam achievement backend implementation

static TbBool steam_achievement_init(void)
{
#ifndef _WIN32
    return false;
#else
    // Check if Steam API DLL is available
    if (!LbFileExists("steam_api.dll"))
    {
        JUSTLOG("Steam API DLL not found, achievements unavailable");
        return false;
    }
    
    // Load Steam API library
    steam_achievement_lib = LoadLibraryA("steam_api.dll");
    if (!steam_achievement_lib)
    {
        WARNLOG("Failed to load steam_api.dll for achievements");
        return false;
    }
    
    // Get function pointers
    SteamAPI_RunCallbacks = reinterpret_cast<SteamAPI_RunCallbacks_t>(
        GetProcAddress(steam_achievement_lib, "SteamAPI_RunCallbacks"));
    
    SteamInternal_ContextInit = reinterpret_cast<SteamInternal_ContextInit_t>(
        GetProcAddress(steam_achievement_lib, "SteamInternal_ContextInit"));
    
    SteamInternal_FindOrCreateUserInterface = reinterpret_cast<SteamInternal_FindOrCreateUserInterface_t>(
        GetProcAddress(steam_achievement_lib, "SteamInternal_FindOrCreateUserInterface"));
    
    if (!SteamAPI_RunCallbacks || !SteamInternal_ContextInit || !SteamInternal_FindOrCreateUserInterface)
    {
        ERRORLOG("Failed to get Steam API function pointers");
        FreeLibrary(steam_achievement_lib);
        steam_achievement_lib = NULL;
        return false;
    }
    
    // Get Steam UserStats interface
    // Note: This is a simplified approach; full implementation would use proper Steam API initialization
    g_pSteamUserStats = reinterpret_cast<ISteamUserStats*>(
        SteamInternal_FindOrCreateUserInterface(0, "SteamUserStats012"));
    
    if (!g_pSteamUserStats)
    {
        WARNLOG("Failed to get Steam UserStats interface");
        FreeLibrary(steam_achievement_lib);
        steam_achievement_lib = NULL;
        return false;
    }
    
    // Request current stats from Steam
    if (!g_pSteamUserStats->RequestCurrentStats())
    {
        WARNLOG("Failed to request Steam stats");
    }
    
    steam_achievements_available = true;
    SYNCLOG("Steam achievements initialized successfully");
    return true;
#endif
}

static void steam_achievement_shutdown(void)
{
#ifdef _WIN32
    if (steam_achievement_lib != NULL)
    {
        FreeLibrary(steam_achievement_lib);
        steam_achievement_lib = NULL;
    }
    
    g_pSteamUserStats = NULL;
    steam_achievements_available = false;
    SteamAPI_RunCallbacks = NULL;
    SteamInternal_ContextInit = NULL;
    SteamInternal_FindOrCreateUserInterface = NULL;
#endif
}

static TbBool steam_achievement_unlock(const char* achievement_id)
{
    if (!steam_achievements_available || !g_pSteamUserStats)
        return false;
    
    // Set achievement in Steam
    if (!g_pSteamUserStats->SetAchievement(achievement_id))
    {
        WARNLOG("Failed to set Steam achievement: %s", achievement_id);
        return false;
    }
    
    // Store stats to Steam servers
    if (!g_pSteamUserStats->StoreStats())
    {
        WARNLOG("Failed to store Steam stats");
        return false;
    }
    
    SYNCLOG("Steam achievement unlocked: %s", achievement_id);
    return true;
}

static TbBool steam_achievement_is_unlocked(const char* achievement_id)
{
    if (!steam_achievements_available || !g_pSteamUserStats)
        return false;
    
    bool achieved = false;
    if (!g_pSteamUserStats->GetAchievement(achievement_id, &achieved))
    {
        return false;
    }
    
    return achieved ? true : false;
}

static void steam_achievement_set_progress(const char* achievement_id, float progress)
{
    if (!steam_achievements_available || !g_pSteamUserStats)
        return;
    
    // Steam doesn't have native progress tracking for achievements
    // We could use stats to track progress, but for simplicity we just unlock at 100%
    if (progress >= 1.0f)
    {
        steam_achievement_unlock(achievement_id);
    }
}

static float steam_achievement_get_progress(const char* achievement_id)
{
    if (!steam_achievements_available || !g_pSteamUserStats)
        return 0.0f;
    
    // Steam achievements are binary (unlocked or not)
    bool achieved = false;
    if (g_pSteamUserStats->GetAchievement(achievement_id, &achieved))
    {
        return achieved ? 1.0f : 0.0f;
    }
    
    return 0.0f;
}

static void steam_achievement_sync(void)
{
    if (!steam_achievements_available)
        return;
    
    // Run Steam callbacks to process achievement notifications
    if (SteamAPI_RunCallbacks != NULL)
    {
        SteamAPI_RunCallbacks();
    }
}

/******************************************************************************/
// Backend registration

static struct AchievementBackend steam_backend = {
    "Steam",                           // name
    AchPlat_Steam,                    // platform_type
    steam_achievement_init,           // init
    steam_achievement_shutdown,       // shutdown
    steam_achievement_unlock,         // unlock
    steam_achievement_is_unlocked,    // is_unlocked
    steam_achievement_set_progress,   // set_progress
    steam_achievement_get_progress,   // get_progress
    steam_achievement_sync,           // sync
};

TbBool steam_achievements_register(void)
{
    return achievements_register_backend(&steam_backend);
}

TbBool steam_achievements_is_available(void)
{
    return steam_achievements_available;
}

/******************************************************************************/
