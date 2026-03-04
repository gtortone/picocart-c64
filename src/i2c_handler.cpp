
#include <cstdint>

#include "pico/i2c_slave.h"

#define OP_REGISTER_READ      0x10
#define OP_REGISTER_WRITE     0x11

static bool op_code_received = false;
static bool reg_addr_received = false;
static uint8_t op_code;
static uint8_t reg_addr;
static uint8_t reg_value;

static uint8_t regs[255];

void i2c_init_regspace(void) {
   regs[0] = 64;     // page size
   regs[1] = 42;     // test value
}

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
   switch (event) {
      case I2C_SLAVE_RECEIVE: // master has written some data
         if(!op_code_received) {
            op_code_received = true;
            op_code = i2c_read_byte_raw(i2c);
         } else {
            if(op_code == OP_REGISTER_READ) {
               if(!reg_addr_received) {
                  reg_addr_received = true;
                  reg_addr = i2c_read_byte_raw(i2c);
               } 
            } else if(op_code == OP_REGISTER_WRITE) {
               if(!reg_addr_received) {
                  reg_addr_received = true;
                  reg_addr = i2c_read_byte_raw(i2c);
               } else {       // fetch reg value
                  reg_value = i2c_read_byte_raw(i2c);
                  regs[reg_addr] = reg_value;
               }
            }
         }
         break;
      case I2C_SLAVE_REQUEST: // master is requesting data
         if(op_code == OP_REGISTER_READ) {
            i2c_write_byte_raw(i2c, regs[reg_addr]);
         }
         break;
      case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
         op_code_received = false;
         reg_addr_received = false;
         break;
      default:
         break;
   }
}

