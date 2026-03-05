
#ifndef I2C_HANDLER_
#define I2C_HANDLER_

void i2c_init_regspace(void);
void i2c_debug(void);
void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);

#endif
