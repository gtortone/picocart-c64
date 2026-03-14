
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/structs/xip.h"

#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/xip_ctrl.h"

#include "board.h"

void sync_with_vic(void) {
   // wait until the raster beam is in the upper or lower border (if VIC-II is enabled)
   uint32_t control;
   do {
      GPIO_GET_LOW_32(control);
   } while( !(control & BA_MASK) );
}

void board_setup(void) {

   // standard overclock
   //set_sys_clock_khz(250000, true);
   //
   // EasyFlash cartridge overclock
   vreg_set_voltage(VREG_VOLTAGE_1_20);
   sleep_ms(300);
   set_sys_clock_khz(330000, true);

   gpio_set_function(UART_TX, UART_FUNCSEL_NUM(UART_ID, UART_TX));
   gpio_set_function(UART_RX, UART_FUNCSEL_NUM(UART_ID, UART_RX));
   uart_init(UART_ID, UART_BAUDRATE);
   stdio_uart_init_full(UART_ID, 115200, UART_TX, UART_RX);

   // configure PSRAM
   gpio_set_function(PSRAM_CS, GPIO_FUNC_XIP_CS1); // CS for PSRAM
	xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;

   gpio_init(I2C_SDA);
   gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
   gpio_disable_pulls(I2C_SDA);

   gpio_init(I2C_SCL);
   gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
   gpio_disable_pulls(I2C_SCL);

   i2c_init(I2C_PORT, I2C_BAUDRATE);

   gpio_init(PHI2);
   gpio_disable_pulls(PHI2);
   gpio_set_dir(PHI2, GPIO_IN);

   gpio_init(ROML);
   gpio_disable_pulls(ROML);
   gpio_set_dir(ROML, GPIO_IN);

   gpio_init(ROMH);
   gpio_disable_pulls(ROMH);
   gpio_set_dir(ROMH, GPIO_IN);

   gpio_init(IO1);
   gpio_disable_pulls(IO1);
   gpio_set_dir(IO1, GPIO_IN);

   gpio_init(IO2);
   gpio_disable_pulls(IO2);
   gpio_set_dir(IO2, GPIO_IN);

   gpio_init(RW);
   gpio_disable_pulls(RW);
   gpio_set_dir(RW, GPIO_IN);

   gpio_init(BA);
   gpio_disable_pulls(BA);
   gpio_set_dir(BA, GPIO_IN);

   gpio_init(LED);
   gpio_set_dir(LED, GPIO_OUT);

   gpio_init(EXROM);
   gpio_disable_pulls(EXROM);
   gpio_set_dir(EXROM, GPIO_OUT);

   gpio_init(GAME);
   gpio_disable_pulls(GAME);
   gpio_set_dir(GAME, GPIO_OUT);

   gpio_init(RESET);
   gpio_set_dir(RESET, GPIO_OUT);

   gpio_init_mask(ADDR_GPIO_MASK | DATA_GPIO_MASK);
   gpio_set_dir_in_masked(ADDR_GPIO_MASK | DATA_GPIO_MASK);

   for(int i=PINROMADDR; i<ADDRWIDTH; i++)
      gpio_disable_pulls(i);

   for(int i=PINROMDATA; i<DATAWIDTH; i++) {
      gpio_disable_pulls(PINROMDATA+i);
      gpio_set_slew_rate(PINROMDATA+i, GPIO_SLEW_RATE_FAST);
   }

   // set DATA lines (D0...D7) as input
   SET_DATA_MODE_IN
}

void set_led_on(void) {
   gpio_put(LED, 1);
}

void set_led_off(void) {
   gpio_put(LED, 0);
}
