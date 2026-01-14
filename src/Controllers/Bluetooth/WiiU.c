#include <stdio.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/async_context.h>
#include <uni.h>

#include "sdkconfig.h"
#include "uni_hid_device.h"
#include "uni_log.h"
#include "usb.h"
#include "report.h"
#include "WiiU.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

#define AXIS_DEADZONE 0xa

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t *d);
WiiUOutReport report[CONFIG_BLUEPAD32_MAX_DEVICES];
WiiUIdxOutReport idx_r;
uint8_t connected_controllers;

// Disconnect Controller Combo declarations
#define DISCONNECT_COMBO (MISC_BUTTON_HOME | MISC_BUTTON_BACK)
static bool disconnect_combo_active[CONFIG_BLUEPAD32_MAX_DEVICES] = { false };

// Controller Connected Check WiiU
typedef enum {
    CTL_UNKNOWN = 0,
    CTL_WIIU_PRO,
    CTL_OTHER,
} controller_type_t;

// Helper functions
static void empty_gamepad_report(WiiUOutReport *gamepad)
{
    gamepad->buttons = 0;
    gamepad->hat = WIIU_HAT_NOTHING;
    gamepad->lx = WIIU_JOYSTICK_MID;
    gamepad->ly = WIIU_JOYSTICK_MID;
    gamepad->rx = WIIU_JOYSTICK_MID;
    gamepad->ry = WIIU_JOYSTICK_MID;
}

uint8_t convert_to_wiiu_axis(int32_t bluepadAxis)
{
    // bluepad32 reports from -512 to 511 as int32_t
    // WiiU reports from 0 to 255 as uint8_t

    bluepadAxis += 513;  // now max possible is 1024
    bluepadAxis /= 4;    // now max possible is 255

    if (bluepadAxis < WIIU_JOYSTICK_MIN)
        bluepadAxis = 0;
    else if ((bluepadAxis > (WIIU_JOYSTICK_MID - AXIS_DEADZONE)) &&
             (bluepadAxis < (WIIU_JOYSTICK_MID + AXIS_DEADZONE))) {
        bluepadAxis = WIIU_JOYSTICK_MID;
    } else if (bluepadAxis > WIIU_JOYSTICK_MAX)
        bluepadAxis = WIIU_JOYSTICK_MAX;

    return (uint8_t)bluepadAxis;
}

static void fill_gamepad_report(int idx, uni_gamepad_t *gp)
{
    empty_gamepad_report(&report[idx]);

    // face buttons
    if ((gp->buttons & BUTTON_A)) report[idx].buttons |= WIIU_MASK_A;
    if ((gp->buttons & BUTTON_B)) report[idx].buttons |= WIIU_MASK_B;
    if ((gp->buttons & BUTTON_X)) report[idx].buttons |= WIIU_MASK_X;
    if ((gp->buttons & BUTTON_Y)) report[idx].buttons |= WIIU_MASK_Y;

    // shoulder buttons
    if ((gp->buttons & BUTTON_SHOULDER_L)) report[idx].buttons |= WIIU_MASK_L;
    if ((gp->buttons & BUTTON_SHOULDER_R)) report[idx].buttons |= WIIU_MASK_R;

    // dpad
    switch (gp->dpad) {
    case DPAD_UP: report[idx].hat = WIIU_HAT_UP; break;
    case DPAD_DOWN: report[idx].hat = WIIU_HAT_DOWN; break;
    case DPAD_LEFT: report[idx].hat = WIIU_HAT_LEFT; break;
    case DPAD_RIGHT: report[idx].hat = WIIU_HAT_RIGHT; break;
    case DPAD_UP | DPAD_RIGHT: report[idx].hat = WIIU_HAT_UPRIGHT; break;
    case DPAD_DOWN | DPAD_RIGHT: report[idx].hat = WIIU_HAT_DOWNRIGHT; break;
    case DPAD_DOWN | DPAD_LEFT: report[idx].hat = WIIU_HAT_DOWNLEFT; break;
    case DPAD_UP | DPAD_LEFT: report[idx].hat = WIIU_HAT_UPLEFT; break;
    default: report[idx].hat = WIIU_HAT_NOTHING; break;
    }

    // sticks
    report[idx].lx = convert_to_wiiu_axis(gp->axis_x);
    report[idx].ly = convert_to_wiiu_axis(gp->axis_y);
    report[idx].rx = convert_to_wiiu_axis(gp->axis_rx);
    report[idx].ry = convert_to_wiiu_axis(gp->axis_ry);

    report[idx].vendor = 0x01;

    if ((gp->buttons & BUTTON_THUMB_L)) report[idx].buttons |= WIIU_MASK_L3;
    if ((gp->buttons & BUTTON_THUMB_R)) report[idx].buttons |= WIIU_MASK_R3;

    // triggers
    if (gp->brake) report[idx].buttons |= WIIU_MASK_ZL;
    if (gp->throttle) report[idx].buttons |= WIIU_MASK_ZR;

    // misc buttons
    if (gp->misc_buttons & MISC_BUTTON_SYSTEM) report[idx].buttons |= WIIU_MASK_HOME;
    if (gp->misc_buttons & MISC_BUTTON_CAPTURE) report[idx].buttons |= WIIU_MASK_CAPTURE;
    if (gp->misc_buttons & MISC_BUTTON_BACK) report[idx].buttons |= WIIU_MASK_MINUS;
    if (gp->misc_buttons & MISC_BUTTON_HOME) report[idx].buttons |= WIIU_MASK_PLUS;
}

static void set_led_status() {
    if (connected_controllers == 0)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    else
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

// Platform Overrides
static void pico_wiiu_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("wiiu_platform: init()\n");

    connected_controllers = 0;

    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // remaps if needed
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_y = UNI_GAMEPAD_MAPPINGS_BUTTON_X;
    mappings.button_x = UNI_GAMEPAD_MAPPINGS_BUTTON_Y;

    uni_gamepad_set_mappings(&mappings);

    idx_r.idx = 0;
    idx_r.report.buttons = 0;
    idx_r.report.hat = WIIU_HAT_NOTHING;
    idx_r.report.lx = 0;
    idx_r.report.ly = 0;
    idx_r.report.rx = 0;
    idx_r.report.ry = 0;
    set_global_wiiu_report(&idx_r);
}

static void pico_wiiu_platform_on_init_complete(void) {
    logi("wiiu_platform: on_init_complete()\n");

    uni_bt_enable_new_connections_safe(true);

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    multicrore_fifo_push_blocking(0); // start USB reading
}

static void pico_wiiu_platform_on_device_connected(uni_hid_device_t* d) {
    logi("wiiu_platform: device connected: %p\n", d);
}

static void pico_wiiu_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("wiiu_platform: device disconnected: %p\n", d);

    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; i++) {
        empty_gamepad_report(&report[i]);
        idx_r.idx = i;
        idx_r.report = report[i];
        set_global_wiiu_report(&idx_r);
    }

    connected_controllers--;
    set_led_status();
}

static uni_error_t pico_wiiu_platform_on_device_ready(uni_hid_device_t* d) {
    logi("wiiu_platform: device ready: %p\n", d);

    controller_type_t type = CTL_OTHER;

    if (d->vendor_id == WII_U_PRO_VID &&
        d->product_id == WII_U_PRO_PID) {
        type = CTL_WIIU_PRO;
    }

    d->platform_data[0] = (uint8_t)type;

    connected_controllers++;
    set_led_status();
    return UNI_ERROR_SUCCESS;
}

static void pico_wiiu_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl)
{
    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) return;

    uni_gamepad_t *gp;
    uint8_t idx = uni_hid_device_get_idx_for_instance(d);
    gp = &ctl->gamepad;

    bool combo_pressed = (gp->misc_buttons & DISCONNECT_COMBO) == DISCONNECT_COMBO;

    if (combo_pressed && !disconnect_combo_active[idx]) {
        disconnect_combo_active[idx] = true;
        logi("Disconnect combo pressed on controller %d\n", idx);
        uni_hid_device_disconnect(d);
        uni_bt_enable_new_connections_safe(true);
        return;
    }

    if (!combo_pressed) disconnect_combo_active[idx] = false;

    fill_gamepad_report(idx, gp);

    idx_r.idx = idx;
    idx_r.report = report[idx];
    set_global_wiiu_report(&idx_r);
}

static const uni_property_t* pico_wiiu_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void pico_wiiu_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    ARG_UNUSED(event);
    ARG_UNUSED(data);
    return;
}

// Helpers
static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    (void)d; // optional lightbar/LED hooks
}

// Entry Point
struct uni_platform* get_wiiu_platform(void) {
    static struct uni_platform plat = {
        .name = "WiiU Platform",
        .init = pico_wiiu_platform_init,
        .on_init_complete = pico_wiiu_platform_on_init_complete,
        .on_device_connected = pico_wiiu_platform_on_device_connected,
        .on_device_disconnected = pico_wiiu_platform_on_device_disconnected,
        .on_device_ready = pico_wiiu_platform_on_device_ready,
        .on_oob_event = pico_wiiu_platform_on_oob_event,
        .on_controller_data = pico_wiiu_platform_on_controller_data,
        .get_property = pico_wiiu_platform_get_property,
    };

    return &plat;
}
