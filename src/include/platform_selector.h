#pragma once

typedef enum {
    PLATFORM_SWITCH = 0,
    PLATFORM_WIIU
} platform_t;

// Current platform
extern platform_t current_platform;
