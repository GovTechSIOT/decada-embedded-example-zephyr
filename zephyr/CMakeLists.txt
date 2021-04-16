cmake_minimum_required(VERSION 3.13.1)

# Out-of-tree board definition
set (BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zephyr-test)

FILE(GLOB app_sources ../src/*.c*)
target_sources(app PRIVATE ${app_sources})
