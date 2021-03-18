#ifndef __AUDIO_H
#define __AUDIO_H

#define SOUND_BUFFER_LEN 1056

void update_audio(void);
void init_audio(void);
void close_audio(void);
void init_sound_stream(void);
void mute_audio(void);


extern int sound_IRQ;


#endif


