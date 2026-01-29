/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_loader.hpp
 *     Modern C++20 configuration loader base class.
 * @par Purpose:
 *     Provides parsing framework for configuration files.
 *     Base class for all specific config loaders.
 * @par Comment:
 *     Uses RAII for file handling.
 *     Template-based for type safety.
 * @author   KeeperFX Modernization Team
 * @date     29 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef KEEPERFX_CONFIG_LOADER_HPP
#define KEEPERFX_CONFIG_LOADER_HPP

#include "config_container.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <cstdio>
#include <cstdlib>

// Include C compatibility headers
extern "C" {
    #include "../bflib_basics.h"
    #include "../bflib_fileio.h"
}

namespace keeperfx {
namespace config {

/**
 * @brief Configuration loading flags.
 */
enum class LoadFlags : unsigned short {
    Standard = 0x00,      /**< Standard load, no special behavior */
    ListOnly = 0x01,      /**< Load only list of items and names, don't parse options */
    AcceptPartial = 0x02, /**< Accept partial files, don't clear previous config */
    IgnoreErrors = 0x04   /**< Don't log error messages on failures */
};

inline LoadFlags operator|(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<unsigned short>(a) | static_cast<unsigned short>(b));
}

inline LoadFlags operator&(LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(static_cast<unsigned short>(a) & static_cast<unsigned short>(b));
}

inline bool has_flag(LoadFlags flags, LoadFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief RAII wrapper for file buffer.
 */
class FileBuffer {
private:
    std::unique_ptr<char[]> data_;
    size_t size_;
    
public:
    FileBuffer() : size_(0) {}
    
    explicit FileBuffer(size_t size) : data_(new char[size + 1]), size_(size) {
        data_[size] = '\0'; // Null terminate
    }
    
    char* data() { return data_.get(); }
    const char* data() const { return data_.get(); }
    size_t size() const { return size_; }
    
    std::string_view view() const {
        return {data_.get(), size_};
    }
    
    operator bool() const { return data_ != nullptr; }
};

/**
 * @brief Base class for configuration loaders.
 * 
 * Provides common functionality for loading and parsing configuration files.
 * Derived classes implement domain-specific parsing logic.
 */
class ConfigLoaderBase {
protected:
    std::string config_name_;                 /**< Configuration name (e.g., "creature") */
    std::filesystem::path config_path_;       /**< Full path to config file */
    unsigned long text_line_number_;          /**< Current line being parsed */
    
    /**
     * @brief Load file into memory buffer (RAII-safe).
     * @param path File path
     * @return File buffer or empty on error
     */
    FileBuffer load_file(const std::filesystem::path& path) {
        // Get file size
        FILE* fp = fopen(path.string().c_str(), "rb");
        if (!fp) {
            return FileBuffer();
        }
        
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        if (file_size <= 0) {
            fclose(fp);
            return FileBuffer();
        }
        
        // Allocate buffer with extra space for null terminator
        FileBuffer buffer(static_cast<size_t>(file_size));
        
        // Read file
        size_t bytes_read = fread(buffer.data(), 1, static_cast<size_t>(file_size), fp);
        fclose(fp);
        
        if (bytes_read != static_cast<size_t>(file_size)) {
            return FileBuffer();
        }
        
        return buffer;
    }
    
    /**
     * @brief Skip whitespace in buffer.
     */
    bool skip_spaces(std::string_view& buffer) {
        while (!buffer.empty() && 
               (buffer[0] == ' ' || buffer[0] == '\t' || buffer[0] == '\r' || 
                buffer[0] == '\n' || buffer[0] == 26 || static_cast<unsigned char>(buffer[0]) < 7)) {
            if (buffer[0] == '\n') {
                text_line_number_++;
            }
            buffer.remove_prefix(1);
        }
        return !buffer.empty();
    }
    
    /**
     * @brief Skip to next line.
     */
    bool skip_to_next_line(std::string_view& buffer) {
        // Skip to end of line
        while (!buffer.empty() && buffer[0] != '\r' && buffer[0] != '\n') {
            buffer.remove_prefix(1);
        }
        // Skip line terminators
        while (!buffer.empty() && (buffer[0] == '\r' || buffer[0] == '\n')) {
            if (buffer[0] == '\n') {
                text_line_number_++;
            }
            buffer.remove_prefix(1);
        }
        return !buffer.empty();
    }
    
    /**
     * @brief Find INI-style block (e.g., "[CREATURE1]").
     */
    bool find_block(std::string_view& buffer, const std::string& block_name) {
        text_line_number_ = 1;
        
        while (!buffer.empty()) {
            skip_spaces(buffer);
            if (buffer.empty()) break;
            
            // Check for block start
            if (buffer[0] != '[') {
                skip_to_next_line(buffer);
                continue;
            }
            buffer.remove_prefix(1); // Skip '['
            
            skip_spaces(buffer);
            
            // Check block name
            if (buffer.size() >= block_name.size() &&
                buffer.substr(0, block_name.size()) == block_name) {
                buffer.remove_prefix(block_name.size());
                skip_spaces(buffer);
                if (!buffer.empty() && buffer[0] == ']') {
                    buffer.remove_prefix(1);
                    skip_to_next_line(buffer);
                    return true;
                }
            }
            skip_to_next_line(buffer);
        }
        return false;
    }
    
    /**
     * @brief Parse a line into key-value pair.
     * @return true if valid line, false if comment/empty/block end
     */
    bool parse_line(std::string_view& buffer, std::string& key, std::string& value) {
        skip_spaces(buffer);
        if (buffer.empty()) return false;
        
        // Check for comment or block end
        if (buffer[0] == ';' || buffer[0] == '[') {
            return false;
        }
        
        // Extract key
        key.clear();
        while (!buffer.empty() && buffer[0] != '=' && buffer[0] != ' ' && 
               buffer[0] != '\t' && buffer[0] != '\r' && buffer[0] != '\n') {
            key += buffer[0];
            buffer.remove_prefix(1);
        }
        
        if (key.empty()) {
            skip_to_next_line(buffer);
            return false;
        }
        
        // Skip to '='
        while (!buffer.empty() && (buffer[0] == ' ' || buffer[0] == '\t' || buffer[0] == '=')) {
            buffer.remove_prefix(1);
        }
        
        // Extract value
        value.clear();
        while (!buffer.empty() && buffer[0] != '\r' && buffer[0] != '\n') {
            value += buffer[0];
            buffer.remove_prefix(1);
        }
        
        // Trim trailing spaces from value
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        
        skip_to_next_line(buffer);
        return true;
    }
    
public:
    virtual ~ConfigLoaderBase() = default;
    
    /**
     * @brief Load configuration (pure virtual - implement in derived class).
     * @param flags Loading flags
     * @return true on success, false on error
     */
    virtual bool load(LoadFlags flags = LoadFlags::Standard) = 0;
    
    /**
     * @brief Reset/clear configuration.
     */
    virtual void reset() = 0;
    
    /**
     * @brief Get configuration name.
     */
    const std::string& get_name() const {
        return config_name_;
    }
    
    /**
     * @brief Get current line number (for error reporting).
     */
    unsigned long get_line_number() const {
        return text_line_number_;
    }
};

/**
 * @brief Template base class for typed configuration loaders.
 * 
 * Provides typed access to configuration container.
 * 
 * @tparam T Configuration item type
 */
template<typename T>
class ConfigLoader : public ConfigLoaderBase {
protected:
    ConfigContainer<T>& container_; /**< Reference to configuration container */
    
public:
    explicit ConfigLoader(ConfigContainer<T>& container, const std::string& name)
        : container_(container) {
        config_name_ = name;
        text_line_number_ = 0;
    }
    
    /**
     * @brief Get configuration container.
     */
    ConfigContainer<T>& get_container() {
        return container_;
    }
    
    const ConfigContainer<T>& get_container() const {
        return container_;
    }
    
    /**
     * @brief Reset configuration.
     */
    void reset() override {
        container_.clear();
    }
};

} // namespace config
} // namespace keeperfx

#endif // KEEPERFX_CONFIG_LOADER_HPP
