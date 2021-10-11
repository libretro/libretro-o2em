#ifndef __VPP_H
#define __VPP_H

#include <stdint.h>

uint8_t read_PB(uint8_t p);
void write_PB(uint8_t p, uint8_t val);
uint8_t vpp_read(uint16_t adr);
void vpp_write(uint8_t dat, uint16_t adr);
void vpp_finish_bmp(uint8_t *vmem, int offx, int offy, int w, int h, int totw, int toth);
void init_vpp(void);
void close_vpp(void);
void load_colplus(uint8_t *col);

#endif
