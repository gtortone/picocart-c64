
#include <stdio.h>      // printf
#include <cstdint>
#include "pico/stdlib.h"
#include "pico/i2c_slave.h"

#include "board.h"

#define SOF          0xAA
#define FRAME_ERR    0xC0

#define ERR_CRC      0x01  // wrong CRC
#define ERR_UNK      0x02  // unknown command
#define ERR_LEN      0x03  // payload length not valid
#define ERR_BUSY     0x04  // slave busy
#define ERR_DENY     0x05  // access denied
#define ERR_INDEX    0x06  // index out of range
#define ERR_NPARAMS  0x07  // number of params not enough

//#define MAX_PAYLOAD 24
//#define RX_BUF_SIZE 32
//#define TX_BUF_SIZE 32
#define MAX_PAYLOAD 650
#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024
#define MAX_REGISTERS_NUM 255

static uint8_t rxbuf[RX_BUF_SIZE];
static uint8_t txbuf[TX_BUF_SIZE];

static volatile int rx_index = 0;
static volatile int tx_len = 0;
static volatile int tx_index = 0;

static uint8_t regs[MAX_REGISTERS_NUM];

extern uint8_t *hram;

//

void i2c_init_regspace(void) {
   regs[0] = 64;     // page size
   regs[1] = 42;     // test value
}

//

uint16_t crc16_ccitt(uint8_t *data, int len) {

   uint16_t crc = 0xFFFF;
   
   for(int i=0;i<len;i++) {
      crc ^= (uint16_t)data[i] << 8;
   
      for(int j=0;j<8;j++) {
         if(crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
         else
            crc <<= 1;
      }
   }
   return crc;
}

//

int handle_command(uint8_t cmd, uint8_t *data, uint8_t len, uint8_t *resp, int *error) {

   uint32_t addr;

   switch(cmd) {

      case 0x01: // ping
         resp[0] = 0x42;
         return 1;
      
      case 0x02: // read register
         if(len >= 1)
            if(data[0] > MAX_REGISTERS_NUM) {
               resp[0] = ERR_INDEX;
               *error = 1;
               return 1;
            }
         resp[0] = regs[data[0]];
         return 1;

      case 0x03: // write register
         if(len >= 2) {
            if(data[0] > MAX_REGISTERS_NUM) {
               resp[0] = ERR_INDEX;
               *error = 1;
               return 1;
            }
            regs[data[0]] = data[1];
         } else {
               resp[0] = ERR_NPARAMS;
               *error = 1;
               return 1;
         }
         return 0;

      case 0x12: // read cartridge location
         if(len >= 3) {
            addr = (data[0] << 16) | (data[1] << 8) | data[2];
            if(addr > ROM_SIZE) {
               resp[0] = ERR_INDEX;
               *error = 1;
               return 1;
            }
         } else {
            resp[0] = ERR_NPARAMS;
            *error = 1;
            return 1;
         }
         resp[0] = hram[addr];
         return 1;

      case 0x13: // write cartridge location
         if(len >= 4) {
            addr = (data[0] << 16) | (data[1] << 8) | data[2];
            if(addr > ROM_SIZE) {
               resp[0] = ERR_INDEX;
               *error = 1;
               return 1;
            }
         } else {
            resp[0] = ERR_NPARAMS;
            *error = 1;
            return 1;
         }
         hram[addr] = data[3];
         return 0;

      case 0x23: // write cartridge buffer
         if(len >= 4) {
            addr = (data[0] << 16) | (data[1] << 8) | data[2];
            if(addr > ROM_SIZE) {
               resp[0] = ERR_INDEX;
               *error = 1;
               return 1;
            }
         } else {
            resp[0] = ERR_NPARAMS;
            *error = 1;
            return 1;
         }
         for(int i=3; i<len; i++)
            hram[addr+i-3] = data[i];
         return 0;

      case 0x70: // set led
         if(len >= 1)
            gpio_put(LED, data[0]);
         return 0;
      
      default:
         return -1;     // command not defined
    }
}

//

void process_frame() {

   if(rx_index < 6)
      return;
   
   if(rxbuf[0] != SOF)
      return;
   
   uint8_t seq = rxbuf[1];
   uint8_t cmd = rxbuf[2];
   uint8_t len = rxbuf[3];
   
   if(len > MAX_PAYLOAD)
      return;
   
   uint16_t crc_rx = ((uint16_t)rxbuf[4+len] << 8) | rxbuf[5+len];
   
   uint16_t crc = crc16_ccitt(&rxbuf[1],3+len);
   
   if(crc != crc_rx) {
      txbuf[0] = SOF;
      txbuf[1] = seq;
      txbuf[2] = FRAME_ERR;   // error frame
      txbuf[3] = 1;           // len
      txbuf[4] = ERR_CRC;     // CRC error

      uint16_t crc_tx = crc16_ccitt(&txbuf[1],4);

      txbuf[5] = crc_tx >> 8;
      txbuf[6] = crc_tx & 0xFF;

      tx_len = 7;
      tx_index = 0;

      return;
   }
   
   int error = 0;
   int resp_len = handle_command(cmd, &rxbuf[4], len, &txbuf[4], &error);

   txbuf[0] = SOF;
   txbuf[1] = seq;
   
   if(resp_len < 0) {
      txbuf[2] = FRAME_ERR;   // cmd
      txbuf[3] = 1;           // len
      txbuf[4] = ERR_UNK;     // command unknown
      resp_len = 1;
   } else {
      txbuf[2] = (error != 0) ? FRAME_ERR : (cmd | 0x80);
      txbuf[3] = resp_len;
   }
   
   uint16_t crc_tx = crc16_ccitt(&txbuf[1],3+resp_len);
   
   txbuf[4+resp_len] = crc_tx >> 8;
   txbuf[5+resp_len] = crc_tx & 0xFF;
   
   tx_len = 6 + resp_len;
   tx_index = 0;
}

// 

void i2c_debug(void) {

   printf("rx_index: %d\n", rx_index);
   for(int i=0; i<RX_BUF_SIZE; i++)
      printf("RXBUF[%d]: 0x%X\n", i, rxbuf[i]);

   printf("tx_index: %d, tx_len: %d\n", tx_index, tx_len);
   for(int i=0; i<TX_BUF_SIZE; i++)
      printf("TXBUF[%d]: 0x%X\n", i, txbuf[i]);
}

//

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {

   switch(event) {
      case I2C_SLAVE_RECEIVE:
         if(rx_index < RX_BUF_SIZE) {
            rxbuf[rx_index++] = i2c_read_byte_raw(i2c);
         }
         break;
      
      case I2C_SLAVE_REQUEST:
         if(tx_index < tx_len) {
            i2c_write_byte_raw(i2c, txbuf[tx_index++]);
         } else {
            i2c_write_byte_raw(i2c, 0x00);
         }
         break;
      
      case I2C_SLAVE_FINISH:
         process_frame();
         rx_index = 0;
         break;
      
      default:
         break;
   }
}
