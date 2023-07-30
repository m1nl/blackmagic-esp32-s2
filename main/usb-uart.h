/**
 * @file usb-uart.h
 * @author Sergey Gavrilov (who.just.the.doctor@gmail.com)
 * @version 1.0
 * @date 2021-12-17
 *
 *
 */
#pragma once

#include "driver/uart.h"

#define USB_UART_PORT_NUM UART_NUM_0
#define USB_UART_TXD_PIN (43)
#define USB_UART_RXD_PIN (44)
#define USB_UART_TX_BUF_SIZE (UART_HW_FIFO_LEN(CLI_UART_PORT_NUM) * 2)
#define USB_UART_RX_BUF_SIZE (UART_HW_FIFO_LEN(CLI_UART_PORT_NUM) * 4)

void usb_uart_init();

void usb_uart_write(const uint8_t* data, size_t data_size);

void usb_uart_set_line_state(bool dtr, bool rts);

typedef struct {
    uint32_t bit_rate;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
} UsbUartConfig;

void usb_uart_set_line_coding(UsbUartConfig config);

UsbUartConfig usb_uart_get_line_coding();
