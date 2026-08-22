zephyr_library()
zephyr_library_sources_ifdef(CONFIG_AHT20 aht20.c)

# Zephyr 4.4.x: out-of-tree libraries must explicitly depend on
# zephyr_generated_headers, or the build can race against headers
# like zephyr/heap_constants.h that kernel.h now includes.
# See: Zephyr v4.4.0 migration guide / GitHub #106438, #106439
add_dependencies(${ZEPHYR_CURRENT_LIBRARY} zephyr_generated_headers)
