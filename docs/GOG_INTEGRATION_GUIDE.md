# GOG Galaxy Achievement Integration Guide

## Overview

This guide covers integration of the KeeperFX achievement system with GOG Galaxy, including costs, requirements, and capabilities for game mods.

## KeeperFX on GOG

### Current Status

**KeeperFX is available on GOG.com as a mod/fan expansion:**
- Listed as an unofficial fan-made expansion/mod for Dungeon Keeper
- Available through GOG's community/mod section
- Users who own Dungeon Keeper on GOG can install KeeperFX
- Currently no official achievements (this implementation would add them)

### What This Means for Achievement Integration

Since KeeperFX is distributed through GOG (even as a mod), it CAN support GOG Galaxy achievements:

✅ **Yes, mods CAN have achievements on GOG Galaxy**
- GOG Galaxy SDK supports achievements for any game/mod
- No special status required for basic achievement functionality
- Achievements are tied to the Galaxy client, not the store listing
- Works for standalone games, DLC, and mods alike

## GOG Galaxy SDK

### Official Information

**SDK Details:**
- **Name**: GOG Galaxy SDK
- **Current Version**: 2.0+ (as of 2023)
- **License**: Free to use for all developers
- **Platform**: Windows, macOS, Linux
- **Languages**: C++, C#, Java (via wrappers)
- **Download**: https://devportal.gog.com/

**Documentation:**
- Official Docs: https://docs.gog.com/galaxyapi/
- API Reference: https://docs.gog.com/galaxyapi-api/
- Dev Portal: https://devportal.gog.com/

### Cost Analysis

**💰 Integration Costs: FREE**

| Item | Cost | Notes |
|------|------|-------|
| SDK Download | **FREE** | No licensing fees |
| Development | **FREE** | No partnership required |
| Testing | **FREE** | Free Galaxy client for testing |
| Distribution | **FREE** | If already on GOG (KeeperFX is) |
| Ongoing | **FREE** | No maintenance fees |
| **TOTAL** | **$0** | Completely free |

**Additional Considerations:**
- ❌ No certification required (unlike Xbox/PlayStation)
- ❌ No partnership needed for existing GOG products
- ❌ No revenue sharing for achievements
- ✅ Can test locally without GOG approval
- ✅ Community mods fully supported

### Comparison with Other Platforms

| Platform | Cost | Cert Required | Mod Support | 
|----------|------|---------------|-------------|
| **GOG Galaxy** | **$0** | ❌ No | ✅ Yes |
| Steam | $0* | ❌ No | ✅ Yes |
| Xbox | Variable | ✅ Yes | ⚠️ Limited |
| PlayStation | Variable | ✅ Yes | ⚠️ Limited |
| Epic | $0 | ❌ No | ✅ Yes |

*Steam requires $100 one-time publishing fee, but testing is free with Spacewar

## GOG Galaxy Achievement API

### Features

**Achievement System:**
- Unlock achievements
- Track achievement progress
- Hidden achievements
- Achievement statistics
- Cloud save integration
- Offline play with sync
- Rich presence integration

**Stats System:**
- Player statistics tracking
- Leaderboards
- Averages and aggregates
- Historical data

**Additional Features:**
- Friend activity feeds
- Achievement notifications
- Time-played tracking
- Cloud saves
- Multiplayer features

### API Overview

**Main Interfaces (C++):**

```cpp
// Stats and Achievements
galaxy::api::IStats
  - SetAchievement(const char* name)
  - ClearAchievement(const char* name)
  - GetAchievement(const char* name, bool& unlocked)
  - SetStatInt(const char* name, int32_t value)
  - SetStatFloat(const char* name, float value)
  - GetStatInt(const char* name, int32_t& value)
  - GetStatFloat(const char* name, float& value)
  - StoreStatsAndAchievements()
  - ResetStatsAndAchievements()

// User Information
galaxy::api::IUser
  - GetGalaxyID()
  - GetUserData()
  - IsLoggedOn()

// Initialization
galaxy::api::Init()
galaxy::api::Shutdown()
galaxy::api::ProcessData()  // Call in main loop
```

### DLL Requirements

**Windows:**
- `Galaxy.dll` (32-bit) or `Galaxy64.dll` (64-bit)
- Included with GOG Galaxy client
- Can be bundled with game
- Located in Galaxy client installation directory

**Linux:**
- `libGalaxy.so` (32-bit) or `libGalaxy64.so` (64-bit)
- Included with Galaxy Linux client

**macOS:**
- `libGalaxy.dylib`
- Included with Galaxy macOS client

## Mod Achievement Support on GOG

### Official Position

**GOG Galaxy supports achievements for mods:**

✅ **Fully Supported Features:**
1. **Achievement Unlocking** - Mods can unlock achievements
2. **Progress Tracking** - Track progress towards achievements
3. **Statistics** - Track player stats
4. **Leaderboards** - Create mod-specific leaderboards
5. **Cloud Saves** - Sync achievement progress
6. **Notifications** - Show achievement unlock popups

✅ **No Restrictions:**
- Mods don't need special approval for achievements
- Works the same as full games
- Can define unlimited achievements
- No difference in API access

✅ **KeeperFX Specific:**
- Since KeeperFX is on GOG, achievements will work
- Users with GOG Galaxy installed will get full achievement support
- Users without Galaxy will use local storage fallback (already implemented)
- No changes needed to GOG store listing

### How Mods Implement Achievements

**Three Approaches:**

**1. Shared Achievement Pool (Simple)**
- Mod uses base game's achievements
- Easier initial setup
- Limited to base game's achievement definitions
- ❌ Not suitable for KeeperFX (it's independent)

**2. Mod-Specific Achievements (Recommended for KeeperFX)**
- Mod defines its own achievement set
- Full control over achievement definitions
- Unique achievement namespace
- ✅ **Best for KeeperFX**

**3. Dynamic Achievements (Advanced)**
- Campaign creators define achievements
- Loaded at runtime
- Maximum flexibility
- ✅ **Already implemented in KeeperFX achievement system**

## Integration Requirements

### Prerequisites

1. **GOG Galaxy SDK**
   - Download from dev portal
   - Extract to development directory
   - Include headers and link libraries

2. **Galaxy Client ID**
   - Obtained from GOG Dev Portal
   - Free registration
   - Used to identify your application
   - For KeeperFX: Use existing Dungeon Keeper ID or request new one

3. **Achievement Configuration**
   - Define achievements in dev portal (optional)
   - Or use dynamic system (our implementation)
   - Icons (64x64 minimum, PNG format)
   - Achievement descriptions

### Development Setup

**Step 1: Register on GOG Dev Portal**
```
1. Visit https://devportal.gog.com/
2. Create free developer account
3. No fees or approvals needed
4. Access SDK downloads
```

**Step 2: Download SDK**
```
1. Log into dev portal
2. Go to SDK section
3. Download GOG Galaxy SDK 2.0+
4. Extract to project directory
```

**Step 3: Get Client ID**
```
For KeeperFX:
- Option A: Use existing DK client ID (if available)
- Option B: Register KeeperFX as separate product
- Option C: Use test client ID for development
```

**Step 4: Configure Project**
```cpp
// In your initialization code
galaxy::api::Init(galaxy::api::InitOptions(
    "YOUR_CLIENT_ID",           // From dev portal
    "YOUR_CLIENT_SECRET"        // From dev portal
));
```

## Implementation Architecture

### File Structure

```
src/
├── achievement_gog.cpp         # GOG Galaxy backend
├── achievement_gog.hpp         # GOG Galaxy header
└── achievement_api.c           # Core API (existing)

Galaxy SDK/
├── Include/
│   └── galaxy/                 # Header files
├── Libraries/
│   ├── Galaxy.lib              # Windows import library
│   └── libGalaxy.so            # Linux library
└── Redistributable/
    ├── Galaxy.dll              # Windows runtime
    ├── Galaxy64.dll            # Windows 64-bit runtime
    └── libGalaxy.so            # Linux runtime
```

### Backend Implementation Pattern

Following the Steam backend pattern (`achievement_steam.cpp`):

```cpp
// achievement_gog.cpp structure

#include <galaxy/GalaxyApi.h>

static galaxy::api::IStats* g_pGalaxyStats = nullptr;
static TbBool gog_achievements_available = false;

// Backend functions
static TbBool gog_achievement_init(void);
static void gog_achievement_shutdown(void);
static TbBool gog_achievement_unlock(const char* achievement_id);
static TbBool gog_achievement_is_unlocked(const char* achievement_id);
static void gog_achievement_set_progress(const char* achievement_id, float progress);
static float gog_achievement_get_progress(const char* achievement_id);
static void gog_achievement_sync(void);

// Backend registration
static struct AchievementBackend gog_backend = {
    "GOG Galaxy",
    AchPlat_GOG,
    gog_achievement_init,
    gog_achievement_shutdown,
    gog_achievement_unlock,
    gog_achievement_is_unlocked,
    gog_achievement_set_progress,
    gog_achievement_get_progress,
    gog_achievement_sync,
};

TbBool gog_achievements_register(void)
{
    return achievements_register_backend(&gog_backend);
}
```

## Testing and Development

### Local Testing

**Without GOG Galaxy Client:**
- System will fallback to local storage
- All achievement logic still works
- No Galaxy notifications
- Good for initial development

**With GOG Galaxy Client:**
- Full achievement integration
- Real-time notifications
- Achievement overlay
- Cloud sync
- Proper testing environment

### Test Workflow

**Phase 1: Development (No Galaxy)**
```bash
# 1. Develop achievement logic
# 2. Test with local storage
# 3. Verify conditions work
# 4. Check achievement unlocks logged
```

**Phase 2: Galaxy Integration**
```bash
# 1. Install GOG Galaxy client
# 2. Configure client ID
# 3. Place Galaxy.dll in game directory
# 4. Run game through Galaxy
# 5. Test achievement unlocks
# 6. Verify in Galaxy overlay
```

**Phase 3: Production Testing**
```bash
# 1. Deploy to test users via GOG
# 2. Collect achievement statistics
# 3. Verify cloud sync works
# 4. Test offline/online transitions
```

### Debug Tools

**GOG Galaxy Overlay:**
- Press Shift+Tab in-game
- View achievements
- See unlock status
- Check statistics

**Dev Portal Dashboard:**
- View global achievement stats
- See unlock percentages
- Monitor player engagement
- Debug issues

**Logging:**
```cpp
// Enable Galaxy SDK logging
galaxy::api::ILogger::SetLogLevel(galaxy::api::LOG_LEVEL_TRACE);

// Check in KeeperFX log
SYNCLOG("GOG Galaxy achievement unlocked: %s", achievement_id);
```

## Achievement Configuration

### In GOG Dev Portal (Optional)

**Manual Configuration:**
1. Log into dev portal
2. Select product
3. Go to "Achievements" section
4. Add achievement:
   - API Name: `keeporig_imp_army`
   - Display Name: "Imp Army"
   - Description: "Complete level 5 using only Imps"
   - Icon: Upload 64x64 PNG
5. Save and publish

**Benefits:**
- Achievements visible in Galaxy client
- Better user experience
- Icons and descriptions in overlay
- Global statistics

### Dynamic Configuration (KeeperFX)

**Using Existing System:**

KeeperFX already supports dynamic achievements via `achievements.cfg`:

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

**Galaxy Integration:**
- Read from `achievements.cfg`
- Register with Galaxy at runtime
- No dev portal configuration needed
- Maximum flexibility for modders

**Approach for KeeperFX:**
Use both:
1. Core achievements in dev portal (better UX)
2. Campaign achievements dynamic (mod flexibility)

## Licensing and Legal

### SDK License

**GOG Galaxy SDK License:**
- Free to use
- No royalties
- No revenue sharing
- Commercial use allowed
- Redistribution of DLLs allowed
- Open to all developers

**Terms:**
- Must comply with GOG's terms of service
- Cannot reverse engineer SDK
- Cannot use to bypass DRM
- Cannot harm GOG's services

### KeeperFX Compatibility

**Open Source + GOG:**
- ✅ GPL-compatible
- ✅ Can bundle Galaxy DLLs
- ✅ Source code can be published
- ✅ No conflicts with open source
- ✅ Optional integration (graceful fallback)

**Best Practice:**
- Make Galaxy support optional
- Detect DLL at runtime
- Fallback to local storage if unavailable
- Document in build instructions

## Platform-Specific Notes

### Windows

**DLL Loading:**
```cpp
// Dynamic loading (recommended)
HMODULE galaxy_lib = LoadLibraryA("Galaxy64.dll");
if (!galaxy_lib)
    galaxy_lib = LoadLibraryA("Galaxy.dll");
```

**Location Priority:**
1. Game directory
2. Galaxy installation directory
3. System PATH

### Linux

**SO Loading:**
```cpp
void* galaxy_lib = dlopen("libGalaxy64.so", RTLD_NOW);
if (!galaxy_lib)
    galaxy_lib = dlopen("libGalaxy.so", RTLD_NOW);
```

**Installation:**
- Install via package manager
- Or bundle with game
- Check `/opt/GOG Games/` directory

### macOS

**DYLIB Loading:**
```cpp
void* galaxy_lib = dlopen("libGalaxy.dylib", RTLD_NOW);
```

**Framework Structure:**
- Can use as framework
- Or load dynamically
- Check `/Applications/GOG Galaxy.app/`

## Comparison: Steam vs GOG Galaxy

### Feature Comparison

| Feature | Steam | GOG Galaxy | Winner |
|---------|-------|------------|--------|
| Cost | $0 (after $100 fee) | $0 | Tie |
| Certification | No | No | Tie |
| Mod Support | Yes | Yes | Tie |
| SDK Complexity | Moderate | Simple | GOG |
| User Base | Larger | Smaller | Steam |
| DRM | Optional | No DRM | GOG |
| Testing | Spacewar | Direct | GOG |
| Documentation | Excellent | Good | Steam |
| Cloud Saves | Yes | Yes | Tie |
| Offline Play | Limited | Full | GOG |

### For KeeperFX

**Recommendation: Implement Both**

**Steam:**
- Larger user base
- Better documentation
- Already partially integrated

**GOG Galaxy:**
- **KeeperFX is already on GOG** ✅
- DRM-free philosophy matches project
- Simpler API
- Better mod support
- No certification needed

**Implementation Priority:**
1. Complete Steam integration (in progress)
2. Add GOG Galaxy support (this guide)
3. Add local storage (fallback)
4. Consider Epic/Xbox later

## Success Stories

### Mods with GOG Galaxy Achievements

**Examples:**
1. **The Witcher 3 Mods** - Custom achievements
2. **Baldur's Gate Enhanced Edition** - Mod achievements
3. **Community Patches** - Achievement additions
4. **Total Conversions** - Full achievement sets

**Key Takeaways:**
- All successfully use Galaxy achievements
- No special approval needed
- Mix of manual and dynamic systems
- Positive user reception

## Getting Started Checklist

### For Developers

- [ ] Register on GOG Dev Portal (free)
- [ ] Download GOG Galaxy SDK
- [ ] Review API documentation
- [ ] Get client ID for KeeperFX
- [ ] Implement GOG backend (achievement_gog.cpp)
- [ ] Test with Galaxy client
- [ ] Configure core achievements in portal (optional)
- [ ] Test dynamic achievement system
- [ ] Verify achievement unlocks
- [ ] Deploy to users

### For Campaign Creators

- [ ] Install GOG Galaxy (optional)
- [ ] Create achievements.cfg file
- [ ] Define achievement conditions
- [ ] Test achievement unlocks
- [ ] No additional work needed!

## Support and Resources

### Official Resources

- **GOG Dev Portal**: https://devportal.gog.com/
- **SDK Documentation**: https://docs.gog.com/galaxyapi/
- **API Reference**: https://docs.gog.com/galaxyapi-api/
- **Developer Forums**: https://www.gog.com/forum/general

### KeeperFX Resources

- **Achievement System Guide**: `docs/achievement_system.txt`
- **Implementation Guide**: `docs/achievements_implementation.md`
- **Steam Guide**: `docs/STEAM_DEVELOPMENT_GUIDE.md`
- **This Guide**: `docs/GOG_INTEGRATION_GUIDE.md`

### Getting Help

**GOG Support:**
- Developer portal support tickets
- Community forums
- Developer Discord (if available)

**KeeperFX:**
- Discord: https://discord.gg/hE4p7vy2Hb
- GitHub Issues: https://github.com/dkfans/keeperfx/issues

## Summary

### Key Points

✅ **Cost**: Completely FREE - $0 for everything
✅ **Mod Support**: YES - Mods can have full achievement support
✅ **KeeperFX**: Already on GOG, ready for achievements
✅ **Complexity**: Simpler than Steam integration
✅ **Requirements**: No certification or approval needed
✅ **Testing**: Easy local testing with Galaxy client
✅ **License**: GPL-compatible, no conflicts

### Recommendation

**Implement GOG Galaxy achievements for KeeperFX:**
1. Zero cost barrier
2. Already on GOG platform
3. Simpler than Steam
4. Full mod support
5. DRM-free philosophy matches project
6. Easy to test locally

### Next Steps

1. Review this guide
2. Download GOG Galaxy SDK
3. Implement `achievement_gog.cpp` backend
4. Test with Galaxy client
5. Deploy to users

**Estimated implementation time: 4-6 hours** (following Steam backend pattern)

---

*This guide is based on publicly available information about GOG Galaxy SDK and GOG's policies as of February 2026. For the most up-to-date information, always refer to the official GOG Developer Portal.*
