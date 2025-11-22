#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

// Serial port base addresses
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8

// Serial port register offsets
#define SERIAL_DATA_REG         0  // Data register (R/W)
#define SERIAL_INT_ENABLE_REG   1  // Interrupt Enable Register
#define SERIAL_FIFO_CTRL_REG    2  // FIFO Control Register (W)
#define SERIAL_LINE_CTRL_REG    3  // Line Control Register
#define SERIAL_MODEM_CTRL_REG   4  // Modem Control Register
#define SERIAL_LINE_STATUS_REG  5  // Line Status Register
#define SERIAL_MODEM_STATUS_REG 6  // Modem Status Register

// Line Status Register bits
#define SERIAL_LSR_DATA_READY    0x01
#define SERIAL_LSR_OVERRUN_ERROR 0x02
#define SERIAL_LSR_PARITY_ERROR  0x04
#define SERIAL_LSR_FRAMING_ERROR 0x08
#define SERIAL_LSR_BREAK_INT     0x10
#define SERIAL_LSR_THR_EMPTY     0x20  // Transmitter Holding Register Empty
#define SERIAL_LSR_TRANSMIT_EMPTY 0x40 // Transmitter Empty

/**
 * Initialize the serial port
 * @param port The base I/O port (e.g., SERIAL_COM1)
 * @param baud_divisor Baud rate divisor (115200 / desired_baud_rate)
 *                     1 = 115200, 2 = 57600, 3 = 38400, etc.
 */
void serial_init(uint16_t port, uint16_t baud_divisor);

/**
 * Write a byte to the serial port
 * @param port The base I/O port
 * @param data The byte to write
 */
void serial_write_byte(uint16_t port, uint8_t data);

/**
 * Write a null-terminated string to the serial port
 * @param port The base I/O port
 * @param str The string to write
 */
void serial_write_string(uint16_t port, const char* str);

/**
 * Check if the transmit buffer is empty (ready to send)
 * @param port The base I/O port
 * @return true if ready to transmit, false otherwise
 */
bool serial_is_transmit_ready(uint16_t port);

/**
 * Read a byte from the serial port (blocking)
 * @param port The base I/O port
 * @return The byte read
 */
uint8_t serial_read_byte(uint16_t port);

/**
 * Check if data is available to read
 * @param port The base I/O port
 * @return true if data is available, false otherwise
 */
bool serial_is_data_available(uint16_t port);

#endif // SERIAL_H
