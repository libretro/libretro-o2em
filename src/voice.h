#ifndef __VOICE_H
#define __VOICE_H

void init_voice(const char *path);
void update_voice(void);
void trigger_voice(int addr);
void reset_voice(void);
void set_voice_bank(int bank);
void close_voice(void);
void mute_voice(void);
int get_voice_status(void);

#endif

