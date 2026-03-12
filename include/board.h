#ifndef BOARD_H_
#define BOARD_H_

#include "hardware/structs/sio.h"
#include <hardware/i2c.h>

// ADDR GPIO pins       :     GP0 - GP15
#define PINROMADDR    0
#define ADDRWIDTH    16

// DATA GPIO pins       :     GP16 - GP23
#define PINROMDATA   16
#define DATAWIDTH     8

// CONTROL pins
#define IO1          24
#define IO2          25
#define PHI2         26
#define ROML         27
#define ROMH         28
#define BA           29
#define RW           30
#define GAME         31    // inverted logic
#define EXROM        32    // inverted logic
#define RESET        33    // inverted logic

// SD pins
#define SD_SPI_PORT  spi0
#define SD_SCK       34
#define SD_MOSI      35
#define SD_MISO      36
#define SD_CS        37

#define LED          39

// I2C pins
#define I2C_PORT     i2c0
#define I2C_ADDR     0x50
#define I2C_SDA      40
#define I2C_SCL      41
#define I2C_BAUDRATE 400000

#define UART_ID      uart0
#define UART_RX      45
#define UART_TX      46
#define UART_BAUDRATE 115200

#define PSRAM_CS     47

#define IO1_MASK     ((uint64_t)1 << IO1)
#define IO2_MASK     ((uint64_t)1 << IO2)
#define PHI2_MASK    ((uint64_t)1 << PHI2)
#define ROML_MASK    ((uint64_t)1 << ROML)
#define ROMH_MASK    ((uint64_t)1 << ROMH)
#define BA_MASK      ((uint64_t)1 << BA)
#define RW_MASK      ((uint64_t)1 << RW)

// masks
#define ADDR_GPIO_MASK     (0xFFFF << PINROMADDR)
#define DATA_GPIO_MASK     (0xFF << PINROMDATA)

#define SET_DATA_MODE_OUT     sio_hw->gpio_oe_set = DATA_GPIO_MASK;
#define SET_DATA_MODE_IN      sio_hw->gpio_oe_clr = DATA_GPIO_MASK;

#define GPIO_GET_LOW_32(v)    pico_default_asm_volatile ("mrc p0, #0, %0, c0, c8" : "=r" (v));
#define GPIO_GET_HIGH_32(v)   pico_default_asm_volatile ("mrc p0, #0, %0, c0, c9" : "=r" (v));

#define ADDR_IN (sio_hw->gpio_in & ADDR_GPIO_MASK) >> PINROMADDR
#define DATA_OUT(v) sio_hw->gpio_out = (sio_hw->gpio_out & ~(0xFF << PINROMDATA)) | ((v)<<PINROMDATA);
#define DATA_IN ((sio_hw->gpio_in & DATA_GPIO_MASK) >> PINROMDATA)
#define DATA_IN_BYTE DATA_IN

#define PSRAM_BASE   0x11000000
#define PSRAM_SIZE   (2048 * 1024)  // 2 MB

#define FLASH_AREA_SIZE    1024 * 1024     // 1 MB
#define FLASH_AREA_OFFSET  PICO_FLASH_SIZE_BYTES - FLASH_AREA_SIZE

//#define ROM_SIZE     (450 * 1024)
#define ROM_SIZE     (128)    // disabled

#define I2C_MAX_PAYLOAD 2100
#define I2C_RX_BUF_SIZE (I2C_MAX_PAYLOAD * 2)
#define I2C_TX_BUF_SIZE (I2C_MAX_PAYLOAD * 2)

#define SYST_CSR (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)

void board_setup(void);
void wait_valid_clock(void);
void sync_with_vic(void);

void set_led_on(void);
void set_led_off(void);

#endif
