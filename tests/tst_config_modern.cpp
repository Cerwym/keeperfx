/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file tst_config_modern.cpp
 *     Unit tests for modern configuration system.
 * @par Purpose:
 *     Tests ConfigContainer and ConfigLoader functionality.
 * @author   KeeperFX Modernization Team
 * @date     29 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/

#include "tst_main.h"
#include "../src/config_modern/config_container.hpp"
#include "../src/config_modern/config_loader.hpp"
#include <cstring>

using namespace keeperfx::config;

// Simple test structure
struct TestConfig {
    char name[32];
    int value;
    float scale;
};

//=============================================================================
// ConfigContainer Tests
//=============================================================================

ADD_TEST(test_config_container_empty) {
    ConfigContainer<TestConfig> container;
    
    CU_ASSERT_EQUAL(container.size(), 0);
    CU_ASSERT_TRUE(container.empty());
    CU_ASSERT_EQUAL(container.try_get(0), nullptr);
}

ADD_TEST(test_config_container_add_single) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item;
    strcpy(item.name, "TestItem");
    item.value = 42;
    item.scale = 1.5f;
    
    size_t idx = container.add(item, "TestItem");
    
    CU_ASSERT_EQUAL(idx, 0);
    CU_ASSERT_EQUAL(container.size(), 1);
    CU_ASSERT_FALSE(container.empty());
    
    const TestConfig* retrieved = container.try_get(0);
    CU_ASSERT_PTR_NOT_NULL(retrieved);
    CU_ASSERT_STRING_EQUAL(retrieved->name, "TestItem");
    CU_ASSERT_EQUAL(retrieved->value, 42);
    CU_ASSERT_DOUBLE_EQUAL(retrieved->scale, 1.5f, 0.001);
}

ADD_TEST(test_config_container_add_multiple) {
    ConfigContainer<TestConfig> container;
    
    // Add 10 items
    for (int i = 0; i < 10; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Item%d", i);
        item.value = i * 10;
        item.scale = i * 0.1f;
        
        size_t idx = container.add(item, item.name);
        CU_ASSERT_EQUAL(idx, static_cast<size_t>(i));
    }
    
    CU_ASSERT_EQUAL(container.size(), 10);
    
    // Verify all items
    for (int i = 0; i < 10; i++) {
        const TestConfig* item = container.try_get(i);
        CU_ASSERT_PTR_NOT_NULL(item);
        
        char expected_name[32];
        snprintf(expected_name, sizeof(expected_name), "Item%d", i);
        CU_ASSERT_STRING_EQUAL(item->name, expected_name);
        CU_ASSERT_EQUAL(item->value, i * 10);
    }
}

ADD_TEST(test_config_container_unlimited_capacity) {
    ConfigContainer<TestConfig> container;
    
    // Add 1000 items (far exceeds old CREATURE_TYPES_MAX of 128)
    const int count = 1000;
    for (int i = 0; i < count; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Creature%d", i);
        item.value = i;
        item.scale = 1.0f;
        
        container.add(item, item.name);
    }
    
    CU_ASSERT_EQUAL(container.size(), count);
    
    // Spot check a few items
    CU_ASSERT_STRING_EQUAL(container.at(0).name, "Creature0");
    CU_ASSERT_STRING_EQUAL(container.at(500).name, "Creature500");
    CU_ASSERT_STRING_EQUAL(container.at(999).name, "Creature999");
}

ADD_TEST(test_config_container_bounds_checking) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item;
    strcpy(item.name, "Item");
    item.value = 1;
    container.add(item, "Item");
    
    // Valid access
    CU_ASSERT_NO_THROW(container.at(0));
    
    // Invalid access should be caught
    bool threw = false;
    try {
        container.at(999);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CU_ASSERT_TRUE(threw);
    
    // try_get should return nullptr for invalid index
    CU_ASSERT_EQUAL(container.try_get(999), nullptr);
}

ADD_TEST(test_config_container_find_by_name) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item1, item2, item3;
    strcpy(item1.name, "Imp");
    item1.value = 1;
    strcpy(item2.name, "Beetle");
    item2.value = 2;
    strcpy(item3.name, "Spider");
    item3.value = 3;
    
    container.add(item1, "Imp");
    container.add(item2, "Beetle");
    container.add(item3, "Spider");
    
    // Find existing items
    TestConfig* found = container.find_by_name("Beetle");
    CU_ASSERT_PTR_NOT_NULL(found);
    CU_ASSERT_STRING_EQUAL(found->name, "Beetle");
    CU_ASSERT_EQUAL(found->value, 2);
    
    // Find non-existing item
    CU_ASSERT_EQUAL(container.find_by_name("Dragon"), nullptr);
}

ADD_TEST(test_config_container_find_by_name_case_insensitive) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item;
    strcpy(item.name, "TestCreature");
    item.value = 42;
    container.add(item, "TestCreature");
    
    // Case-insensitive search
    TestConfig* found1 = container.find_by_name_ci("testcreature");
    CU_ASSERT_PTR_NOT_NULL(found1);
    CU_ASSERT_EQUAL(found1->value, 42);
    
    TestConfig* found2 = container.find_by_name_ci("TESTCREATURE");
    CU_ASSERT_PTR_NOT_NULL(found2);
    CU_ASSERT_EQUAL(found2->value, 42);
}

ADD_TEST(test_config_container_duplicate_name) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item1, item2;
    strcpy(item1.name, "Duplicate");
    item1.value = 1;
    strcpy(item2.name, "Duplicate");
    item2.value = 2;
    
    container.add(item1, "Duplicate");
    
    // Adding duplicate should throw
    bool threw = false;
    try {
        container.add(item2, "Duplicate");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CU_ASSERT_TRUE(threw);
    
    // Should still have only one item
    CU_ASSERT_EQUAL(container.size(), 1);
}

ADD_TEST(test_config_container_add_or_update) {
    ConfigContainer<TestConfig> container;
    
    TestConfig item1;
    strcpy(item1.name, "Updatable");
    item1.value = 1;
    
    // First add
    size_t idx1 = container.add_or_update(item1, "Updatable");
    CU_ASSERT_EQUAL(container.size(), 1);
    CU_ASSERT_EQUAL(container.at(idx1).value, 1);
    
    // Update existing
    TestConfig item2;
    strcpy(item2.name, "Updatable");
    item2.value = 2;
    
    size_t idx2 = container.add_or_update(item2, "Updatable");
    CU_ASSERT_EQUAL(idx1, idx2); // Same index
    CU_ASSERT_EQUAL(container.size(), 1); // Still one item
    CU_ASSERT_EQUAL(container.at(idx2).value, 2); // Value updated
}

ADD_TEST(test_config_container_span_access) {
    ConfigContainer<TestConfig> container;
    
    // Add several items
    for (int i = 0; i < 5; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Item%d", i);
        item.value = i;
        container.add(item, item.name);
    }
    
    // Get span (non-owning view)
    std::span<const TestConfig> items = container.items();
    CU_ASSERT_EQUAL(items.size(), 5);
    
    // Iterate via span
    int sum = 0;
    for (const auto& item : items) {
        sum += item.value;
    }
    CU_ASSERT_EQUAL(sum, 0 + 1 + 2 + 3 + 4); // 10
}

ADD_TEST(test_config_container_clear) {
    ConfigContainer<TestConfig> container;
    
    // Add items
    for (int i = 0; i < 5; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Item%d", i);
        container.add(item, item.name);
    }
    
    CU_ASSERT_EQUAL(container.size(), 5);
    
    // Clear
    container.clear();
    CU_ASSERT_EQUAL(container.size(), 0);
    CU_ASSERT_TRUE(container.empty());
    CU_ASSERT_EQUAL(container.find_by_name("Item0"), nullptr);
}

ADD_TEST(test_config_container_reserve) {
    ConfigContainer<TestConfig> container;
    
    // Reserve space for 100 items
    container.reserve(100);
    
    // Add items - no reallocations should occur
    for (int i = 0; i < 100; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Item%d", i);
        container.add(item, item.name);
    }
    
    CU_ASSERT_EQUAL(container.size(), 100);
}

//=============================================================================
// ConfigLoader Tests
//=============================================================================

// Mock loader for testing
class MockConfigLoader : public ConfigLoader<TestConfig> {
public:
    explicit MockConfigLoader(ConfigContainer<TestConfig>& container)
        : ConfigLoader(container, "test_config") {}
    
    bool load(LoadFlags flags = LoadFlags::Standard) override {
        // Simulate loading
        for (int i = 0; i < 3; i++) {
            TestConfig item;
            snprintf(item.name, sizeof(item.name), "MockItem%d", i);
            item.value = i * 100;
            item.scale = i * 0.5f;
            container_.add(item, item.name);
        }
        return true;
    }
};

ADD_TEST(test_config_loader_basic) {
    ConfigContainer<TestConfig> container;
    MockConfigLoader loader(container);
    
    CU_ASSERT_STRING_EQUAL(loader.get_name().c_str(), "test_config");
    
    bool success = loader.load();
    CU_ASSERT_TRUE(success);
    CU_ASSERT_EQUAL(container.size(), 3);
    
    CU_ASSERT_STRING_EQUAL(container.at(0).name, "MockItem0");
    CU_ASSERT_STRING_EQUAL(container.at(1).name, "MockItem1");
    CU_ASSERT_STRING_EQUAL(container.at(2).name, "MockItem2");
}

ADD_TEST(test_config_loader_reset) {
    ConfigContainer<TestConfig> container;
    MockConfigLoader loader(container);
    
    loader.load();
    CU_ASSERT_EQUAL(container.size(), 3);
    
    loader.reset();
    CU_ASSERT_EQUAL(container.size(), 0);
    CU_ASSERT_TRUE(container.empty());
}

ADD_TEST(test_config_loader_flags) {
    ConfigContainer<TestConfig> container;
    MockConfigLoader loader(container);
    
    // Test different load flags
    LoadFlags flags = LoadFlags::AcceptPartial | LoadFlags::IgnoreErrors;
    CU_ASSERT_TRUE(has_flag(flags, LoadFlags::AcceptPartial));
    CU_ASSERT_TRUE(has_flag(flags, LoadFlags::IgnoreErrors));
    CU_ASSERT_FALSE(has_flag(flags, LoadFlags::ListOnly));
}

//=============================================================================
// Performance Tests
//=============================================================================

ADD_TEST(test_config_container_performance_large_dataset) {
    ConfigContainer<TestConfig> container;
    
    // Add 10,000 items (stress test)
    const int count = 10000;
    container.reserve(count); // Pre-allocate for fairness
    
    for (int i = 0; i < count; i++) {
        TestConfig item;
        snprintf(item.name, sizeof(item.name), "Creature%d", i);
        item.value = i;
        item.scale = 1.0f;
        container.add(item, item.name);
    }
    
    CU_ASSERT_EQUAL(container.size(), count);
    
    // Test random access performance
    CU_ASSERT_EQUAL(container.at(5000).value, 5000);
    CU_ASSERT_STRING_EQUAL(container.at(7500).name, "Creature7500");
    
    // Test lookup performance
    TestConfig* found = container.find_by_name("Creature9999");
    CU_ASSERT_PTR_NOT_NULL(found);
    CU_ASSERT_EQUAL(found->value, 9999);
}
