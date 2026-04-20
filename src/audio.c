
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

#define PERIOD1 11
#define PERIOD2 44

#define AUD_CTRL  0xAA
#define AUD_D0	  0xA7
#define AUD_D1	  0xA8
#define AUD_D2	  0xA9

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

void audio_process(unsigned char *buffer){

	unsigned long old_aud_data, aud_data;
	int pos, pnt, cnt, period, noise, enabled, volume;
	pnt = cnt = 0;

	noise = VDCwrite[AUD_CTRL] & 0x10;
	enabled = VDCwrite[AUD_CTRL] & 0x80;

	/* Generate the aud_data */
	old_aud_data = aud_data = VDCwrite[AUD_D2] | (VDCwrite[AUD_D1] << 8) | (VDCwrite[AUD_D0] << 16);

	if(enabled)  /* Sound is enabled */
	{
		for( pnt = 0; pnt < SOUND_BUFFER_LEN; pnt++)
		{
			pos = (tweakedaudio) ? (pnt/3) : (MAXLINES-1);
			volume = AudioVector[pos] & 0x0F;
			buffer[pnt] = (aud_data & 0x01) * (0x10 * volume);
			period = (AudioVector[pos] & 0x20) ? PERIOD1 : PERIOD2;
			enabled = AudioVector[pos] & 0x80;
			if( ++cnt >= period )
			{
				cnt = 0;
				aud_data = ((aud_data >> 1) | ((aud_data & 1) << 23));
				/* Check if noise should be applied */
				if (noise)
				{
					/* Noise tap is on bits 0 and 5 and fed back to bits 15 (and 23!) */
					unsigned long new_bit = ( ( old_aud_data ) ^ ( old_aud_data >> 5 ) ) & 0x01;
					aud_data = ( old_aud_data & 0xFF0000 ) | ( ( old_aud_data & 0xFFFF ) >> 1 ) | ( new_bit << 15 ) | ( new_bit << 23 );
				}
				VDCwrite[AUD_D2] = aud_data & 0xFF;
				VDCwrite[AUD_D1] = ( aud_data >> 8 ) & 0xFF;
				VDCwrite[AUD_D0] = ( aud_data >> 16 ) & 0xFF;
				old_aud_data = aud_data;

			}

		}
	}
	else
	{
		/* Sound disabled, so clear the buffer */
		for( pnt = 0; pnt < SOUND_BUFFER_LEN; pnt++)
		{
			buffer[pnt] = 0;
		}
	}
	if (app_data.filter) filter(buffer, SOUND_BUFFER_LEN);
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
   if (app_data.sound_en)
   {
#if 0
      int vol;
      if (app_data.filter)
         vol = (255*app_data.svolume)/100;
      else
         vol = (255*app_data.svolume)/200;
#endif
      flt_a   = flt_b = 0.0;
      flt_prv = 0;
   }
}


void mute_audio(void)
{
}

void close_audio(void)
{
	app_data.sound_en=0;
}



