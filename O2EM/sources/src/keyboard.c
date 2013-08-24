
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
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "config.h"
#include "vmachine.h"
#include "vdc.h"
#include "audio.h"
#include "voice.h"
#include "vpp.h"
#include "keyboard.h"

#ifndef RETRO
#include "allegro.h"
#else
#include "libretro.h"
#include "wrapalleg.h"
#endif

#include "score.h"

int NeedsPoll = 0;

Byte keycode;
Byte last_key;
Byte new_int=0;	/* Is new interrupt installed */

Byte key_done=0;
Byte key_debug=0;
#ifndef RETRO

struct keyb keybtab[] = {
	{KEY_A,"A"},
	{KEY_B,"B"},
	{KEY_C,"C"},
	{KEY_D,"D"},
	{KEY_E,"E"},
	{KEY_F,"F"},
	{KEY_G,"G"},
	{KEY_H,"H"},
	{KEY_I,"I"},
	{KEY_J,"J"},
	{KEY_K,"K"},
	{KEY_L,"L"},
	{KEY_M,"M"},
	{KEY_N,"N"},
	{KEY_O,"O"},
	{KEY_P,"P"},
	{KEY_Q,"Q"},
	{KEY_R,"R"},
	{KEY_S,"S"},
	{KEY_T,"T"},
	{KEY_U,"U"},
	{KEY_V,"V"},
	{KEY_W,"W"},
	{KEY_X,"X"},
	{KEY_Y,"Y"},
	{KEY_Z,"Z"},
	{KEY_0,"0"},
	{KEY_1,"1"},
	{KEY_2,"2"},
	{KEY_3,"3"},
	{KEY_4,"4"},
	{KEY_5,"5"},
	{KEY_6,"6"},
	{KEY_7,"7"},
	{KEY_8,"8"},
	{KEY_9,"9"},
	{KEY_0_PAD,"0_PAD"},
	{KEY_1_PAD,"1_PAD"},
	{KEY_2_PAD,"2_PAD"},
	{KEY_3_PAD,"3_PAD"},
	{KEY_4_PAD,"4_PAD"},
	{KEY_5_PAD,"5_PAD"},
	{KEY_6_PAD,"6_PAD"},
	{KEY_7_PAD,"7_PAD"},
	{KEY_8_PAD,"8_PAD"},
	{KEY_9_PAD,"9_PAD"},
	{KEY_TILDE,"TILDE"},
	{KEY_MINUS,"MINUS"},
	{KEY_EQUALS,"EQUALS"},
	{KEY_BACKSPACE,"BACKSPACE"},
	{KEY_TAB,"TAB"},
	{KEY_OPENBRACE,"OPENBRACE"},
	{KEY_CLOSEBRACE,"CLOSEBRACE"},
	{KEY_ENTER,"ENTER"},
	{KEY_COLON,"COLON"},
	{KEY_QUOTE,"QUOTE"},
	{KEY_BACKSLASH,"BACKSLASH"},
	{KEY_BACKSLASH2,"BACKSLASH2"},
	{KEY_COMMA,"COMMA"},
	{KEY_STOP,"STOP"},
	{KEY_SLASH,"SLASH"},
	{KEY_SPACE,"SPACE"},
	{KEY_INSERT,"INSERT"},
	{KEY_DEL,"DEL"},
	{KEY_HOME,"HOME"},
	{KEY_END,"END"},
	{KEY_PGUP,"PGUP"},
	{KEY_PGDN,"PGDN"},
	{KEY_LEFT,"LEFT"},
	{KEY_RIGHT,"RIGHT"},
	{KEY_UP,"UP"},
	{KEY_DOWN,"DOWN"},
	{KEY_SLASH_PAD,"SLASH_PAD"},
	{KEY_ASTERISK,"ASTERISK"},
	{KEY_MINUS_PAD,"MINUS_PAD"},
	{KEY_PLUS_PAD,"PLUS_PAD"},
	{KEY_DEL_PAD,"DEL_PAD"},
	{KEY_ENTER_PAD,"ENTER_PAD"},
	{KEY_PRTSCR,"PRTSCR"},
	{KEY_PAUSE,"PAUSE"},
	{KEY_ABNT_C1,"ABNT_C1"},
	{KEY_YEN,"YEN"},
	{KEY_KANA,"KANA"},
	{KEY_AT,"AT"},
	{KEY_CIRCUMFLEX,"CIRCUMFLEX"},
	{KEY_COLON2,"COLON2"},
	{KEY_KANJI,"KANJI"},
	{KEY_LSHIFT,"LSHIFT"},
	{KEY_RSHIFT,"RSHIFT"},
	{KEY_LCONTROL,"LCONTROL"},
	{KEY_RCONTROL,"RCONTROL"},
	{KEY_ALT,"ALT"},
	{KEY_ALTGR,"ALTGR"},
	{KEY_LWIN,"LWIN"},
	{KEY_RWIN,"RWIN"},
	{KEY_MENU,"MENU"},
	{KEY_SCRLOCK,"SCRLOCK"},
	{KEY_NUMLOCK,"NUMLOCK"},
	{KEY_F1,"F1"},
	{KEY_F2,"F2"},
	{KEY_F3,"F3"},
	{KEY_F4,"F4"},
	{KEY_F5,"F5"},
	{KEY_F6,"F6"},
	{KEY_F7,"F7"},
	{KEY_F8,"F8"},
	{KEY_F9,"F9"},
	{KEY_F10,"F10"},
	{KEY_F11,"F11"},
	{KEY_F12,"F12"},
	{KEY_ESC,"ESC"},
    {0,""}
};
#else
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
#endif

int joykeys[2][5] = {{0,0,0,0,0},{0,0,0,0,0}};
int joykeystab[128];
int syskeys[8] = {0,0,0,0,0,0,0,0};


void set_defjoykeys(int jn, int sc){
	if (sc)
		set_joykeys(jn,RETROK_w,RETROK_s,RETROK_a,RETROK_d,RETROK_SPACE);
	else
		set_joykeys(jn,RETROK_UP,RETROK_DOWN,RETROK_LEFT,RETROK_RIGHT,RETROK_l);
}

	
void set_defsystemkeys(void){
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

#ifdef RETRO
unsigned char key[256*2];

void rloadstate(){

		int stateError;
		if ((stateError=loadstate(app_data.statefile))==0)
		{
			printf("Savefile loaded.\n");
		}
		else if (stateError>=199)
		{
			if (stateError==199) printf("Wrong ROM-File for Savefile.\n");
			else if (stateError==200+ROM_O2) printf("Wrong BIOS for Savefile: O2ROM needed.\n");
			else if (stateError==200+ROM_G7400) printf("Wrong BIOS for Savefile: G7400 ROM needed.\n");
			else if (stateError==200+ROM_C52) printf("Wrong BIOS for Savefile: C52 ROM needed.\n");
			else if (stateError==200+ROM_JOPAC) printf("Wrong BIOS for Savefile: JOPAC ROM needed.\n");
			else printf("Wrong BIOS for Savefile: UNKNOWN ROM needed.\n");
		}

}

void rsavestate(){

		if (savestate(app_data.statefile)==0)
		{
			printf("Savefile saved.\n");
		}
}

void rreset(){
		init_cpu();
		init_roms();
		init_vpp();
		clearscr();
}

void rscore(){

	set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);
}

void rscrshot(){
	//TODO
}

#endif

void handle_key(void){

	if (NeedsPoll)
		 poll_keyboard();

	if (key[syskeys[0]] || key[RETROK_ESCAPE]) {
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[0]] || key[RETROK_ESCAPE]);
		key_done=1;
	}

	if (key[syskeys[1]]) {
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[1]]);

		mute_audio();
		mute_voice();
		abaut();

		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

			if (key[RETROK_LALT] && key[RETROK_RETURN]) {
				app_data.fullscreen = app_data.fullscreen ? 0 : 1;
				grmode();
				abaut();
				do {
					rest(5);
					if (NeedsPoll)
						poll_keyboard();

				} while (key[RETROK_RETURN]);
			}		

		} while ((!key[syskeys[1]]) && (!key[RETROK_ESCAPE]) && (!key[syskeys[0]]));
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[1]]);
		
		init_sound_stream();
	}		

	if (key[syskeys[5]])
	{
		if (savestate(app_data.statefile)==0)
		{
			display_msg("Savefile saved.",5);
		}
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[5]]);

	}

	/* LOAD STATE */
	if (key[syskeys[6]])
	{
		int stateError;
		if ((stateError=loadstate(app_data.statefile))==0)
		{
			display_msg("Savefile loaded.",5);
		}
		else if (stateError>=199)
		{
			if (stateError==199) display_msg("Wrong ROM-File for Savefile.",5);
			else if (stateError==200+ROM_O2) display_msg("Wrong BIOS for Savefile: O2ROM needed.",5);
			else if (stateError==200+ROM_G7400) display_msg("Wrong BIOS for Savefile: G7400 ROM needed.",5);
			else if (stateError==200+ROM_C52) display_msg("Wrong BIOS for Savefile: C52 ROM needed.",5);
			else if (stateError==200+ROM_JOPAC) display_msg("Wrong BIOS for Savefile: JOPAC ROM needed.",5);
			else display_msg("Wrong BIOS for Savefile: UNKNOWN ROM needed.",5);
		}
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[6]]);
	}

	if (key[syskeys[2]]) key_debug=1;

	if (key[syskeys[3]]) {
		init_cpu();
		init_roms();
		init_vpp();
		clearscr();
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[syskeys[3]]);
	}

    /* SET HIGHSCORE */
	if (key[syskeys[7]])
	{
		set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);
	}

 
	if (key[syskeys[4]]) {
#ifndef RETRO
		BITMAP *bmp;
		PALETTE pal;
		char *p;
		static char name[1024];
		static int scshot_counter = 0;

		if (strlen(app_data.scshot)>0){
			if ((p=strchr(app_data.scshot,'@'))) {
				*p = 0;
				sprintf(name, "%s%02d%s", app_data.scshot, scshot_counter++, p+1);
				*p = '@';
			} else {
				strcpy(name, app_data.scshot);
			}
			get_palette(pal);
			bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
			save_bitmap(name, bmp, pal);
			destroy_bitmap(bmp);
			do {
				rest(5);
				if (NeedsPoll) 
				poll_keyboard();

			} while (key[syskeys[4]]);
		}
#endif
	}


	if (key[RETROK_LALT] && key[RETROK_RETURN]) {
		app_data.fullscreen = app_data.fullscreen ? 0 : 1;
		grmode();
		do {
			rest(5);
			if (NeedsPoll)
				poll_keyboard();

		} while (key[RETROK_RETURN]);
	}		

}

extern int jbt[5];//up dw lf rg fi

Byte keyjoy(int jn){
	Byte d;
	d=0xFF;
	if ((jn>=0) && (jn<=1)){
#ifdef RETRO
		if (jbt[0]) d &= 0xFE;
		if (jbt[1]) d &= 0xFB;
		if (jbt[2]) d &= 0xF7;
		if (jbt[3]) d &= 0xFD;
		if (jbt[4]) d &= 0xEF;
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


void init_keyboard(void){
	key_done=0;
	key_debug=0;  

#ifndef RETRO 
	install_keyboard();	
#else
	memset(key,0,512);
#endif
	new_int=1;
#ifndef RETRO 
	NeedsPoll = keyboard_needs_poll();
#else
	NeedsPoll = 1;
#endif
}


void Set_Old_Int9(void){
#ifndef RETRO 
   remove_keyboard();
#endif
   new_int=0;
}

