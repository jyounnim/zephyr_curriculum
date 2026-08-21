# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(00_ZEPHYR_CURRICULUM_LAB)

target_sources(app PRIVATE src/main.c)
