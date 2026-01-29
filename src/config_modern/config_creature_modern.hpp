/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_creature_modern.hpp
 *     Modern creature configuration loader using RAII and dynamic allocation.
 * @par Purpose:
 *     Demonstrates migration from fixed-size arrays to unlimited capacity.
 *     Pilot implementation for creature configuration domain.
 * @par Comment:
 *     Replaces CREATURE_TYPES_MAX limit with dynamic std::vector storage.
 * @author   KeeperFX Modernization Team
 * @date     29 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef KEEPERFX_CONFIG_CREATURE_MODERN_HPP
#define KEEPERFX_CONFIG_CREATURE_MODERN_HPP

#include "config_container.hpp"
#include "config_loader.hpp"
#include <string>
#include <string_view>
#include <charconv>

// Include C headers for data structures
extern "C" {
    #include "../config_creature.h"
    #include "../globals.h"
}

namespace keeperfx {
namespace config {

/**
 * @brief Modern creature configuration loader.
 * 
 * Migrates creature configuration from fixed-size arrays to dynamic storage.
 * 
 * Old system limitations:
 * - CREATURE_TYPES_MAX = 128 (hard limit)
 * - Fixed array: CreatureModelConfig model[128]
 * - Wasted memory if fewer creatures used
 * - Cannot add more creatures for mods
 * 
 * New system benefits:
 * - Unlimited creatures (only limited by RAM)
 * - Memory scales with actual usage
 * - RAII-safe resource management
 * - Bounds-checked access
 */
class CreatureConfigLoader : public ConfigLoader<CreatureModelConfig> {
private:
    /**
     * @brief Parse integer value safely.
     */
    template<typename T>
    bool parse_int(const std::string& str, T& value) {
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        return ec == std::errc();
    }
    
    /**
     * @brief Parse creature model configuration block.
     * 
     * Example format:
     * [CREATURE1]
     * Name = IMP
     * Health = 250
     * Strength = 20
     * ...
     */
    bool parse_creature_block(std::string_view& buffer, CreatureModelConfig& creature) {
        text_line_number_++;
        
        std::string key, value;
        while (parse_line(buffer, key, value)) {
            if (key.empty()) continue;
            
            // Parse creature properties
            if (key == "Name") {
                strncpy(creature.name, value.c_str(), COMMAND_WORD_LEN - 1);
                creature.name[COMMAND_WORD_LEN - 1] = '\0';
            }
            else if (key == "Health") {
                parse_int(value, creature.health);
            }
            else if (key == "Strength") {
                parse_int(value, creature.strength);
            }
            else if (key == "Armour") {
                parse_int(value, creature.armour);
            }
            else if (key == "Dexterity") {
                parse_int(value, creature.dexterity);
            }
            else if (key == "Defense") {
                parse_int(value, creature.defense);
            }
            else if (key == "Luck") {
                parse_int(value, creature.luck);
            }
            else if (key == "Speed") {
                parse_int(value, creature.base_speed);
            }
            else if (key == "Flying") {
                creature.flying = (value == "1" || value == "TRUE" || value == "YES");
            }
            // ... More properties would be parsed here ...
            // This is a simplified example showing the pattern
        }
        
        return true;
    }
    
public:
    explicit CreatureConfigLoader(ConfigContainer<CreatureModelConfig>& container, 
                                  const std::filesystem::path& config_path)
        : ConfigLoader(container, "creature.cfg") {
        config_path_ = config_path;
    }
    
    /**
     * @brief Load creature configuration file.
     * 
     * Parses creature.cfg and populates container with unlimited creatures.
     * 
     * Old code would reject creatures beyond CREATURE_TYPES_MAX:
     * ```c
     * if (n+1 >= CREATURE_TYPES_MAX) {
     *     WARNLOG("Cannot add creature, at maximum");
     *     return false; // Configuration silently dropped!
     * }
     * ```
     * 
     * New code has no such limit:
     * ```cpp
     * container_.add(creature, creature.name); // Always succeeds (or throws on OOM)
     * ```
     */
    bool load(LoadFlags flags = LoadFlags::Standard) override {
        // Clear existing data unless accepting partial
        if (!has_flag(flags, LoadFlags::AcceptPartial)) {
            container_.clear();
        }
        
        // Load file into memory buffer
        FileBuffer buffer = load_file(config_path_);
        if (!buffer) {
            if (!has_flag(flags, LoadFlags::IgnoreErrors)) {
                // Log error using C API
                extern "C" void LbErrorLog(const char* format, ...);
                LbErrorLog("Cannot load creature config: %s", config_path_.string().c_str());
            }
            return false;
        }
        
        // Parse configuration
        std::string_view view = buffer.view();
        text_line_number_ = 0;
        
        // Parse each creature block
        int creature_num = 1;
        while (!view.empty()) {
            // Find next creature block
            std::string block_name = "CREATURE" + std::to_string(creature_num);
            if (!find_block(view, block_name)) {
                break; // No more creatures
            }
            
            // Parse creature properties
            CreatureModelConfig creature = {};  // Zero-initialize
            if (parse_creature_block(view, creature)) {
                // Add to container - NO LIMIT CHECK!
                // Old system: would fail at creature #128
                // New system: unlimited (only RAM is the limit)
                try {
                    container_.add(creature, creature.name);
                } catch (const std::invalid_argument& e) {
                    if (!has_flag(flags, LoadFlags::IgnoreErrors)) {
                        extern "C" void LbWarnLog(const char* format, ...);
                        LbWarnLog("Duplicate creature name: %s", creature.name);
                    }
                }
            }
            
            creature_num++;
        }
        
        // Log success
        if (!has_flag(flags, LoadFlags::IgnoreErrors)) {
            extern "C" void LbSyncLog(const char* format, ...);
            LbSyncLog("Loaded %zu creatures (old limit was 128)", container_.size());
        }
        
        return true;
    }
};

/**
 * @brief Global configuration system (replaces global arrays).
 * 
 * Old system:
 * ```c
 * extern struct CreatureConfig game.conf.crtr_conf;
 * game.conf.crtr_conf.model[128]; // Fixed array
 * ```
 * 
 * New system:
 * ```cpp
 * extern CreatureConfigSystem g_creature_config;
 * g_creature_config.get_creatures(); // std::span<const CreatureModelConfig>
 * ```
 */
class CreatureConfigSystem {
private:
    ConfigContainer<CreatureModelConfig> creatures_;
    std::unique_ptr<CreatureConfigLoader> loader_;
    
public:
    CreatureConfigSystem() = default;
    
    /**
     * @brief Initialize configuration system.
     */
    void initialize(const std::filesystem::path& config_path) {
        loader_ = std::make_unique<CreatureConfigLoader>(creatures_, config_path);
    }
    
    /**
     * @brief Load all creature configuration.
     */
    bool load_all(LoadFlags flags = LoadFlags::Standard) {
        if (!loader_) {
            return false;
        }
        return loader_->load(flags);
    }
    
    /**
     * @brief Get all creatures (read-only view).
     */
    std::span<const CreatureModelConfig> get_creatures() const {
        return creatures_.items();
    }
    
    /**
     * @brief Get creature by index (bounds-checked).
     */
    const CreatureModelConfig* get_creature(size_t index) const {
        return creatures_.try_get(index);
    }
    
    /**
     * @brief Find creature by name.
     */
    const CreatureModelConfig* find_creature(const std::string& name) const {
        return creatures_.find_by_name(name);
    }
    
    /**
     * @brief Get creature count (no more CREATURE_TYPES_MAX!).
     */
    size_t get_creature_count() const {
        return creatures_.size();
    }
    
    /**
     * @brief Reset all creatures.
     */
    void reset() {
        creatures_.clear();
    }
    
    // C API compatibility layer
    // These functions allow old C code to work with new system
    
    /**
     * @brief C-compatible accessor (for migration period).
     * 
     * Usage in old C code:
     * ```c
     * extern "C" struct CreatureModelConfig* creature_stats_get_modern(int idx);
     * CreatureModelConfig* cfg = creature_stats_get_modern(idx);
     * ```
     */
    CreatureModelConfig* get_creature_mutable(size_t index) {
        return creatures_.try_get(index);
    }
};

} // namespace config
} // namespace keeperfx

// C API compatibility functions (for gradual migration)
extern "C" {

// Global instance (replaces old game.conf.crtr_conf)
// In implementation file: keeperfx::config::CreatureConfigSystem g_creature_config_modern;

/**
 * @brief Get creature configuration (modern system).
 * 
 * Compatibility wrapper for old code.
 */
// struct CreatureModelConfig* creature_stats_get_modern(int idx);

} // extern "C"

#endif // KEEPERFX_CONFIG_CREATURE_MODERN_HPP
