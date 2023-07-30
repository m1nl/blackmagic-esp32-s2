#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/uart.h"

#include "usb.h"
#include "usb-uart.h"

#define USB_UART_DEFAULT_BAUD_RATE (115200)
#define USB_UART_DEFAULT_STOP_BITS (UART_STOP_BITS_1)
#define USB_UART_DEFAULT_PARITY (UART_PARITY_DISABLE)
#define USB_UART_DEFAULT_DATA_BITS (UART_DATA_8_BITS)

static const char* TAG = "usb-uart";

static void usb_uart_rx_task(void* pvParameters);

void usb_uart_init() {
    ESP_LOGI(TAG, "init");

    uart_config_t uart_config = {
        .baud_rate = USB_UART_DEFAULT_BAUD_RATE,
        .stop_bits = USB_UART_DEFAULT_STOP_BITS,
        .parity = USB_UART_DEFAULT_PARITY,
        .data_bits = USB_UART_DEFAULT_DATA_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(
        USB_UART_PORT_NUM, USB_UART_RX_BUF_SIZE, USB_UART_TX_BUF_SIZE, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_set_pin(USB_UART_PORT_NUM, USB_UART_TXD_PIN, USB_UART_RXD_PIN, -1, -1));
    ESP_ERROR_CHECK(uart_param_config(USB_UART_PORT_NUM, &uart_config));

    xTaskCreate(usb_uart_rx_task, "usb_uart_rx", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "init done");
}

void usb_uart_write(const uint8_t* data, size_t data_size) {
    size_t pos = 0;

    while(pos < data_size) {
        size_t length = data_size - pos;

        if(length > USB_UART_TX_BUF_SIZE) {
            length = USB_UART_TX_BUF_SIZE;
        }

        uart_write_bytes(USB_UART_PORT_NUM, data + pos, length);
        uart_wait_tx_done(USB_UART_PORT_NUM, 20 / portTICK_PERIOD_MS);

        pos += length;
    }
}

void usb_uart_set_line_state(bool dtr, bool rts) {
    // do nothing, we don't have rts and dtr pins
}

void usb_uart_set_line_coding(UsbUartConfig config) {
    uart_stop_bits_t stop_bits;
    uart_parity_t parity;
    uart_word_length_t data_bits;

    ESP_ERROR_CHECK(uart_set_baudrate(USB_UART_PORT_NUM, config.bit_rate));

    // cdc.h
    // 0: 1 stop bit
    // 1: 1.5 stop bits
    // 2: 2 stop bits
    switch(config.stop_bits) {
    case 0:
        stop_bits = UART_STOP_BITS_1;
        break;
    case 1:
        stop_bits = UART_STOP_BITS_1_5;
        break;
    case 2:
        stop_bits = UART_STOP_BITS_2;
        break;
    default:
        stop_bits = USB_UART_DEFAULT_STOP_BITS;
        break;
    }

    ESP_ERROR_CHECK(uart_set_stop_bits(USB_UART_PORT_NUM, stop_bits));

    // cdc.h
    // 0: None
    // 1: Odd
    // 2: Even
    // 3: Mark
    // 4: Space
    switch(config.parity) {
    case 0:
        parity = UART_PARITY_DISABLE;
        break;
    case 1:
        parity = UART_PARITY_ODD;
        break;
    case 2:
        parity = UART_PARITY_EVEN;
        break;
    default:
        parity = USB_UART_DEFAULT_PARITY;
        break;
    }

    ESP_ERROR_CHECK(uart_set_parity(USB_UART_PORT_NUM, parity));

    // cdc.h
    // 5, 6, 7, 8 or 16
    switch(config.data_bits) {
    case 5:
        data_bits = UART_DATA_5_BITS;
        break;
    case 6:
        data_bits = UART_DATA_6_BITS;
        break;
    case 7:
        data_bits = UART_DATA_7_BITS;
        break;
    case 8:
        data_bits = UART_DATA_8_BITS;
        break;
    default:
        data_bits = USB_UART_DEFAULT_DATA_BITS;
        break;
    }

    ESP_ERROR_CHECK(uart_set_word_length(USB_UART_PORT_NUM, data_bits));
}

UsbUartConfig usb_uart_get_line_coding() {
    uart_stop_bits_t stop_bits;
    uart_parity_t parity;
    uart_word_length_t data_bits;

    UsbUartConfig config = {
        .bit_rate = 0,
        .parity = 0,
        .stop_bits = 0,
        .data_bits = 0,
    };

    ESP_ERROR_CHECK(uart_get_baudrate(USB_UART_PORT_NUM, &config.bit_rate));

    ESP_ERROR_CHECK(uart_get_stop_bits(USB_UART_PORT_NUM, &stop_bits));

    switch(stop_bits) {
    case UART_STOP_BITS_1:
        config.stop_bits = 0;
        break;
    case UART_STOP_BITS_1_5:
        config.stop_bits = 1;
        break;
    case UART_STOP_BITS_2:
        config.stop_bits = 2;
        break;
    default:
        break;
    }

    ESP_ERROR_CHECK(uart_get_parity(USB_UART_PORT_NUM, &parity));

    switch(parity) {
    case UART_PARITY_DISABLE:
        config.parity = 0;
        break;
    case UART_PARITY_ODD:
        config.parity = 1;
        break;
    case UART_PARITY_EVEN:
        config.parity = 2;
        break;
    default:
        break;
    }

    ESP_ERROR_CHECK(uart_get_word_length(USB_UART_PORT_NUM, &data_bits));

    switch(data_bits) {
    case UART_DATA_5_BITS:
        config.data_bits = 5;
        break;
    case UART_DATA_6_BITS:
        config.data_bits = 6;
        break;
    case UART_DATA_7_BITS:
        config.data_bits = 7;
        break;
    case UART_DATA_8_BITS:
        config.data_bits = 8;
        break;
    default:
        break;
    }

    return config;
}

#include "network-uart.h"
#include "network-http.h"

static void usb_uart_rx_task(void* pvParameters) {
    uint8_t* data = malloc(USB_UART_RX_BUF_SIZE);

    while(1) {
        int len = uart_read_bytes(
            USB_UART_PORT_NUM, data, USB_UART_RX_BUF_SIZE, 20 / portTICK_PERIOD_MS);

        for(size_t i = 0; i < len; i++) {
            if((i + 1) == len) {
                usb_uart_tx_char(data[i], true);
            } else {
                usb_uart_tx_char(data[i], false);
            }
        }

        network_http_uart_write_data(data, len);

        if(network_uart_connected()) {
            network_uart_send(data, len);
        }
    }
}
