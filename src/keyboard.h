#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include <stdint.h>

void Set_Old_Int9(void);
void init_keyboard(void);
void set_joykeys(int joy, int up, int down, int left, int right, int fire);
void set_systemkeys(int k_quit,int k_pause,int k_debug,int k_reset,int k_screencap,int k_save,int k_load,int k_inject);
void set_defjoykeys(int joy, int sc);
void set_defsystemkeys(void);
uint8_t keyjoy(int jn);

extern	uint8_t new_int;
extern int NeedsPoll;
extern uint8_t key_done;
extern int joykeys[2][5];
extern int joykeystab[128];
extern int syskeys[8];

struct keyb{
	int keybcode;
	char *keybname;
};

extern struct keyb keybtab[];

#endif

