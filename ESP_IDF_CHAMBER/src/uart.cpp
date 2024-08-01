#include <Arduino.h>
#include "uart.h"
#include "driver/uart.h"
#define BUF_SIZE        (127)
#define ECHO_READ_TOUT          (3) 
#define PACKET_READ_TICS        (100 / portTICK_PERIOD_MS)
static const char TAG[] = __FILE__;
TaskHandle_t UartTask = NULL;

void uart_setup(uart_port_t uart_num, int baud_rate, uart_word_length_t word_lenght) {
  // setup UART connection
  uart_config_t uart_config = {
    .baud_rate = baud_rate,
    .data_bits = word_lenght,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122
  };
  uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(uart_num, &uart_config);
  uart_set_pin(uart_num, CLOCK_DCF_TXD, CLOCK_DCF_RXD, CLOCK_DCF_RTS, CLOCK_DCF_CTS);
  uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX);
  uart_set_rx_timeout(uart_num, ECHO_READ_TOUT);
}
