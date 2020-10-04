
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
 *   O2 audio emulation
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "config.h"
#include "vmachine.h"
#include "audio.h"

#define SAMPLE_RATE 44100
#define PERIOD1 11
#define PERIOD2 44

#define SOUND_BUFFER_LEN 1056

#define AUD_CTRL  0xAA
#define AUD_D0	  0xA7
#define AUD_D1	  0xA8
#define AUD_D2	  0xA9

extern int SND;
extern uint8_t soundBuffer[SOUND_BUFFER_LEN];

int sound_IRQ;

static double flt_a=0.0, flt_b=0.0;
static unsigned char flt_prv = 0;


static void filter(unsigned char *buffer, unsigned long len)
{
	static unsigned char buf[SOUND_BUFFER_LEN];
	unsigned long i;
	if (len>SOUND_BUFFER_LEN)
      return;

	memcpy(buf,buffer,len);	

	for (i=0; i<len; i++)
   {
		int t = (i==0)?(buf[0]-flt_prv):(buf[i]-buf[i-1]);
		if (t) flt_b = (double)t;
		flt_a += flt_b/4.0 - flt_a/80.0;
		flt_b -= flt_b/4.0;
		if ((flt_a>255.0)||(flt_a<-255.0))
         flt_a=0.0;
		buffer[i] = (unsigned char)((flt_a+255.0)/2.0);
	}
	flt_prv = buf[len-1];
}

void audio_process(unsigned char *buffer)
{
   unsigned long aud_data = (VDCwrite[AUD_D2] | 
         (VDCwrite[AUD_D1] << 8) | (VDCwrite[AUD_D0] << 16));

   int intena = VDCwrite[0xA0] & 0x04;
   int pnt    = 0;
   int cnt    = 0;

   int noise = VDCwrite[AUD_CTRL] & 0x10;
   int enabled = VDCwrite[AUD_CTRL] & 0x80;
   int rndbit = (enabled && noise) ? (rand()%2) : 0;

   while (pnt < SOUND_BUFFER_LEN)
   {
      int period, re_circ;

      int pos = (tweakedaudio) ? (pnt/3) : (MAXLINES-1);
      int volume = AudioVector[pos] & 0x0F;
      enabled = AudioVector[pos] & 0x80;
      period = (AudioVector[pos] & 0x20) ? PERIOD1 : PERIOD2;
      re_circ = AudioVector[pos] & 0x40;

      buffer[pnt++] = (enabled) ? ((aud_data & 0x01)^rndbit) * (0x10 * volume) : 0;
      cnt++;

      if (cnt >= period)
      {
         cnt=0;
         aud_data = (re_circ) ? ((aud_data >> 1) | ((aud_data & 1) << 23)) : (aud_data >> 1);
         rndbit = (enabled && noise) ? (rand()%2) : 0;

         if (enabled && intena && (!sound_IRQ))
         {
            sound_IRQ = 1;
            ext_IRQ();
         }		
      }
   }

   if (app_data.filter)
      filter(buffer, SOUND_BUFFER_LEN);	
}



void update_audio(void)
{
   audio_process(soundBuffer);
}


void init_audio(void)
{
   sound_IRQ=0;		
   if ((app_data.sound_en) || (app_data.voice))
      init_sound_stream();
}


void init_sound_stream(void)
{
#if 0
   int vol;
#endif
   if (app_data.sound_en)
   {
#if 0
      if (app_data.filter)
         vol = (255*app_data.svolume)/100;
      else
         vol = (255*app_data.svolume)/200;
#endif
      flt_a = flt_b = 0.0;
      flt_prv = 0;
   }
}


void mute_audio(void)
{
	SND=0;
}

void close_audio(void)
{
	app_data.sound_en=0;
}


