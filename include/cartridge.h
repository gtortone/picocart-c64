#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#define COMPILER_BARRIER() asm volatile("" ::: "memory")

uint8_t run_cart(char *filename=NULL, bool clear_buffer=true);

void run_cart_normal(void);
void run_cart_magic_desk(void);
void run_cart_ocean(void);
void run_cart_fun_play(void);
void run_cart_super_games(void);
void run_cart_easyflash(void);
void run_cart_dinamic(void);
void run_cart_zaxxon(void);

#endif
