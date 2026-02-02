# Steam Development and Testing Guide for KeeperFX

## Overview

Yes! Steam provides a **Steamworks SDK** that allows developers to test Steam features (including achievements) locally during development, even before a game is released on Steam. This is specifically designed for this use case.

## How Steam Development Works

### 1. Steamworks Partner Account

**For Testing Purposes (Free):**
- Steam allows development and testing without a released game
- You can create a "Spaceworks test app" for free development
- Get your own App ID for development/testing
- Full access to achievements, stats, and other Steamworks features

**Process:**
1. Sign up at https://partner.steamgames.com/ (requires one-time $100 fee for publishing, but you can test for free)
2. OR use Steam's test AppID (480 - "Spacewar") for development/testing
3. Configure achievements in the Steamworks dashboard
4. Download Steamworks SDK

### 2. Using "Spacewar" (AppID 480) for Development

Steam provides a special test application called **Spacewar** (AppID 480) specifically for developers to test Steamworks features:

**Advantages:**
- ✅ Free - No partner account needed
- ✅ Full achievement support
- ✅ Can be used immediately
- ✅ Test all Steamworks features
- ✅ See achievements in Steam overlay

**Limitations:**
- ❌ Achievements won't transfer to your real app
- ❌ Shared by all developers (not private)
- ❌ Can't customize achievement definitions in dashboard

**Setup:**
```
1. Download Steamworks SDK
2. Create steam_appid.txt with content: 480
3. Place steam_api.dll in KeeperFX directory
4. Run Steam client
5. Launch KeeperFX - it will connect to Steam using AppID 480
```

### 3. Creating Your Own Development App

**For Proper Testing:**

1. **Register on Steamworks Partner**
   - Visit https://partner.steamgames.com/
   - Create account (one-time $100 fee for publishing, but you get it back in revenue)
   - Create a new app (doesn't need to be released)

2. **Configure Your App**
   - Get your unique AppID (e.g., 123456)
   - Set app name, description
   - Configure achievements in Steamworks dashboard
   - Set visibility to "Private" (only you can see it)

3. **Local Development Setup**
   ```
   keeperfx/
   ├── keeperfx.exe
   ├── steam_api.dll          # From Steamworks SDK
   └── steam_appid.txt        # Contains your AppID: 123456
   ```

4. **Test Achievements**
   - Achievements configured in dashboard appear in your game
   - Test unlocking during gameplay
   - View in Steam overlay (Shift+Tab)
   - Check achievement stats in Steamworks dashboard
   - Reset achievements for re-testing

## Development Workflow

### Initial Setup

1. **Download Steamworks SDK**
   ```
   https://partner.steamgames.com/downloads/steamworks_sdk.zip
   ```

2. **Extract Files**
   ```
   steamworks_sdk/
   ├── public/steam/          # Header files (optional, we don't need them)
   ├── redistributable_bin/
   │   ├── steam_api.dll      # Windows 32-bit
   │   ├── steam_api64.dll    # Windows 64-bit
   │   └── libsteam_api.so    # Linux
   └── tools/                 # ContentBuilder, etc.
   ```

3. **Copy to KeeperFX Directory**
   ```bash
   cp steam_api.dll /path/to/keeperfx/
   echo "480" > /path/to/keeperfx/steam_appid.txt  # For Spacewar testing
   ```

### Testing Achievements

#### Option 1: Use Spacewar (Quick Start)

```bash
# 1. Create steam_appid.txt
echo "480" > steam_appid.txt

# 2. Copy steam_api.dll
cp /path/to/steamworks_sdk/redistributable_bin/steam_api.dll .

# 3. Run Steam client (must be running)

# 4. Launch KeeperFX
./keeperfx.exe

# 5. Open Steam overlay (Shift+Tab) to see Spacewar achievements
```

**Testing in Code:**
```c
// KeeperFX will automatically connect to AppID 480
// Achievements defined in code will work, but won't show in overlay
// unless configured in Spacewar's dashboard (which you can't access)

// You can still test:
// - Steam connection
// - Achievement unlock calls
// - Local tracking
// - API integration
```

#### Option 2: Use Your Own App (Recommended)

**A. Configure Achievements in Steamworks Dashboard**

1. Log into https://partner.steamgames.com/
2. Select your app
3. Go to "Steamworks Admin" → "Stats & Achievements"
4. Click "Add New Stat" or "Add New Achievement"
5. Configure each achievement:
   - **API Name**: Must match your code (e.g., `keeporig.imp_army`)
   - **Display Name**: "Imp Army"
   - **Description**: "Complete level 5 using only Imps"
   - **Hidden**: Yes/No
   - **Icon**: Upload achievement icon image
6. Save and publish to "Default" branch

**B. Map KeeperFX Achievements to Steam**

Update `achievement_steam.cpp` to map achievement IDs:

```cpp
// Map campaign achievement ID to Steam API name
static const char* map_achievement_id_to_steam(const char* achievement_id)
{
    // Campaign achievements are namespaced: "campaign.achievement"
    // Steam wants API names like: "keeporig_imp_army"
    
    // Simple mapping: replace dots with underscores
    static char steam_id[128];
    strncpy(steam_id, achievement_id, sizeof(steam_id));
    
    // Replace '.' with '_'
    for (char* p = steam_id; *p; p++)
    {
        if (*p == '.')
            *p = '_';
    }
    
    return steam_id;
}
```

**C. Test Your Achievements**

```bash
# 1. Update steam_appid.txt with your AppID
echo "123456" > steam_appid.txt

# 2. Launch KeeperFX with Steam running
./keeperfx.exe

# 3. Play and unlock achievements

# 4. Check Steam overlay (Shift+Tab)
# - View achievement progress
# - See notifications when unlocked

# 5. Check Steamworks dashboard
# - View unlock statistics
# - See which accounts unlocked what
# - Reset for re-testing
```

### Debugging Steam Integration

**Enable Logging:**

Create `steam_appid.txt` with additional flags:
```
480
LogNetworkData=1
LogSteamworksCallback=1
```

**Check Logs:**
```bash
# Steam writes logs to:
# Windows: C:\Program Files (x86)\Steam\logs\
# Look for: connection_log.txt, console_log.txt

# KeeperFX logs:
tail -f keeperfx.log | grep -i "steam\|achievement"
```

**Common Issues:**

1. **"Steam API not initialized"**
   - Ensure Steam client is running
   - Check steam_api.dll is present
   - Verify steam_appid.txt exists and contains valid AppID

2. **Achievements not unlocking**
   - Check achievement is configured in Steamworks dashboard
   - Verify API name matches code
   - Check Steam overlay shows the achievement
   - Review keeperfx.log for errors

3. **"Failed to get Steam UserStats interface"**
   - Update steam_api.dll to latest version
   - Ensure Steam client is up to date
   - Try restarting Steam

### Resetting Achievements for Testing

**Via Steamworks Dashboard:**
1. Go to your app's "Stats & Achievements" page
2. Click "Reset Stats" button
3. Choose "All Users" or specific user
4. Confirm reset

**Via Code (Development Only):**
```cpp
// In achievement_steam.cpp, add debug function:
void steam_achievement_reset_all(void)
{
#ifdef _DEBUG
    if (!g_pSteamUserStats)
        return;
    
    // Clear all achievements
    for (int i = 0; i < achievements_count; i++)
    {
        g_pSteamUserStats->ClearAchievement(achievements[i].id);
    }
    
    g_pSteamUserStats->StoreStats();
    SYNCLOG("All achievements reset for testing");
#endif
}
```

## Recommended Development Setup for KeeperFX

### Phase 1: Local Development (No Steam)

**Use local storage for initial development:**

```c
// In main initialization
#ifdef DEVELOPMENT_MODE
    // Use local storage for testing
    current_platform = AchPlat_Local;
#endif
```

**Benefits:**
- No Steam dependency
- Faster iteration
- Works offline
- Easy to reset

### Phase 2: Steam Integration Testing (Spacewar)

**Use AppID 480 for basic Steam testing:**

```bash
echo "480" > steam_appid.txt
```

**Benefits:**
- Test Steam connection
- Verify API calls work
- See Steam overlay integration
- No configuration needed

### Phase 3: Full Steam Testing (Your App)

**Create development app on Steamworks:**

1. Register on Steamworks Partner
2. Create private app for KeeperFX
3. Configure all achievements in dashboard
4. Test complete integration

**Benefits:**
- Full achievement configuration
- Proper icons and descriptions
- Statistics tracking
- Private testing environment

## Configuration Examples

### For Development (steam_appid.txt)

**Using Spacewar (Quick Testing):**
```
480
```

**Using Your App (Full Testing):**
```
1234567
```

**With Debug Logging:**
```
480
LogNetworkData=1
```

### Achievement Configuration Mapping

**KeeperFX Config (achievements.cfg):**
```ini
ACHIEVEMENT imp_army
    NAME = "Imp Army"
    DESCRIPTION = "Complete level 5 using only Imps"
    POINTS = 25
    LEVEL = 5
    COMPLETE_LEVEL = 1
    ONLY_CREATURE = IMP
END_ACHIEVEMENT
```

**Steamworks Dashboard Configuration:**
```
API Name: keeporig_imp_army
Display Name: Imp Army
Description: Complete level 5 using only Imps
Hidden: No
Icon: imp_army.png (upload 64x64 image)
```

## Best Practices

### 1. Development Workflow

```
Local Testing → Spacewar Testing → Private App Testing → Public Release
     ↓                ↓                    ↓                    ↓
  No Steam        Basic Steam      Full Steam Config    Production
  Fastest         Quick verify     Complete testing     Live users
```

### 2. Achievement Naming Convention

**KeeperFX Internal:**
```
campaign.achievement_id
Example: keeporig.imp_army
```

**Steam API Name:**
```
campaign_achievement_id
Example: keeporig_imp_army
```

**Conversion in code:**
```cpp
const char* steam_id = map_achievement_id_to_steam("keeporig.imp_army");
// Returns: "keeporig_imp_army"
```

### 3. Testing Checklist

- [ ] Steam client running
- [ ] steam_api.dll present
- [ ] steam_appid.txt with correct AppID
- [ ] Achievement configured in Steamworks (if using your app)
- [ ] API names match between code and Steamworks
- [ ] Icons uploaded to Steamworks
- [ ] Test unlock in-game
- [ ] Verify in Steam overlay
- [ ] Check Steamworks stats dashboard
- [ ] Test achievement reset
- [ ] Test with Steam offline (should fallback to local)

## Summary

**Yes, you can absolutely develop and test Steam achievements locally!**

**Quick Start:**
1. Download Steamworks SDK
2. Use AppID 480 (Spacewar) for immediate testing
3. Create `steam_appid.txt` with `480`
4. Copy `steam_api.dll` to KeeperFX directory
5. Run Steam client
6. Launch KeeperFX and test

**For Production:**
1. Create Steamworks Partner account
2. Register KeeperFX as new app
3. Configure achievements in dashboard
4. Use your AppID instead of 480
5. Test complete integration

The implementation I provided already supports all of this - just follow the setup steps above!

## Additional Resources

- **Steamworks SDK Download**: https://partner.steamgames.com/downloads/steamworks_sdk.zip
- **Steamworks Documentation**: https://partner.steamgames.com/doc/sdk
- **Achievements Guide**: https://partner.steamgames.com/doc/features/achievements
- **Stats & Achievements**: https://partner.steamgames.com/doc/features/achievements/ach_guide
- **Testing Locally**: https://partner.steamgames.com/doc/sdk/api#initialization_and_shutdown
- **Spacewar Example**: Included in Steamworks SDK under `/sdk/tools/ContentBuilder/content/`

## Contact

For Steamworks-specific questions:
- Steamworks Developer Forums: https://steamcommunity.com/groups/steamworks/discussions
- Steamworks Support: https://partner.steamgames.com/support

For KeeperFX achievement implementation:
- Discord: https://discord.gg/hE4p7vy2Hb
- GitHub Issues: https://github.com/dkfans/keeperfx/issues
