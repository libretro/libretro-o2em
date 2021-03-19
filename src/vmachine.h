#ifndef VMACHINE_H
#define VMACHINE_H

#include <stdint.h>

#ifdef __LIBRETRO__
#include <stddef.h>
#include <stdbool.h>
size_t savestate_size(void);
bool savestate_to_mem(uint8_t *data, size_t size);
bool loadstate_from_mem(const uint8_t *data, size_t size);
#endif

#define LINECNT 21
#define MAXLINES 500
#define MAXSNAP 50

#define VBLCLK 5493
#define EVBLCLK_NTSC 5964
#define EVBLCLK_PAL 7259

#define FPS_NTSC 60
#define FPS_PAL 50

extern uint8_t dbstick1, dbstick2;
extern int last_line;

extern int evblclk;

extern int master_clk;		/* Master clock */
extern int int_clk;		/* counter for length of /INT pulses for JNI */
extern int h_clk;   /* horizontal clock */
extern uint8_t coltab[256];
extern int mstate;

extern uint8_t rom_table[8][4096];
extern uint8_t intRAM[];
extern uint8_t extRAM[];
extern uint8_t extROM[];
extern uint8_t VDCwrite[256];
extern uint8_t ColorVector[MAXLINES];
extern uint8_t AudioVector[MAXLINES];
extern uint8_t *rom;
extern uint8_t *megarom;

extern int frame;
extern int key2[128];
extern int key2vcnt;
extern unsigned long clk_counter;

extern int enahirq;
extern int pendirq;
extern int useforen;
extern long regionoff;
extern int sproff;
extern int tweakedaudio;

uint8_t read_P2(void);
int snapline(int pos, uint8_t reg, int t);
void ext_write(uint8_t dat, uint16_t adr);
uint8_t ext_read(uint16_t adr);
void handle_vbl(void);
void handle_evbl(void);
void handle_evbll(void);
uint8_t in_bus(void);
void write_p1(uint8_t d);
uint8_t read_t1(void);
void init_system(void);
void init_roms(void);
void run(void);
int savestate(char* filename);
int loadstate(char* filename);


extern struct resource {
	int bank;
	int debug;
	int stick[2];
	int sticknumber[2];
	int limit;
	int sound_en;
	int speed;
	int wsize;
	int fullscreen;
	int scanlines;
	int voice;
	int svolume;
	int vvolume;	
	int exrom;
	int three_k;
	int filter;
	int euro;
	int openb;
	int megaxrom;
	int vpp;
	int bios;
	unsigned long crc;
	char *window_title;
	char *scshot;
	int scoretype;
	int scoreaddress;
	int default_highscore;
	int breakpoint;
	char *statefile;
} app_data;


#endif  /* VMACHINE_H */

