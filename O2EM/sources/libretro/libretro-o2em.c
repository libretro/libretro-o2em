#include "libretro.h"

#include "libretro-o2em.h"

extern unsigned short int mbmp[TEX_WIDTH * TEX_HEIGHT];
extern int SHIFTON,RLOOP,pauseg,SND ,snd_sampler;
extern short signed int SNDBUF[1024*2];
extern char RPATH[512];

extern void update_input(void);
extern void texture_init(void);
extern void Emu_init();
extern void Emu_uninit();

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

void retro_init(void)
{
	texture_init();	
}

void retro_deinit(void)
{	 
	 Emu_uninit();
}

unsigned retro_api_version(void)
{
   	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   	(void)port;
   	(void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   	memset(info, 0, sizeof(*info));
   	info->library_name     = "O2EM";
   	info->library_version  = "1.18";
   	info->valid_extensions = "bin";
   	info->need_fullpath    = true;
        info->block_extract = false;	
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   	struct retro_game_geometry geom = { TEX_WIDTH, TEX_HEIGHT, TEX_WIDTH, TEX_HEIGHT,4.0 / 3.0 };
   	struct retro_system_timing timing = { 60.0, 44100.0 };
   
   	info->geometry = geom;
   	info->timing   = timing;
}
 
void retro_set_environment(retro_environment_t cb)
{
   	environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   	audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   	video_cb = cb;
}


void retro_reset(void){}

void retro_shutdown_o2em(void)
{
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void dumpaudio(int len){
	int x;
	   	if(SND==1)
		{
   	   		signed short int *p=(signed short int *)SNDBUF;
   	   		for(x=0;x<len;x++)audio_cb(*p,*p++);			
	   	}
}

void retro_run(void)
{
	RetroLoop();	   
	update_input();
   	video_cb(mbmp,EMUWITH,EMUHEIGHT,TEX_WIDTH << 1);
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
	char retrok=keycode;

  	// printf( "Down: %s, Code: %d, Char: %u, Mod: %u. ,(%d)\n",
  	//       down ? "yes" : "no", keycode, character, mod,cpck);

	if (keycode>=320);
	else{
	
		if(down && retrok==304/*0x2a*/){
										
			if(SHIFTON == 1)retro_key_up(retrok);
			else retro_key_down(retrok);
			SHIFTON=-SHIFTON;							
			
		}
		else {
			if(down && retrok!=-1)		
				retro_key_down(retrok);	
			else if(!down && retrok!=-1)
				retro_key_up(retrok);
		}	

	}

}

bool retro_load_game(const struct retro_game_info *info)
{
    	const char *full_path;

    	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    	{
    		fprintf(stderr, "RGB565 is not supported.\n");
    		return false;
    	}

    	struct retro_keyboard_callback cb = { keyboard_cb };
    	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

    	(void)info;

    	full_path = info->path;

    	strcpy(RPATH,full_path);

	RLOOP=1;

	Emu_init();

    	return true;
}

void retro_unload_game(void){

}

unsigned retro_get_region(void)
{
   	return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   	(void)type;
   	(void)info;
   	(void)num;
   	return false;
}

size_t retro_serialize_size(void)
{
   	return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   	return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
	return false;
}

void *retro_get_memory_data(unsigned id)
{
   	(void)id;
   	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   	(void)id;
   	return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   	(void)index;
   	(void)enabled;
   	(void)code;
}

