#include "libretro.h"

#include "libretro-o2em.h"

#include "graph.h"
#include "vkbd.h"

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

unsigned short int mbmp[TEX_WIDTH * TEX_HEIGHT];

int RLOOP=1,NPAGE=-1, KCOL=1, BKGCOLOR=0;
int SHIFTON=-1,SHOWKEY=-1,STATUTON=-1;
int PAUSE=-1,SND=1; 

short signed int SNDBUF[1024*2];
int snd_sampler = 44100 / 60;
char RPATH[512];

static int mbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

long mframe=0;
unsigned long  Ktime=0 , LastFPSTime=0;

int BOXDEC= 32+2;
int STAT_BASEY=CROP_HEIGHT;

int jbt[5]={0,0,0,0,0};
extern unsigned char key[256*2];

void RetroLoop(){

	if(PAUSE==-1)cpu_exec();
	RLOOP=1;
}

extern int omain(int argc, char *argv[]);

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   	input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   	input_poll_cb = cb;
}

long GetTicks(void)
{ // in MSec
   	struct timeval tv;
   	gettimeofday (&tv, NULL);
   	return (tv.tv_sec*1000000 + tv.tv_usec)/1000;                                                                              
} 

void texture_init(){

	memset(mbmp, 0, sizeof(mbmp));
}

void Emu_init(){

	int result = 0;

	char **argv2 = (char *[]){"o2em\0", "vp_16.bin\0","\0"};

	argv2[1]=RPATH;
	omain(2,argv2);
}

void Emu_uninit(){

	close_audio();
	close_voice();
	close_display();
	retro_destroybmp();
}

void Print_Statut(){

	Draw_text((char*)mbmp,STAT_DECX+40 ,STAT_BASEY,0xffff,0x8080,1,2,40,(SHIFTON>0?"SHFT":""));
        Draw_text((char*)mbmp,STAT_DECX+80 ,STAT_BASEY,0xffff,0x8080,1,2,40,"MS:%d",0);
	Draw_text((char*)mbmp,STAT_DECX+120,STAT_BASEY,0xffff,0x8080,1,2,40,"Joy:%d",0);
}

void retro_key_down(unsigned char retrok){
	key[retrok]=1;
}
 
void retro_key_up(unsigned char retrok){
	key[retrok]=0; 
}

/*
L2  show/hide Statut
R2  swap kbd pages
L   show/hide vkbd
R   switch Shift ON/OFF
SEL EMU KSPACE
STR EMU KRETURN
A   fire/mousea/valid key in vkbd
B   EMU KL 
X   EMU K0
Y   EMU K1
*/
void update_joy(void){

	int i;
   	input_poll_cb();

	if(SHOWKEY!=1){
		for(i=4;i<9;i++)jbt[i-4]= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i ); // Joy press	UP/DW/RT/LF/A

		key[RETROK_l]=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 0 ); //B -> L
        	key[RETROK_RETURN]=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 3 ); // START -> RETURN
		key[RETROK_SPACE]=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 2 ); // SELECT -> SPACE
        	key[RETROK_0]=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 9 ); // X -> 0
		key[RETROK_1]=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 1 ); // Y -> 1
		
	}
}

void update_input(void)
{
	int i;
	//   RETRO      B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
        //   INDEX      0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
	static int vbt[16]={0x1C,0x39,0x01,0x3B,0x01,0x02,0x04,0x08,0x80,0x6D,0x15,0x31,0x24,0x1F,0x6E,0x6F};
	static int oldi=-1;
	static int vkx=0,vky=0;

	if(oldi!=-1){		
		retro_key_up(oldi);
		oldi=-1;
	}

   	input_poll_cb();


	i=10;//show vkey toggle
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		SHOWKEY=-SHOWKEY;		
	}	

        i=11;//switch shift On/Off 
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		SHIFTON=-SHIFTON;		
	}

	i=12;//show/hide statut
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		STATUTON=-STATUTON;
	}

	i=13;//swap kbd pages
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		if(SHOWKEY==1){
			NPAGE=-NPAGE;			
		}
	}
		
	if(SHOWKEY==1){

		static int vkflag[5]={0,0,0,0,0};		
		
		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && vkflag[0]==0 )
		    	vkflag[0]=1;
		else if (vkflag[0]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ){
		   	vkflag[0]=0;
			vky -= 1; 
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && vkflag[1]==0 )
		    	vkflag[1]=1;
		else if (vkflag[1]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ){
		   	vkflag[1]=0;
			vky += 1; 
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && vkflag[2]==0 )
		    	vkflag[2]=1;
		else if (vkflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ){
		   	vkflag[2]=0;
			vkx -= 1;
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && vkflag[3]==0 )
		    	vkflag[3]=1;
		else if (vkflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ){
		   	vkflag[3]=0;
			vkx += 1;
		}

		if(vkx<0)vkx=9;
		if(vkx>9)vkx=0;
		if(vky<0)vky=4;
		if(vky>4)vky=0;

		virtual_kdb((char*)mbmp,vkx,vky);

		i=8;
		if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==0) 	
			vkflag[4]=1;
		else if( !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==1) {

			vkflag[4]=0;
			i=check_vkey2(vkx,vky);

			if(i==-2){ //SWAP VKBD PAGE
				NPAGE=-NPAGE;oldi=-1;				
			}
			else if(i==-1){ //NOP
				oldi=-1;
			}
			else if(i==-3){//KDB bgcolor				
				KCOL=-KCOL;
				oldi=-1;
			}
			else if(i==-4){//VKbd show/hide 
				oldi=-1;			
				SHOWKEY=-SHOWKEY;
			}
			else if(i==-5){//inject 
				rscore();
				oldi=-1;
			}
			else if(i==-6){//quit 
				retro_shutdown_o2em();
				oldi=-1;
			}
			else if(i==-7){//pause 
				PAUSE=-PAUSE;				
				oldi=-1;
			}
			else if(i==-8){//reset 
				rreset();
				oldi=-1;
			}
			else if(i==-9){//screenshot 
				//TODO
				oldi=-1;
			}
			else if(i==-10){//save 
				rsavestate();
				oldi=-1;
			}
			else if(i==-11){//load 
				rloadstate();
				oldi=-1;
			}
			else if(i==-12){//SOUND ON/OFF 
				SND=-SND;
				oldi=-1;
			}
			else {	
				if(i==304){					
					
					if(SHIFTON == 1)retro_key_up(i);
					else retro_key_down(i);
					SHIFTON=-SHIFTON;				
					oldi=-1;
				}
				else {
					oldi=i;
					retro_key_down(i);					
				}
			}				

		}

         	if(STATUTON==1)Print_Statut();

		return;
	}

	if(STATUTON==1)Print_Statut();

}

