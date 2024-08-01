#include <cstdint>
#include <StreamString.h>
#include <hal/uart_types.h>
#include <WString.h>
#ifndef UART_H
#define UART_H

// UART for Clock DCF
#define CLOCK_DCF_TXD  (GPIO_NUM_23)
#define CLOCK_DCF_RXD  (GPIO_NUM_22)
#define CLOCK_DCF_RTS  (GPIO_NUM_18)
#define CLOCK_DCF_CTS  (UART_PIN_NO_CHANGE)
#define CLOCK_BUF_SIZE (128)
void uart_setup(uart_port_t uart_num, int baud_rate, uart_word_length_t word_lenght);

#endif
