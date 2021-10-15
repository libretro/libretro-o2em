
/*
 *   O2EM Free Odyssey2 / Videopac+ Emulator
 *
 *   Created by Daniel Boris <dboris@comcast.net>  (c) 1997,1998
 *
 *   Developed by Andre de la Rocha   <adlroc@users.sourceforge.net>
 *             Arlindo M. de Oliveira <dgtec@users.sourceforge.net>
 *
 *   http://o2em.sourceforge.net
 *
 *
 *
 *   Keyboard emulation
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "config.h"
#include "vmachine.h"
#include "vdc.h"
#include "audio.h"
#include "voice.h"
#include "vpp.h"
#include "keyboard.h"

#include <libretro.h>
#include "wrapalleg.h"

#include "score.h"

int NeedsPoll = 0;

uint8_t keycode;
uint8_t last_key;
uint8_t new_int=0;	/* Is new interrupt installed */

uint8_t key_done=0;
uint8_t key_debug=0;

/* TODO/FIXME - we should find a way to do a non-timer based approach and get
 * rid of the sleeping here */

#define RETROK_TILDE 178

struct keyb keybtab[] = {
	{RETROK_a,"A"},
	{RETROK_b,"B"},
	{RETROK_c,"C"},
	{RETROK_d,"D"},
	{RETROK_e,"E"},
	{RETROK_f,"F"},
	{RETROK_g,"G"},
	{RETROK_h,"H"},
	{RETROK_i,"I"},
	{RETROK_j,"J"},
	{RETROK_k,"K"},
	{RETROK_l,"L"},
	{RETROK_m,"M"},
	{RETROK_n,"N"},
	{RETROK_o,"O"},
	{RETROK_p,"P"},
	{RETROK_q,"Q"},
	{RETROK_r,"R"},
	{RETROK_s,"S"},
	{RETROK_t,"T"},
	{RETROK_u,"U"},
	{RETROK_v,"V"},
	{RETROK_w,"W"},
	{RETROK_x,"X"},
	{RETROK_y,"Y"},
	{RETROK_z,"Z"},
	{RETROK_0,"0"},
	{RETROK_1,"1"},
	{RETROK_2,"2"},
	{RETROK_3,"3"},
	{RETROK_4,"4"},
	{RETROK_5,"5"},
	{RETROK_6,"6"},
	{RETROK_7,"7"},
	{RETROK_8,"8"},
	{RETROK_9,"9"},
	{RETROK_KP0,"0_PAD"},
	{RETROK_KP1,"1_PAD"},
	{RETROK_KP2,"2_PAD"},
	{RETROK_KP3,"3_PAD"},
	{RETROK_KP4,"4_PAD"},
	{RETROK_KP5,"5_PAD"},
	{RETROK_KP6,"6_PAD"},
	{RETROK_KP7,"7_PAD"},
	{RETROK_KP8,"8_PAD"},
	{RETROK_KP9,"9_PAD"},
	{RETROK_TILDE,"TILDE"},
	{RETROK_MINUS,"MINUS"},
	{RETROK_EQUALS,"EQUALS"},
	{RETROK_BACKSPACE,"BACKSPACE"},
	{RETROK_TAB,"TAB"},
	{RETROK_LEFTBRACKET,"OPENBRACE"},
	{RETROK_RIGHTBRACKET,"CLOSEBRACE"},
	{RETROK_RETURN,"ENTER"},
	{RETROK_COLON,"COLON"},
	{RETROK_QUOTE,"QUOTE"},
	{RETROK_BACKSLASH,"BACKSLASH"},
//	{RETROK_BACKSLASH2,"BACKSLASH2"},
	{RETROK_COMMA,"COMMA"},
//	{RETROK_STOP,"STOP"},
	{RETROK_SLASH,"SLASH"},
	{RETROK_SPACE,"SPACE"},
	{RETROK_INSERT,"INSERT"},
	{RETROK_DELETE,"DEL"},
	{RETROK_HOME,"HOME"},
	{RETROK_END,"END"},
	{RETROK_PAGEUP,"PGUP"},
	{RETROK_PAGEDOWN,"PGDN"},
	{RETROK_LEFT,"LEFT"},
	{RETROK_RIGHT,"RIGHT"},
	{RETROK_UP,"UP"},
	{RETROK_DOWN,"DOWN"},
	{RETROK_KP_DIVIDE,"SLASH_PAD"},
	{RETROK_KP_MULTIPLY,"ASTERISK"},
	{RETROK_KP_MINUS,"MINUS_PAD"},
	{RETROK_KP_PLUS,"PLUS_PAD"},
	{RETROK_KP_PERIOD,"DEL_PAD"},
	{RETROK_KP_ENTER,"ENTER_PAD"},
	{RETROK_PRINT,"PRTSCR"},
	{RETROK_PAUSE,"PAUSE"},
//	{RETROK_ABNT_C1,"ABNT_C1"},
//	{RETROK_YEN,"YEN"},
//	{RETROK_KANA,"KANA"},
	{RETROK_AT,"AT"},
	{RETROK_CARET,"CIRCUMFLEX"},
//	{RETROK_COLON2,"COLON2"},
//	{RETROK_KANJI,"KANJI"},
	{RETROK_LSHIFT,"LSHIFT"},
	{RETROK_RSHIFT,"RSHIFT"},
	{RETROK_LCTRL,"LCONTROL"},
	{RETROK_RCTRL,"RCONTROL"},
	{RETROK_LALT,"ALT"},
	{RETROK_RALT,"ALTGR"},
	{RETROK_LMETA,"LWIN"},
	{RETROK_RMETA,"RWIN"},
	{RETROK_MENU,"MENU"},
	{RETROK_SCROLLOCK,"SCRLOCK"},
	{RETROK_NUMLOCK,"NUMLOCK"},
	{RETROK_F1,"F1"},
	{RETROK_F2,"F2"},
	{RETROK_F3,"F3"},
	{RETROK_F4,"F4"},
	{RETROK_F5,"F5"},
	{RETROK_F6,"F6"},
	{RETROK_F7,"F7"},
	{RETROK_F8,"F8"},
	{RETROK_F9,"F9"},
	{RETROK_F10,"F10"},
	{RETROK_F11,"F11"},
	{RETROK_F12,"F12"},
	{RETROK_ESCAPE,"ESC"},
    {0,""}
};

int joykeys[2][5] = {{0,0,0,0,0},{0,0,0,0,0}};
int joykeystab[128];
int syskeys[8] = {0,0,0,0,0,0,0,0};


void set_defjoykeys(int jn, int sc)
{
	//if (sc)
	//	set_joykeys(jn,RETROK_w,RETROK_s,RETROK_a,RETROK_d,RETROK_SPACE);
	//else
	//	set_joykeys(jn,RETROK_UP,RETROK_DOWN,RETROK_LEFT,RETROK_RIGHT,RETROK_l);
}

	
void set_defsystemkeys(void)
{
   set_systemkeys(RETROK_F12,RETROK_F1,RETROK_F4,RETROK_F5,RETROK_F8,RETROK_F2,RETROK_F3,RETROK_F6);
}



void set_joykeys(int jn, int up, int down, int left, int right, int fire){
	int i,j;
	if ((jn<0) || (jn>1)) return;
	joykeys[jn][0] = up;
	joykeys[jn][1] = down;
	joykeys[jn][2] = left;
	joykeys[jn][3] = right;
	joykeys[jn][4] = fire;

	for (i=0; i<128; i++) joykeystab[i]=0;

	for (j=0; j<2; j++)
		for (i=0; i<5; i++) {
			if ((joykeys[j][i]<1) || (joykeys[j][i]>127))
				joykeys[j][i] = 0;
			else
				joykeystab[joykeys[j][i]] = 1;
		}
}
	
void set_systemkeys(int k_quit,int k_pause,int k_debug,int k_reset,int k_screencap,int k_save,int k_load,int k_inject)
{
	syskeys[0] = k_quit;
	syskeys[1] = k_pause;
	syskeys[2] = k_debug;
	syskeys[3] = k_reset;
	syskeys[4] = k_screencap;
	syskeys[5] = k_save;
	syskeys[6] = k_load;
	syskeys[7] = k_inject;
}

#ifdef __LIBRETRO__
unsigned char key[256*2];
#endif

extern int joystick_data[2][5]; //Up, Down, Left, Right, "Action"

uint8_t keyjoy(int jn)
{
	uint8_t d;
	d=0xFF;
	if ((jn>=0) && (jn<=1))
   {
#ifdef __LIBRETRO__
		if (joystick_data[jn][0]) d &= 0xFE;
		if (joystick_data[jn][1]) d &= 0xFB;
		if (joystick_data[jn][2]) d &= 0xF7;
		if (joystick_data[jn][3]) d &= 0xFD;
		if (joystick_data[jn][4]) d &= 0xEF;
#else
		if (NeedsPoll) 
			poll_keyboard();			

		if (key[joykeys[jn][0]]) d &= 0xFE;
		if (key[joykeys[jn][1]]) d &= 0xFB;
		if (key[joykeys[jn][2]]) d &= 0xF7;
		if (key[joykeys[jn][3]]) d &= 0xFD;
		if (key[joykeys[jn][4]]) d &= 0xEF;
#endif
	}
	return d;
}


void init_keyboard(void)
{
	key_done=0;
	key_debug=0;  

#ifndef __LIBRETRO__ 
	install_keyboard();	
#else
	memset(key,0,512);
#endif
	new_int=1;
#ifndef __LIBRETRO__ 
	NeedsPoll = keyboard_needs_poll();
#else
	NeedsPoll = 1;
#endif
}


void Set_Old_Int9(void){
#ifndef __LIBRETRO__ 
   remove_keyboard();
#endif
   new_int=0;
}

