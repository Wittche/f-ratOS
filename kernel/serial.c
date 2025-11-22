#include "serial.h"

// I/O port functions (inline assembly)
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(uint16_t port, uint16_t baud_divisor) {
    // Disable all interrupts
    outb(port + SERIAL_INT_ENABLE_REG, 0x00);

    // Enable DLAB (Divisor Latch Access Bit) to set baud rate
    outb(port + SERIAL_LINE_CTRL_REG, 0x80);

    // Set baud rate divisor (low byte)
    outb(port + SERIAL_DATA_REG, (uint8_t)(baud_divisor & 0xFF));

    // Set baud rate divisor (high byte)
    outb(port + SERIAL_INT_ENABLE_REG, (uint8_t)((baud_divisor >> 8) & 0xFF));

    // Configure: 8 bits, no parity, 1 stop bit (8N1)
    // Bit layout: 0x03 = 0b00000011
    // - Bits 0-1: 11 = 8 data bits
    // - Bit 2: 0 = 1 stop bit
    // - Bits 3-5: 000 = no parity
    // - Bit 6: 0 = no break control
    // - Bit 7: 0 = disable DLAB
    outb(port + SERIAL_LINE_CTRL_REG, 0x03);

    // Enable FIFO, clear TX/RX queues, 14-byte threshold
    // Bit layout: 0xC7 = 0b11000111
    // - Bit 0: 1 = Enable FIFO
    // - Bit 1: 1 = Clear receive FIFO
    // - Bit 2: 1 = Clear transmit FIFO
    // - Bit 3: 0 = DMA mode (not used)
    // - Bits 6-7: 11 = 14-byte threshold
    outb(port + SERIAL_FIFO_CTRL_REG, 0xC7);

    // Enable IRQs, set RTS/DSR
    // Bit layout: 0x0B = 0b00001011
    // - Bit 0: 1 = DTR (Data Terminal Ready)
    // - Bit 1: 1 = RTS (Request To Send)
    // - Bit 2: 0 = OUT1 (not used)
    // - Bit 3: 1 = OUT2 (enable interrupts)
    // - Bit 4: 0 = Loop mode disabled
    outb(port + SERIAL_MODEM_CTRL_REG, 0x0B);

    // Test the serial chip (loopback test)
    // Set in loopback mode, test the serial chip
    outb(port + SERIAL_MODEM_CTRL_REG, 0x1E);

    // Send test byte
    outb(port + SERIAL_DATA_REG, 0xAE);

    // Check if serial is working (test byte should be returned)
    if (inb(port + SERIAL_DATA_REG) != 0xAE) {
        // Serial is faulty, but we'll continue anyway
        // In a real OS, you might want to disable serial here
    }

    // Set serial to normal operation mode
    // (not-loopback, IRQs enabled, OUT1 and OUT2 bits enabled)
    outb(port + SERIAL_MODEM_CTRL_REG, 0x0F);
}

bool serial_is_transmit_ready(uint16_t port) {
    // Check if the transmit holding register is empty (bit 5)
    return (inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LSR_THR_EMPTY) != 0;
}

void serial_write_byte(uint16_t port, uint8_t data) {
    // Wait until the transmit buffer is empty
    while (!serial_is_transmit_ready(port)) {
        // Busy wait
    }

    // Write the byte to the data register
    outb(port + SERIAL_DATA_REG, data);
}

void serial_write_string(uint16_t port, const char* str) {
    if (!str) {
        return;
    }

    while (*str) {
        // Convert LF to CRLF for proper line breaks on serial terminals
        if (*str == '\n') {
            serial_write_byte(port, '\r');
        }
        serial_write_byte(port, *str);
        str++;
    }
}

bool serial_is_data_available(uint16_t port) {
    // Check if data is ready to be read (bit 0)
    return (inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LSR_DATA_READY) != 0;
}

uint8_t serial_read_byte(uint16_t port) {
    // Wait until data is available
    while (!serial_is_data_available(port)) {
        // Busy wait
    }

    // Read and return the byte from the data register
    return inb(port + SERIAL_DATA_REG);
}
