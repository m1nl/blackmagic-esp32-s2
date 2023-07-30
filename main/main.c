#include <stdint.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>

#include "usb.h"
#include "nvs.h"
#include "gdb_main.h"
#include "led.h"
#include "cli-uart.h"
#include "i2c.h"
#include "network.h"
#include "network-http.h"
#include "network-gdb.h"
#include "network-uart.h"
#include "factory-reset-service.h"

#include <gdb-glue.h>
#include <soft-uart-log.h>

static const char* TAG = "main";

void gdb_application_thread(void* pvParameters) {
    ESP_LOGI("gdb", "start");
    while(1) {
        gdb_main();
    }
    ESP_LOGI("gdb", "end");
}

#include <platform.h>

void app_main(void) {
    // Software UART logging at pin 7, 57600 baud
    //    soft_uart_log_init(7, 57600);

    ESP_LOGI(TAG, "start");

    factory_reset_service_init();
    platform_init();

    gdb_glue_init();

    led_init();
    led_set_blue(255);

    nvs_init();
    network_init();
    network_http_server_init();
    network_gdb_server_init();
    network_uart_server_init();

    usb_init();
    cli_uart_init();

    // TODO uart and i2c share the same pins, need switching mechanics
    // i2c_init();
    // i2c_scan();

    xTaskCreate(&gdb_application_thread, "gdb_thread", 4096, NULL, 5, NULL);
    led_set_blue(0);
    ESP_LOGI(TAG, "end");
}
