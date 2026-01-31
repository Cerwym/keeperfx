/**
 * Example standalone test file demonstrating how to write unit tests for KeeperFX
 * 
 * This file shows:
 * - How to structure tests using the ADD_TEST macro
 * - Various CUnit assertion types
 * - Testing standalone utility functions
 * - Best practices for test organization
 */

#include "tst_main.h"
#include <cstring>
#include <cmath>

// Example standalone utility functions to test
// In a real scenario, these would be in your source files

namespace math_utils {
    int add(int a, int b) {
        return a + b;
    }
    
    int multiply(int a, int b) {
        return a * b;
    }
    
    double square_root(double x) {
        return sqrt(x);
    }
}

namespace string_utils {
    bool starts_with(const char* str, const char* prefix) {
        if (!str || !prefix) return false;
        return strncmp(str, prefix, strlen(prefix)) == 0;
    }
    
    int count_chars(const char* str, char c) {
        if (!str) return 0;
        int count = 0;
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] == c) count++;
        }
        return count;
    }
}

namespace array_utils {
    int find_max(const int* arr, int size) {
        if (!arr || size <= 0) return 0;
        int max = arr[0];
        for (int i = 1; i < size; i++) {
            if (arr[i] > max) max = arr[i];
        }
        return max;
    }
    
    bool is_sorted(const int* arr, int size) {
        if (!arr || size <= 1) return true;
        for (int i = 1; i < size; i++) {
            if (arr[i] < arr[i-1]) return false;
        }
        return true;
    }
}

// ============================================================================
// TESTS: Basic Arithmetic
// ============================================================================

ADD_TEST(test_addition_positive_numbers)
{
    CU_ASSERT_EQUAL(math_utils::add(2, 3), 5);
    CU_ASSERT_EQUAL(math_utils::add(10, 20), 30);
    CU_ASSERT_EQUAL(math_utils::add(0, 0), 0);
}

ADD_TEST(test_addition_negative_numbers)
{
    CU_ASSERT_EQUAL(math_utils::add(-5, -3), -8);
    CU_ASSERT_EQUAL(math_utils::add(-10, 5), -5);
    CU_ASSERT_EQUAL(math_utils::add(10, -10), 0);
}

ADD_TEST(test_multiplication)
{
    CU_ASSERT_EQUAL(math_utils::multiply(3, 4), 12);
    CU_ASSERT_EQUAL(math_utils::multiply(0, 100), 0);
    CU_ASSERT_EQUAL(math_utils::multiply(-2, 5), -10);
    CU_ASSERT_EQUAL(math_utils::multiply(-3, -4), 12);
}

ADD_TEST(test_square_root)
{
    // Use DOUBLE_EQUAL with epsilon for floating point comparisons
    CU_ASSERT_DOUBLE_EQUAL(math_utils::square_root(4.0), 2.0, 0.0001);
    CU_ASSERT_DOUBLE_EQUAL(math_utils::square_root(9.0), 3.0, 0.0001);
    CU_ASSERT_DOUBLE_EQUAL(math_utils::square_root(0.0), 0.0, 0.0001);
}

// ============================================================================
// TESTS: String Operations
// ============================================================================

ADD_TEST(test_starts_with_true_cases)
{
    CU_ASSERT_TRUE(string_utils::starts_with("hello world", "hello"));
    CU_ASSERT_TRUE(string_utils::starts_with("test", "test"));
    CU_ASSERT_TRUE(string_utils::starts_with("abc", ""));
}

ADD_TEST(test_starts_with_false_cases)
{
    CU_ASSERT_FALSE(string_utils::starts_with("hello", "world"));
    CU_ASSERT_FALSE(string_utils::starts_with("test", "testing"));
    CU_ASSERT_FALSE(string_utils::starts_with("", "test"));
}

ADD_TEST(test_starts_with_null_safety)
{
    CU_ASSERT_FALSE(string_utils::starts_with(nullptr, "test"));
    CU_ASSERT_FALSE(string_utils::starts_with("test", nullptr));
    CU_ASSERT_FALSE(string_utils::starts_with(nullptr, nullptr));
}

ADD_TEST(test_count_chars)
{
    CU_ASSERT_EQUAL(string_utils::count_chars("hello", 'l'), 2);
    CU_ASSERT_EQUAL(string_utils::count_chars("hello", 'h'), 1);
    CU_ASSERT_EQUAL(string_utils::count_chars("hello", 'x'), 0);
    CU_ASSERT_EQUAL(string_utils::count_chars("aaaaaa", 'a'), 6);
}

ADD_TEST(test_count_chars_edge_cases)
{
    CU_ASSERT_EQUAL(string_utils::count_chars("", 'a'), 0);
    CU_ASSERT_EQUAL(string_utils::count_chars(nullptr, 'a'), 0);
}

// ============================================================================
// TESTS: Array Operations
// ============================================================================

ADD_TEST(test_find_max_normal_cases)
{
    int arr1[] = {1, 5, 3, 9, 2};
    CU_ASSERT_EQUAL(array_utils::find_max(arr1, 5), 9);
    
    int arr2[] = {-5, -1, -10, -3};
    CU_ASSERT_EQUAL(array_utils::find_max(arr2, 4), -1);
    
    int arr3[] = {42};
    CU_ASSERT_EQUAL(array_utils::find_max(arr3, 1), 42);
}

ADD_TEST(test_find_max_edge_cases)
{
    int arr[] = {1, 2, 3};
    CU_ASSERT_EQUAL(array_utils::find_max(nullptr, 5), 0);
    CU_ASSERT_EQUAL(array_utils::find_max(arr, 0), 0);
    CU_ASSERT_EQUAL(array_utils::find_max(arr, -1), 0);
}

ADD_TEST(test_is_sorted_true_cases)
{
    int arr1[] = {1, 2, 3, 4, 5};
    CU_ASSERT_TRUE(array_utils::is_sorted(arr1, 5));
    
    int arr2[] = {1, 1, 1, 1};
    CU_ASSERT_TRUE(array_utils::is_sorted(arr2, 4));
    
    int arr3[] = {-10, -5, 0, 5, 10};
    CU_ASSERT_TRUE(array_utils::is_sorted(arr3, 5));
}

ADD_TEST(test_is_sorted_false_cases)
{
    int arr1[] = {5, 4, 3, 2, 1};
    CU_ASSERT_FALSE(array_utils::is_sorted(arr1, 5));
    
    int arr2[] = {1, 3, 2, 4};
    CU_ASSERT_FALSE(array_utils::is_sorted(arr2, 4));
}

ADD_TEST(test_is_sorted_edge_cases)
{
    int arr[] = {42};
    CU_ASSERT_TRUE(array_utils::is_sorted(nullptr, 5));  // nullptr is considered sorted
    CU_ASSERT_TRUE(array_utils::is_sorted(arr, 0));      // empty is sorted
    CU_ASSERT_TRUE(array_utils::is_sorted(arr, 1));      // single element is sorted
}

// ============================================================================
// TESTS: Demonstrating Multiple Assertions
// ============================================================================

ADD_TEST(test_comprehensive_validation)
{
    // You can have multiple assertions in one test
    // This is useful for testing related properties together
    
    int result = math_utils::add(5, 3);
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT_NOT_EQUAL(result, 0);
    CU_ASSERT(result > 0);
    CU_ASSERT(result < 100);
    
    // Test string operations together
    const char* test_str = "KeeperFX";
    CU_ASSERT_TRUE(string_utils::starts_with(test_str, "Keeper"));
    CU_ASSERT_FALSE(string_utils::starts_with(test_str, "Dungeon"));
    CU_ASSERT_EQUAL(string_utils::count_chars(test_str, 'e'), 3);
}

// ============================================================================
// TESTS: Boundary and Special Cases
// ============================================================================

ADD_TEST(test_boundary_values)
{
    // Always test boundary conditions
    CU_ASSERT_EQUAL(math_utils::multiply(0, 0), 0);
    CU_ASSERT_EQUAL(math_utils::multiply(1, 1), 1);
    CU_ASSERT_EQUAL(math_utils::multiply(-1, -1), 1);
    
    // Test limits
    int large = 1000000;
    CU_ASSERT_EQUAL(math_utils::add(large, large), 2000000);
}

ADD_TEST(test_null_pointer_handling)
{
    // Always test null/invalid input handling
    CU_ASSERT_FALSE(string_utils::starts_with(nullptr, "test"));
    CU_ASSERT_EQUAL(string_utils::count_chars(nullptr, 'x'), 0);
    CU_ASSERT_EQUAL(array_utils::find_max(nullptr, 5), 0);
}

/**
 * This example file demonstrates:
 * 
 * 1. Test Organization:
 *    - Group related tests together
 *    - Use descriptive test names
 *    - Add comments to explain test purpose
 * 
 * 2. Test Coverage:
 *    - Normal/happy path cases
 *    - Edge cases (empty, null, boundary values)
 *    - Error conditions
 * 
 * 3. Assertion Types:
 *    - CU_ASSERT_EQUAL for exact values
 *    - CU_ASSERT_TRUE/FALSE for boolean conditions
 *    - CU_ASSERT_DOUBLE_EQUAL for floating point
 *    - Multiple assertions in one test when appropriate
 * 
 * 4. Best Practices:
 *    - Test one behavior per test function
 *    - Make tests independent
 *    - Use clear, descriptive names
 *    - Test both success and failure paths
 */
