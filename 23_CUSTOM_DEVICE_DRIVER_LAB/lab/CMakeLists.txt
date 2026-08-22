cmake_minimum_required(VERSION 3.20.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})   # also search this project's dts/bindings

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aht20_driver_example)

target_sources(app PRIVATE src/main.c)

# NOTE: the driver source is added directly to the "app" target instead of
# its own zephyr_library(), because zephyr_library() called via
# add_subdirectory() from an application's CMakeLists.txt runs in
# "application mode" - at that point Zephyr's library list is already
# finalized, so the library becomes a silent no-op and never gets linked
# (it still compiles, but undefined reference errors show up at link time).
target_sources_ifdef(CONFIG_AHT20 app PRIVATE drivers/sensor/aht20/aht20.c)
