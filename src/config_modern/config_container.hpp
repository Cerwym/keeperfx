/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_container.hpp
 *     Modern C++20 configuration container with RAII and dynamic allocation.
 * @par Purpose:
 *     Provides type-safe, unlimited-capacity storage for configuration items.
 *     Replaces fixed-size array limitations with std::vector and std::span.
 * @par Comment:
 *     Uses RAII principles to prevent memory leaks.
 *     Template-based for type safety and code reuse.
 * @author   KeeperFX Modernization Team
 * @date     29 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef KEEPERFX_CONFIG_CONTAINER_HPP
#define KEEPERFX_CONFIG_CONTAINER_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <span>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace keeperfx {
namespace config {

/**
 * @brief RAII-based container for configuration items with unlimited capacity.
 * 
 * Provides safe, dynamic storage for configuration data with bounds checking
 * and name-based lookup. Replaces fixed-size arrays (e.g., CREATURE_TYPES_MAX).
 * 
 * Key features:
 * - Unlimited capacity (scales with actual data)
 * - Automatic memory management (RAII)
 * - Bounds-checked access
 * - Name-based lookup
 * - Non-owning views via std::span
 * 
 * @tparam T Configuration item type (e.g., CreatureModelConfig)
 */
template<typename T>
class ConfigContainer {
private:
    std::vector<T> items_;                               // Dynamic storage
    std::vector<std::string> names_;                     // Item names
    std::unordered_map<std::string, size_t> name_index_; // Fast name lookup
    
public:
    // RAII: Automatic cleanup on destruction
    ConfigContainer() = default;
    ~ConfigContainer() = default;
    
    // Non-copyable (prevent accidental expensive copies)
    ConfigContainer(const ConfigContainer&) = delete;
    ConfigContainer& operator=(const ConfigContainer&) = delete;
    
    // Movable (efficient transfer of ownership)
    ConfigContainer(ConfigContainer&&) noexcept = default;
    ConfigContainer& operator=(ConfigContainer&&) noexcept = default;
    
    /**
     * @brief Get item at index with bounds checking.
     * @param index Item index
     * @return Reference to item
     * @throws std::out_of_range if index is invalid
     */
    T& at(size_t index) {
        if (index >= items_.size()) {
            throw std::out_of_range("ConfigContainer::at: index out of range");
        }
        return items_[index];
    }
    
    const T& at(size_t index) const {
        if (index >= items_.size()) {
            throw std::out_of_range("ConfigContainer::at: index out of range");
        }
        return items_[index];
    }
    
    /**
     * @brief Unchecked access (use only when bounds are guaranteed).
     * @param index Item index
     * @return Reference to item (undefined behavior if out of bounds)
     */
    T& operator[](size_t index) {
        return items_[index];
    }
    
    const T& operator[](size_t index) const {
        return items_[index];
    }
    
    /**
     * @brief Try to get item at index (safe, returns nullptr if invalid).
     * @param index Item index
     * @return Pointer to item or nullptr if out of bounds
     */
    T* try_get(size_t index) {
        return index < items_.size() ? &items_[index] : nullptr;
    }
    
    const T* try_get(size_t index) const {
        return index < items_.size() ? &items_[index] : nullptr;
    }
    
    /**
     * @brief Find item by name.
     * @param name Item name (case-sensitive)
     * @return Pointer to item or nullptr if not found
     */
    T* find_by_name(const std::string& name) {
        auto it = name_index_.find(name);
        return it != name_index_.end() ? &items_[it->second] : nullptr;
    }
    
    const T* find_by_name(const std::string& name) const {
        auto it = name_index_.find(name);
        return it != name_index_.end() ? &items_[it->second] : nullptr;
    }
    
    /**
     * @brief Find item by name (case-insensitive).
     * @param name Item name
     * @return Pointer to item or nullptr if not found
     */
    T* find_by_name_ci(const std::string& name) {
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        for (size_t i = 0; i < names_.size(); ++i) {
            std::string lower_stored = names_[i];
            std::transform(lower_stored.begin(), lower_stored.end(), lower_stored.begin(), ::tolower);
            if (lower_stored == lower_name) {
                return &items_[i];
            }
        }
        return nullptr;
    }
    
    const T* find_by_name_ci(const std::string& name) const {
        return const_cast<ConfigContainer*>(this)->find_by_name_ci(name);
    }
    
    /**
     * @brief Get index of named item.
     * @param name Item name
     * @return Index or std::nullopt if not found
     */
    std::optional<size_t> get_index(const std::string& name) const {
        auto it = name_index_.find(name);
        return it != name_index_.end() ? std::optional<size_t>(it->second) : std::nullopt;
    }
    
    /**
     * @brief Add new item (no capacity limits!).
     * @param item Item to add
     * @param name Item name (must be unique)
     * @return Index of added item
     * @throws std::invalid_argument if name already exists
     */
    size_t add(const T& item, const std::string& name) {
        if (name_index_.find(name) != name_index_.end()) {
            throw std::invalid_argument("ConfigContainer::add: name already exists: " + name);
        }
        
        size_t index = items_.size();
        items_.push_back(item);
        names_.push_back(name);
        name_index_[name] = index;
        return index;
    }
    
    /**
     * @brief Add new item with move semantics (efficient).
     * @param item Item to add (moved)
     * @param name Item name (must be unique)
     * @return Index of added item
     */
    size_t add(T&& item, const std::string& name) {
        if (name_index_.find(name) != name_index_.end()) {
            throw std::invalid_argument("ConfigContainer::add: name already exists: " + name);
        }
        
        size_t index = items_.size();
        items_.push_back(std::move(item));
        names_.push_back(name);
        name_index_[name] = index;
        return index;
    }
    
    /**
     * @brief Add or update item (creates if doesn't exist, updates if exists).
     * @param item Item to add/update
     * @param name Item name
     * @return Index of item
     */
    size_t add_or_update(const T& item, const std::string& name) {
        auto it = name_index_.find(name);
        if (it != name_index_.end()) {
            items_[it->second] = item;
            return it->second;
        }
        return add(item, name);
    }
    
    /**
     * @brief Reserve capacity (optimization to avoid reallocations).
     * @param capacity Expected number of items
     */
    void reserve(size_t capacity) {
        items_.reserve(capacity);
        names_.reserve(capacity);
        name_index_.reserve(capacity);
    }
    
    /**
     * @brief Get non-owning view of all items (C++20 std::span).
     * Zero-overhead - just a pointer and size.
     */
    std::span<T> items() {
        return {items_.data(), items_.size()};
    }
    
    std::span<const T> items() const {
        return {items_.data(), items_.size()};
    }
    
    /**
     * @brief Get all item names.
     */
    std::span<const std::string> names() const {
        return {names_.data(), names_.size()};
    }
    
    /**
     * @brief Get item name by index.
     */
    const std::string& get_name(size_t index) const {
        if (index >= names_.size()) {
            throw std::out_of_range("ConfigContainer::get_name: index out of range");
        }
        return names_[index];
    }
    
    /**
     * @brief Get number of items.
     */
    size_t size() const {
        return items_.size();
    }
    
    /**
     * @brief Check if container is empty.
     */
    bool empty() const {
        return items_.empty();
    }
    
    /**
     * @brief Clear all items.
     */
    void clear() {
        items_.clear();
        names_.clear();
        name_index_.clear();
    }
    
    /**
     * @brief Shrink allocated memory to fit current size.
     */
    void shrink_to_fit() {
        items_.shrink_to_fit();
        names_.shrink_to_fit();
    }
    
    // Iterator support (for range-based for loops)
    auto begin() { return items_.begin(); }
    auto end() { return items_.end(); }
    auto begin() const { return items_.begin(); }
    auto end() const { return items_.end(); }
    auto cbegin() const { return items_.cbegin(); }
    auto cend() const { return items_.cend(); }
};

} // namespace config
} // namespace keeperfx

#endif // KEEPERFX_CONFIG_CONTAINER_HPP
