#include "report.h"

#include <stdbool.h>

#include <pico/multicore.h>
#include <pico/async_context.h>
#include <pico/cyw43_arch.h>
#include <memory.h>

#include "usb.h"
#include "Switch.h"
#include "WiiU.h"

// used between threads
static SwitchIdxOutReport shared_switch_report;
static WiiUIdxOutReport shared_wiiu_report;
static uint32_t unused;

void set_global_gamepad_report(SwitchIdxOutReport *src) {
    if (!src) return;

    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(&shared_switch_report, src, sizeof(shared_switch_report));
    async_context_release_lock(context);
    multicore_fifo_push_timeout_us(0, 1);
}

void get_global_gamepad_report(SwitchIdxOutReport *dest) {
    multicore_fifo_pop_timeout_us(1, &unused);
    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(dest, &shared_switch_report, sizeof(*dest));
    async_context_release_lock(context);
}

void set_global_wiiu_report(WiiUIdxOutReport *src) {
    if (!src) return;

    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(&shared_wiiu_report, src, sizeof(shared_wiiu_report));
    async_context_release_lock(context);
    multicore_fifo_push_timeout_us(0, 1);
}

void get_global_wiiu_report(WiiUIdxOutReport *dest) {
    multicore_fifo_pop_timeout_us(1, &unused);
    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(dest, &shared_wiiu_report, sizeof(*dest));
    async_context_release_lock(context);
}
