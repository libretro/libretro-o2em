#ifndef __AUDIO_H
#define __AUDIO_H

#define SOUND_BUFFER_LEN 1056

void update_audio(void);
void init_audio(void);
void close_audio(void);
void init_sound_stream(void);
void mute_audio(void);


#include <stdint.h>

extern int sound_IRQ;
extern int snd_shift_count;
extern uint32_t snd_period_acc;


#endif


