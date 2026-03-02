#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "board.h"
#include "hw_config.h"

static spi_t spi = {
      .hw_inst = spi0,
      .miso_gpio = SD_MISO,
      .mosi_gpio = SD_MOSI,
      .sck_gpio = SD_SCK,
      .baud_rate = 2 * 1000 * 1000,
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = SD_CS 
};

static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if
};

size_t sd_get_num(void) {
    return 1;
};

sd_card_t* sd_get_by_num(size_t num) {
   if (0 == num)
      return &sd_card;
   else
      return NULL;
};
