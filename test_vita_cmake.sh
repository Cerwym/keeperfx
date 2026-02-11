#!/bin/bash
# Test script to verify VITA variable is set correctly

# Create a minimal test CMakeLists.txt
cat > /tmp/test_vita_detect.cmake << 'TESTCMAKE'
cmake_minimum_required(VERSION 3.20)
project(test_vita C)

message(STATUS "=== Platform Detection Test ===")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "VITA variable: ${VITA}")
message(STATUS "WIN32 variable: ${WIN32}")
message(STATUS "UNIX variable: ${UNIX}")

if(VITA)
    message(STATUS "✓ VITA platform detected - ARM architecture will be used")
    message(STATUS "  No x86-64 march flags should be applied")
else()
    message(STATUS "✗ VITA platform NOT detected")
endif()
TESTCMAKE

echo "Testing with vita.cmake toolchain..."
mkdir -p /tmp/test-vita-build
cd /tmp/test-vita-build
cmake -DCMAKE_TOOLCHAIN_FILE=/home/runner/work/keeperfx/keeperfx/vita.cmake /tmp -P /tmp/test_vita_detect.cmake 2>&1 | grep -E "(Platform Detection|CMAKE_SYSTEM|VITA|march)"
