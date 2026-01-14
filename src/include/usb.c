#include "usb.h"

#include <tusb.h>
#include <stdint.h>
#include <stdbool.h>

#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/async_context.h>

#include "report.h"
#include "Switch.h"
#include "WiiU.h"
#include "platform_selector.h"

void usb_core_task()
{
	tud_init(0);

    SwitchIdxOutReport r_switch = {0};
    WiiUIdxOutReport r_wiiu = {0};

    while (1) {
        tud_task();

        if (tud_suspended()) {
            tud_remote_wakeup();
            continue;
        }

        if (current_platform == PLATFORM_WIIU) {
            get_global_wiiu_report(&r_wiiu);
            if (tud_hid_n_ready(0)) {
                tud_hid_n_report(0, 0, &r_wiiu.report, sizeof(r_wiiu.report));
            }
        } else {
            get_global_gamepad_report(&r_switch);
            if (tud_hid_n_ready(r_switch.idx)) {
                tud_hid_n_report(r_switch.idx, 0, &r_switch.report, sizeof(r_switch.report));
            }
        }
    }
}