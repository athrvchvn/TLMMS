#include "watchdog.h"
#include "config.h"
#include "esp_task_wdt.h"

void Watchdog::begin() {
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);  // true = panic (reset) on trigger
    esp_task_wdt_add(NULL);                          // subscribe main task
}

void Watchdog::feed() {
    esp_task_wdt_reset();
}
