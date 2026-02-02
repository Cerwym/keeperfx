/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file achievement_gog.cpp
 *     GOG Galaxy achievement backend implementation.
 * @par Purpose:
 *     Implements achievement support using GOG Galaxy SDK.
 * @par Comment:
 *     Provides GOG Galaxy-specific achievement unlock and tracking.
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
#include "achievement_gog.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include "achievement_api.h"
#include "bflib_fileio.h"
#include "keeperfx.hpp"
#include "post_inc.h"

/******************************************************************************/
// GOG Galaxy SDK type definitions
// These match the GOG Galaxy SDK but are defined here to avoid requiring the full SDK

// Forward declarations for Galaxy API interfaces
namespace galaxy {
namespace api {

// Stats and Achievements interface
class IStats
{
public:
    virtual ~IStats() {}
    
    virtual void SetAchievement(const char* name) = 0;
    virtual void ClearAchievement(const char* name) = 0;
    virtual bool GetAchievement(const char* name, bool& unlocked, uint32_t& unlockTime, float& progress) = 0;
    
    virtual void SetStatInt(const char* name, int32_t value) = 0;
    virtual void SetStatFloat(const char* name, float value) = 0;
    virtual void UpdateAvgRateStat(const char* name, float countThisSession, double sessionLength) = 0;
    
    virtual int32_t GetStatInt(const char* name) = 0;
    virtual float GetStatFloat(const char* name) = 0;
    
    virtual void StoreStatsAndAchievements() = 0;
    virtual void ResetStatsAndAchievements() = 0;
};

// User interface
class IUser
{
public:
    virtual ~IUser() {}
    virtual bool IsLoggedOn() = 0;
};

// Initialization options
struct InitOptions
{
    const char* clientID;
    const char* clientSecret;
    
    InitOptions(const char* id, const char* secret)
        : clientID(id), clientSecret(secret) {}
};

// API functions
typedef void (*InitFunc)(const InitOptions& options);
typedef void (*ShutdownFunc)();
typedef void (*ProcessDataFunc)();
typedef IStats* (*StatsFunc)();
typedef IUser* (*UserFunc)();

} // namespace api
} // namespace galaxy

/******************************************************************************/
// Function pointer types for dynamically loaded GOG Galaxy API
typedef void (*GalaxyAPI_Init_t)(const galaxy::api::InitOptions& options);
typedef void (*GalaxyAPI_Shutdown_t)();
typedef void (*GalaxyAPI_ProcessData_t)();
typedef galaxy::api::IStats* (*GalaxyAPI_Stats_t)();
typedef galaxy::api::IUser* (*GalaxyAPI_User_t)();

// Global GOG Galaxy API function pointers
static GalaxyAPI_Init_t GalaxyAPI_Init = NULL;
static GalaxyAPI_Shutdown_t GalaxyAPI_Shutdown = NULL;
static GalaxyAPI_ProcessData_t GalaxyAPI_ProcessData = NULL;
static GalaxyAPI_Stats_t GalaxyAPI_Stats = NULL;
static GalaxyAPI_User_t GalaxyAPI_User = NULL;

// Galaxy interfaces
static galaxy::api::IStats* g_pGalaxyStats = NULL;
static galaxy::api::IUser* g_pGalaxyUser = NULL;

#ifdef _WIN32
static HMODULE gog_lib = NULL;
#endif

static TbBool gog_achievements_available = false;

// Client ID configuration (should be loaded from config file in production)
#define GOG_CLIENT_ID "keeperfx_client_id"
#define GOG_CLIENT_SECRET "keeperfx_client_secret"

/******************************************************************************/
// GOG achievement backend implementation

static TbBool gog_achievement_init(void)
{
#ifndef _WIN32
    // TODO: Add Linux/macOS support
    JUSTLOG("GOG Galaxy not supported on this platform yet");
    return false;
#else
    // Check if Galaxy DLL is available
    if (!LbFileExists("Galaxy64.dll") && !LbFileExists("Galaxy.dll"))
    {
        JUSTLOG("GOG Galaxy DLL not found, achievements unavailable");
        return false;
    }
    
    // Load Galaxy library (try 64-bit first)
    gog_lib = LoadLibraryA("Galaxy64.dll");
    if (!gog_lib)
    {
        gog_lib = LoadLibraryA("Galaxy.dll");
    }
    
    if (!gog_lib)
    {
        WARNLOG("Failed to load GOG Galaxy DLL");
        return false;
    }
    
    // Get function pointers
    // Note: These are simplified - real Galaxy SDK uses different function names
    // In production, you would use the actual Galaxy SDK headers
    GalaxyAPI_Init = reinterpret_cast<GalaxyAPI_Init_t>(
        GetProcAddress(gog_lib, "GalaxyAPI_Init"));
    
    GalaxyAPI_Shutdown = reinterpret_cast<GalaxyAPI_Shutdown_t>(
        GetProcAddress(gog_lib, "GalaxyAPI_Shutdown"));
    
    GalaxyAPI_ProcessData = reinterpret_cast<GalaxyAPI_ProcessData_t>(
        GetProcAddress(gog_lib, "GalaxyAPI_ProcessData"));
    
    GalaxyAPI_Stats = reinterpret_cast<GalaxyAPI_Stats_t>(
        GetProcAddress(gog_lib, "GalaxyAPI_Stats"));
    
    GalaxyAPI_User = reinterpret_cast<GalaxyAPI_User_t>(
        GetProcAddress(gog_lib, "GalaxyAPI_User"));
    
    if (!GalaxyAPI_Init || !GalaxyAPI_Shutdown || !GalaxyAPI_ProcessData)
    {
        ERRORLOG("Failed to get GOG Galaxy API function pointers");
        FreeLibrary(gog_lib);
        gog_lib = NULL;
        return false;
    }
    
    // Initialize Galaxy API
    // In production, load client ID and secret from config file
    try
    {
        galaxy::api::InitOptions options(GOG_CLIENT_ID, GOG_CLIENT_SECRET);
        GalaxyAPI_Init(options);
    }
    catch (...)
    {
        ERRORLOG("Failed to initialize GOG Galaxy API");
        FreeLibrary(gog_lib);
        gog_lib = NULL;
        return false;
    }
    
    // Get Stats interface
    if (GalaxyAPI_Stats)
    {
        g_pGalaxyStats = GalaxyAPI_Stats();
    }
    
    // Get User interface
    if (GalaxyAPI_User)
    {
        g_pGalaxyUser = GalaxyAPI_User();
    }
    
    if (!g_pGalaxyStats)
    {
        WARNLOG("Failed to get GOG Galaxy Stats interface");
        GalaxyAPI_Shutdown();
        FreeLibrary(gog_lib);
        gog_lib = NULL;
        return false;
    }
    
    // Check if user is logged on
    if (g_pGalaxyUser && !g_pGalaxyUser->IsLoggedOn())
    {
        WARNLOG("User not logged into GOG Galaxy");
        // Continue anyway - achievements will be stored locally and synced later
    }
    
    gog_achievements_available = true;
    SYNCLOG("GOG Galaxy achievements initialized successfully");
    return true;
#endif
}

static void gog_achievement_shutdown(void)
{
#ifdef _WIN32
    if (GalaxyAPI_Shutdown != NULL)
    {
        GalaxyAPI_Shutdown();
    }
    
    if (gog_lib != NULL)
    {
        FreeLibrary(gog_lib);
        gog_lib = NULL;
    }
    
    g_pGalaxyStats = NULL;
    g_pGalaxyUser = NULL;
    gog_achievements_available = false;
    GalaxyAPI_Init = NULL;
    GalaxyAPI_Shutdown = NULL;
    GalaxyAPI_ProcessData = NULL;
    GalaxyAPI_Stats = NULL;
    GalaxyAPI_User = NULL;
#endif
}

static TbBool gog_achievement_unlock(const char* achievement_id)
{
    if (!gog_achievements_available || !g_pGalaxyStats)
        return false;
    
    try
    {
        // Set achievement in Galaxy
        g_pGalaxyStats->SetAchievement(achievement_id);
        
        // Store stats and achievements to Galaxy servers
        g_pGalaxyStats->StoreStatsAndAchievements();
        
        SYNCLOG("GOG Galaxy achievement unlocked: %s", achievement_id);
        return true;
    }
    catch (...)
    {
        WARNLOG("Failed to unlock GOG Galaxy achievement: %s", achievement_id);
        return false;
    }
}

static TbBool gog_achievement_is_unlocked(const char* achievement_id)
{
    if (!gog_achievements_available || !g_pGalaxyStats)
        return false;
    
    try
    {
        bool unlocked = false;
        uint32_t unlockTime = 0;
        float progress = 0.0f;
        
        if (g_pGalaxyStats->GetAchievement(achievement_id, unlocked, unlockTime, progress))
        {
            return unlocked ? true : false;
        }
    }
    catch (...)
    {
        WARNLOG("Failed to check GOG Galaxy achievement: %s", achievement_id);
    }
    
    return false;
}

static void gog_achievement_set_progress(const char* achievement_id, float progress)
{
    if (!gog_achievements_available || !g_pGalaxyStats)
        return;
    
    // GOG Galaxy supports achievement progress natively
    // We use a stat to track progress
    try
    {
        char stat_name[128];
        snprintf(stat_name, sizeof(stat_name), "%s_progress", achievement_id);
        
        g_pGalaxyStats->SetStatFloat(stat_name, progress);
        
        // Auto-unlock if progress reaches 100%
        if (progress >= 1.0f)
        {
            gog_achievement_unlock(achievement_id);
        }
        else
        {
            // Store progress
            g_pGalaxyStats->StoreStatsAndAchievements();
        }
    }
    catch (...)
    {
        WARNLOG("Failed to set GOG Galaxy achievement progress: %s", achievement_id);
    }
}

static float gog_achievement_get_progress(const char* achievement_id)
{
    if (!gog_achievements_available || !g_pGalaxyStats)
        return 0.0f;
    
    try
    {
        bool unlocked = false;
        uint32_t unlockTime = 0;
        float progress = 0.0f;
        
        if (g_pGalaxyStats->GetAchievement(achievement_id, unlocked, unlockTime, progress))
        {
            if (unlocked)
                return 1.0f;
            
            return progress;
        }
    }
    catch (...)
    {
        WARNLOG("Failed to get GOG Galaxy achievement progress: %s", achievement_id);
    }
    
    return 0.0f;
}

static void gog_achievement_sync(void)
{
    if (!gog_achievements_available)
        return;
    
    // Process Galaxy callbacks
    if (GalaxyAPI_ProcessData != NULL)
    {
        try
        {
            GalaxyAPI_ProcessData();
        }
        catch (...)
        {
            // Ignore callback processing errors
        }
    }
}

/******************************************************************************/
// Backend registration

static struct AchievementBackend gog_backend = {
    "GOG Galaxy",                      // name
    AchPlat_GOG,                       // platform_type
    gog_achievement_init,              // init
    gog_achievement_shutdown,          // shutdown
    gog_achievement_unlock,            // unlock
    gog_achievement_is_unlocked,       // is_unlocked
    gog_achievement_set_progress,      // set_progress
    gog_achievement_get_progress,      // get_progress
    gog_achievement_sync,              // sync
};

TbBool gog_achievements_register(void)
{
    return achievements_register_backend(&gog_backend);
}

TbBool gog_achievements_is_available(void)
{
    return gog_achievements_available;
}

/******************************************************************************/
