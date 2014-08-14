
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
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "crc32.h"
#include "audio.h"
#include "vmachine.h"
#include "config.h"
#include "vdc.h"
#include "cpu.h"
#include "debug.h"
#include "keyboard.h"
#include "voice.h"

#ifndef __LIBRETRO__
#include "allegro.h"
#else
#include "wrapalleg.h"

#ifdef AND
#warning android log
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "TAG", __VA_ARGS__)
#endif

#endif

#include "score.h"
#ifdef ALLEGRO_WINDOWS
#include "winalleg.h"
#endif
static char bios[MAXC], scshot[MAXC], xrom[MAXC], romdir[MAXC], xbios[MAXC], 
       biosdir[MAXC], arkivo[MAXC][MAXC], biossux[MAXC], romssux[MAXC], 
       odyssey2[MAXC], g7400[MAXC], c52[MAXC], jopac[MAXC], file_l[MAXC], bios_l[MAXC],
       file_v[MAXC],scorefile[MAXC], statefile[MAXC], path[MAXC]; 
FILE *f;
char name_f, rom_f, c_j;
char pathx, *k, *launcher_flag_r, *launcher_flag_b, identify;
int file_name(char *pathx);
int suck_bios();
int suck_roms();
int identify_bios(char *identify);
int contax, o2flag, g74flag, c52flag, jopflag, helpflag;
int j;
unsigned long crcx = ~0;
static long filesize(FILE *stream);
static void load_bios(const char *biosname);
static void load_cart(char *file);
static void helpus(void);
int parse_option(char *attr, char *val);
void read_default_config(void);
/********************* Main*/
int omain(int argc, char *argv[]){
	int i, cnt, cntt, cnttt, way;
	static char file[MAXC], attr[MAXC], val[MAXC], *p, *binver;
	#if defined(ALLEGRO_WINDOWS)
	binver = "Windows binary";
	#elif defined(ALLEGRO_DOS)
	binver = "DOS binary";
	#elif defined(ALLEGRO_LINUX)
	binver = "Linux binary";
	#elif defined(ALLEGRO_BEOS)
	binver = "BEOS binary";
	#elif defined(ALLEGRO_QNX)
	binver = "QNX binary";
	#elif defined(ALLEGRO_UNIX)
	binver = "UNIX binary";
	#elif defined(ALLEGRO_MPW)
	binver = "MacOS binary";
	#else
	binver = "Unknown binary";
	#endif
	printf("%s %s\n","\nO2EM v" O2EM_VERSION " " RELEASE_DATE "  - ", binver);
	printf("Free Odyssey2 / Videopac+ Emulator - http://o2em.sourceforge.net\n");
	printf("Created by Daniel Boris (c)1996/1998\n");
    printf("Developed by:\n");
	printf("     Andre de la Rocha since version 0.80\n");
	printf("     Arlindo M. de Oliveira since version 1.16\n");
    printf("\n");

    app_data.debug = 0;
	app_data.stick[0] = app_data.stick[1] = 1;
	app_data.sticknumber[0] = app_data.sticknumber[1] = 0;
	set_defjoykeys(0,0);
	set_defjoykeys(1,1);
	set_defsystemkeys();
	app_data.bank = 0;
	app_data.limit = 1;
	app_data.sound_en = 1;
	app_data.speed = 100;
	app_data.wsize = 2;
	#ifdef ALLEGRO_DOS
	app_data.fullscreen = 1;
	#else
	app_data.fullscreen = 0;
	#endif
	app_data.scanlines = 0;
	app_data.voice = 1;
	app_data.window_title = "O2EM v" O2EM_VERSION;
	app_data.svolume = 100;
	app_data.vvolume = 100;
	app_data.filter = 0;
	app_data.exrom = 0;
	app_data.three_k = 0;
	app_data.crc = 0;
	app_data.scshot = scshot;
	app_data.statefile = statefile;
	app_data.euro = 0;
	app_data.openb = 0;
	app_data.vpp = 0;
	app_data.bios = 0;
	app_data.scoretype = 0;
	app_data.scoreaddress = 0;
	app_data.default_highscore = 0;
	app_data.breakpoint = 65535;
	app_data.megaxrom = 0;
	strcpy(file,"");
	strcpy(file_l,"");
	strcpy(bios_l,"");
    strcpy(bios,"");
	strcpy(scshot,"");
	strcpy(statefile,"");
    strcpy(xrom,"");
	strcpy(scorefile,"highscore.txt");
	read_default_config();
	if (argc >= 2){
    for(i=1; i<argc; i++) {
		if (argv[i][0] != '-') 	{
			strncat(file,argv[i],MAXC-1);
	        file[MAXC-1]=0;
	        strcpy(file_v,file);
		} else {
			p=strtok(argv[i],"=");
	        if (p){
				strncpy(attr,p+1,MAXC-1);
				attr[MAXC-1]=0;
			   } else
				strcpy(attr,"");
			    p=strtok(NULL,"=");
			if (p){
				strncpy(val,p,MAXC-1);
				val[MAXC-1]=0;
			    if (!strcmp(attr,"romdir")||!strcmp(attr,"ROMDIR"))
                   {
                    strcpy(romdir,val);
                    strcat(romdir,file);
                    strcpy(file,romdir);
                    strcpy(romdir,val);
                   }
                if (!strcmp(attr,"biosdir")||!strcmp(attr,"BIOSDIR"))
                   {
                    strcpy(biosdir,val);
                   }                                       
            } else
			strcpy(val,"");
			strlwr(attr);
			if (!parse_option(attr, val)) exit(EXIT_FAILURE);
		}
    }
    if (helpflag) helpus();
    if (strlen(file)==0) {
		fprintf(stderr,"Error: file name missing\n");
		exit(EXIT_FAILURE);
	}

#ifdef __LIBRETRO__
sprintf(statefile,"%s.state\0",file);
#endif
	printf("Starting emulation ...\n");
#ifndef __LIBRETRO__
	allegro_init();
	install_timer();
#endif
	init_audio();

#ifndef __LIBRETRO__
	printf("Using Allegro %s\n",allegro_id);
#endif 


/********************** ROMs if Launcher running... */
    k = strchr(romdir, '/'); 

    launcher_flag_r = strchr(file, '\\');

    if (k != 0) {
                 strcpy (xrom,romdir);
                }
                else if (!launcher_flag_r)
                        {

                        strcpy(xrom,"roms/");
                        strcpy(romdir,file);
#ifndef __LIBRETRO__
                        strcpy(file,xrom);
                        strcat(file,romdir);
#endif
                        strcpy(romdir,xrom);

                        }
                        else
                        {    
         
                        cnt = 0;
                        cntt = 0;
                        cnttt = 0;
                        way = 0;
                        for (cnt=0; file[cnt] != '\0'; cnt=cnt+1) 
                        { 
                        if ( file[cnt] == '\\' ) 
                           {
                           cnttt = cnt;
                           }
                        } 
                        for (cnt=0; cnt<=cnttt; cnt++)
                        { 
                        file_l[cnt] = file[cnt];
                        } 

                        strcpy (romdir,file_l);
                        strcpy (xrom,romdir);
                        }

#ifdef __LIBRETRO__
#ifdef AND
	sprintf(xrom,"%s\0","/mnt/sdcard/O2EM/roms/");
	strcpy(romdir,xrom);
#else
	sprintf(xrom,"%s\0","./roms/");
	strcpy(romdir,xrom);
#endif
#endif


    file_name(xrom);

    if (contax < 3)
                 {
                 printf("\nROMs directory empty!\n");
                 exit(EXIT_FAILURE);
                 }

    app_data.crc = crc32_file(file);

    crcx = app_data.crc;
    suck_roms(); 

/********************** BIOSs if Launcher running... */     
launcher_flag_b = strchr(bios, '\\');

if (!launcher_flag_b){
    k = strchr(biosdir, '/');

    if (k != 0) {    
                 strcpy (xbios,biosdir);
                }
                else           
                        {
                        strcpy (xbios,"bios/");
                        strcpy (biosdir,xbios);
                        }
#ifdef __LIBRETRO__
#ifdef AND
	sprintf(xbios,"%s\0","/mnt/sdcard/O2EM/bios/");
	strcpy (biosdir,xbios);
#else	
	sprintf(xbios,"%s\0","./bios/");
	strcpy (biosdir,xbios);
#endif
#endif

    file_name(xbios);

    if (contax < 3)
                 {
                 printf("\nBIOS directory empty!\n");
                 exit(EXIT_FAILURE);                 
                 }

    suck_bios();

    c_j = strcmp(bios,"jopac");
    if ((rom_f!=1) && (c_j!=0)) strcpy(bios,g7400);
    if ((!o2flag) && (!jopflag) && (!c52flag) && (!g74flag))
                                              {
                                             printf("\ndir '%s' without BIOS !",biosdir);
                                             exit(EXIT_FAILURE);
                                              }
    printf("BIOS found:\n");
    if (!strcmp(bios,"g7400")){
                               strcpy(bios,g7400);
                               if (g74flag != 1) {
                                             printf("\nG7400 BIOS not found !");
                                             exit(EXIT_FAILURE);
                                             } 
                               }
    if (g74flag) printf("  G7400 VP+\n");
    if (!strcmp(bios,"c52")){ 
                             strcpy(bios,c52);
                             if (c52flag != 1) {
                                             printf("\nC52 BIOS not found !");
                                             exit(EXIT_FAILURE);
                                             } 
                                 }
    if (c52flag) printf("  C52\n");
    if (!strcmp(bios,"jopac")){
                               strcpy(bios,jopac);
                               if (jopflag != 1) {
                                          printf("\nJOPAC BIOS not found !");
                                          exit(EXIT_FAILURE);
                                             } 
                               }
    if (jopflag) printf("  JOPAC VP+\n");
    if ((!strcmp(bios,"")) || (!strcmp(bios,"o2rom")))
                            {
                            strcpy(bios,odyssey2);
                            if ((!o2flag)&&(!c52flag)&&(rom_f)){
                                             printf("Odyssey2 BIOS not found !\n");
                                             exit(EXIT_FAILURE);
                                             } 
                            if ((!o2flag)&&(c52flag)&&(rom_f)){
                                             printf("\nOdyssey2 BIOS not found !\n");
                                             printf("Loading C52 BIOS ... ");
                                             strcpy(bios,c52);
                                             }
                            }
    if (o2flag) printf("  Odyssey 2\n");
    }                                           
    if (launcher_flag_b)
       {
       identify_bios(bios);
                if (rom_f!=1)
                   {
                   if (!((g74flag)||(jopflag)))
                      {
                      fprintf(stderr,"\nError: ROM only VP+ BIOS");
                      exit(EXIT_FAILURE);
                      }
                   }
       }      


      if (!launcher_flag_b)
                  {  
                  if (rom_f!=1)
                     {  
                     if (!((g74flag)||(jopflag)))
                         {
                         printf("\nROM only VP+ BIOS\n");
                         exit(EXIT_FAILURE);
                         }
                     if (!(g74flag))
                         {
                         printf("\nVP+ G7400 BIOS not found !");
                         printf("\nLoading VP+ Jopac BIOS ...");
                         strcpy(bios,jopac);
                         }
                     }
                  }
    load_bios(bios);

	load_cart(file);
	if (app_data.voice) load_voice_samples(path);

	init_display();

	init_cpu();

	init_system();

	set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);
	int stateError;
	if ((stateError=loadstate(app_data.statefile))==0)
	{
		printf("Savefile loaded.");
	}
	else if (stateError>=199)
	{
		if (stateError==199) fprintf(stderr,"Wrong ROM-File for Savefile.");
		else if (stateError==200+ROM_O2) fprintf(stderr,"Wrong BIOS for Savefile: O2ROM needed.");
		else if (stateError==200+ROM_G7400) fprintf(stderr,"Wrong BIOS for Savefile: G7400 ROM needed.");
		else if (stateError==200+ROM_C52) fprintf(stderr,"Wrong BIOS for Savefile: C52 ROM needed.");
		else if (stateError==200+ROM_JOPAC) fprintf(stderr,"Wrong BIOS for Savefile: JOPAC ROM needed.");
		else fprintf(stderr,"Wrong BIOS for Savefile: UNKNOWN ROM needed.");
		return(0);
	}
	if (app_data.debug) key_debug=1;
	#ifndef _DEBUG
	#ifdef ALLEGRO_WINDOWS
	FreeConsole();
	#endif
	#endif

#ifdef __LIBRETRO__
return 1;
#endif
	run();

    if (app_data.scoretype!=0) save_highscore(get_score(app_data.scoretype, app_data.scoreaddress), scorefile);
	exit(EXIT_SUCCESS);
 }
if (!strcmp(attr,"help")||!strcmp(attr,"HELP")) helpus();
printf("type o2em -help");
exit(EXIT_SUCCESS);
}
END_OF_MAIN();
/******************************************************/
int parse_option(char *attr, char *val){
	int t;
	if (!strcmp(attr,"nolimit")) {
		app_data.limit = !(val[0]!='0');
	} else if (!strcmp(attr,"nosound")) {
		app_data.sound_en = !(val[0]!='0');
	} else if (!strcmp(attr,"novoice")) {
		app_data.voice = !(val[0]!='0');
	} else if (!strcmp(attr,"filter")) {
		app_data.filter = (val[0]!='0');
	} else if (!strcmp(attr,"debug")) {
		app_data.debug = (val[0]!='0');
	} else if ((!strcmp(attr,"s1")) || (!strcmp(attr,"s2"))) {
		int sn;
		sn = (!strcmp(attr,"s1"))? 0 : 1;
		if (strlen(val)<2){
			t = -1;
			sscanf(val,"%d",&t);
			if ((t>=0) && (t<=3)) {
				if ((t==1)||(t==2)){
					app_data.stick[sn] = 1;
					set_defjoykeys(sn,t-1);
				} else {
					app_data.stick[sn] = (t==0) ? 0 : 2;
					if (t==3)
					{
						app_data.sticknumber[sn] = app_data.sticknumber[0]+app_data.sticknumber[1]+1;
					}
                    set_joykeys(sn,0,0,0,0,0);
				}
			} else {
				fprintf(stderr,"Invalid value for option %s\n",attr);
				return 0;
			}
		} else {
			char *p,*s;
			int i,k,code,nk,codes[5];
			strupr(val);
			nk = 0;
			p = strtok(val,",");
			while (p) {
				i = code = 0;
				k = keybtab[i].keybcode;
				s = keybtab[i].keybname;
				while (k && (code==0)) {
					if (strcmp(s,p)==0) code = k;
					i++;
					k = keybtab[i].keybcode;
					s = keybtab[i].keybname;
				}
				if (!code) {
					fprintf(stderr,"Invalid value for option %s : key %s unknown\n",attr,p);
					return 0;
				}					
				codes[nk] = code;
				p = strtok(NULL,",");
				nk++;
				if (nk>5) {
					fprintf(stderr,"Invalid value for option %s : invalid number of keys\n",attr);
					return 0;
				}
			}
			if (nk != 5) {
				fprintf(stderr,"Invalid value for option %s : invalid number of keys\n",attr);
				return 0;
			}
			app_data.stick[sn] = 1;
			set_joykeys(sn,codes[0],codes[1],codes[2],codes[3],codes[4]);
		}




	} else if (!strcmp(attr,"s0")) {
			char *p,*s;
			int i,k,code,nk,codes[8];
			strupr(val);
			nk = 0;
			p = strtok(val,",");
			while (p) {
				i = code = 0;
				k = keybtab[i].keybcode;
				s = keybtab[i].keybname;
				while (k && (code==0)) {
					if (strcmp(s,p)==0) code = k;
					i++;
					k = keybtab[i].keybcode;
					s = keybtab[i].keybname;
				}
				if (!code) {
					fprintf(stderr,"Invalid value for option %s : key %s unknown\n",attr,p);
					return 0;
				}
				codes[nk] = code;
				p = strtok(NULL,",");
				nk++;
				if (nk>8) {
					fprintf(stderr,"Invalid value for option %s : invalid number of keys\n",attr);
					return 0;
				}
			}
			if (nk != 8) {
				fprintf(stderr,"Invalid value for option %s : invalid number of keys\n",attr);
				return 0;
			}
			set_systemkeys(codes[0],codes[1],codes[2],codes[3],codes[4],codes[5],codes[6],codes[7]);

} else if (!strcmp(attr,"speed")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>0) && (t<=10000))
			app_data.speed = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
	} else if (!strcmp(attr,"svolume")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>=0) && (t<=100))
			app_data.svolume = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
		if (t==0) app_data.sound_en=0;
	} else if (!strcmp(attr,"vvolume")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>=0) && (t<=100))
			app_data.vvolume = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
		if (t==0) app_data.voice=0;
	} else if (!strcmp(attr,"wsize")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>0) && (t<5)) {
			app_data.wsize = t;
			app_data.fullscreen = 0;
		} else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
	} else if (!strcmp(attr,"fullscreen")) {
		app_data.fullscreen = (val[0]!='0');
	} else if (!strcmp(attr,"scanlines")) {
		app_data.scanlines = (val[0]!='0');
	} else if (!strcmp(attr,"scshot")) {
		strcpy(scshot,val);
	} else if (!strcmp(attr,"savefile")) {
		strcpy(statefile,val);
	} else if (!strcmp(attr,"biosdir")) {
		strcpy(xbios,val);
	} else if (!strcmp(attr,"bios")) {
		strcpy(bios,val);
	} else if (!strcmp(attr,"romdir")) {
		strcpy(xrom,val);
	} else if (!strcmp(attr,"help")) {
		helpflag = 1;
	} else if (!strcmp(attr,"euro")) {
		app_data.euro = (val[0]!='0');
	} else if (!strcmp(attr,"exrom")) {
		app_data.exrom = (val[0]!='0');
	} else if (!strcmp(attr,"3k")) {
		app_data.three_k = (val[0]!='0');
	} else if (!strcmp(attr,"g7400")){
		strcpy(bios,"g7400");
	} else if (!strcmp(attr,"c52")) {
		strcpy(bios,"c52");
    } else if (!strcmp(attr,"jopac")) {
		strcpy(bios,"jopac");
    } else if (!strcmp(attr,"o2rom")) {
		strcpy(bios,"o2rom");
        } else if (!strcmp(attr,"scorefile")) {
		strcpy(scorefile,val);
    	} else if (!strcmp(attr,"scoreadr")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>=0) && (t<=255))
			app_data.scoreaddress = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
    	} else if (!strcmp(attr,"scoretype")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>=0) && (t<=9999))
			app_data.scoretype = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
    	} else if (!strcmp(attr,"score")) {
		t = -1;
		sscanf(val,"%d",&t);
		if ((t>=0) && (t<=999999))
			app_data.default_highscore = t;
		else {
			fprintf(stderr,"Invalid value for option %s\n",attr);
			return 0;
		}
    	} else {
               if (!helpflag){ 
		       fprintf(stderr,"Invalid option : %s\n",attr);
		       return 0;
               }
	}
	return 1;
}
/****************************************************/
void read_default_config(void){
	FILE *f;
	static char attr[MAXC], val[MAXC], s[MAXC];
	char *p, *fn;
	int i,l;
	fn = "o2em_def.cfg";
	f = fopen(fn,"r");
	if (!f) {
		fn = "O2EM_DEF.CFG";
		f = fopen(fn,"r");
	}
	if (!f) return;

	l=0;
	while (fgets(s,MAXC-1,f)){
		l++;
		p=s;
		while (*p && (isspace(*p))) p++;
		if (*p && (*p != '#')) {
			i=0;
			while (*p && (!isspace(*p)) && (*p != '=')) attr[i++] = *p++;
			attr[i]=0;
			while (*p && (isspace(*p))) p++;
			i=0;
			if (*p == '='){
				p++;
				while (*p && (isspace(*p))) p++;
				if (*p == '"'){
					p++;
					while (*p && (*p != '"') && (*p != '\n') && (*p != '\r')) val[i++] = *p++;
				} else {
					while (*p && (!isspace(*p))) val[i++] = *p++;
				}
			}
			val[i]=0;
			if (strlen(attr)>0) {
				strlwr(attr);
				if (!parse_option(attr,val)) {
					printf("Error in the %s file at line number %d !\n\n",fn,l);
				}
			}
		}
	}
	fclose(f);
}
/****************************************************/
static long filesize(FILE *stream){
   long curpos, length;
   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);
   return length;
}
/****************************************************/
static void load_bios(const char *biosname){
	FILE *fn;
	static char s[MAXC+10];
	unsigned long crc;
	int i;

        if ((biosname[strlen(biosname)-1]=='/') ||
           (biosname[strlen(biosname)-1]=='\\') ||
           (biosname[strlen(biosname)-1]==':')) {
		strcpy(s,biosname);
		strcat(s,odyssey2);
		fn = fopen(s,"rb");

		if (!fn) {
			strcpy(s,biosname);
			strcat(s,odyssey2);
			fn = fopen(s,"rb");
        		}
	      } else { 
	
    	strcpy(s,biosname);
		fn = fopen(biosname,"rb");
        }
	
    if (!fn) {
		fprintf(stderr,"Error loading bios ROM (%s)\n",s);
		exit(EXIT_FAILURE);
	}
 	if (fread(rom_table[0],1024,1,fn) != 1) {
 		fprintf(stderr,"Error loading bios ROM %s\n",odyssey2);
 		exit(EXIT_FAILURE);
 	}
    
	    strcpy(s,biosname);
		fn = fopen(biosname,"rb");
    if (!fn) {
		fprintf(stderr,"Error loading bios ROM (%s)\n",s);
		exit(EXIT_FAILURE);
	}
 	if (fread(rom_table[0],1024,1,fn) != 1) {
 		fprintf(stderr,"Error loading bios ROM %s\n",odyssey2);
 		exit(EXIT_FAILURE);
 	}
    fclose(fn);
	for (i=1; i<8; i++) memcpy(rom_table[i],rom_table[0],1024);
	crc = crc32_buf(rom_table[0],1024);
	if (crc==0x8016A315) {
		printf("Odyssey2 bios ROM loaded\n");
		app_data.vpp = 0;
		app_data.bios = ROM_O2;
	} else if (crc==0xE20A9F41) {
		printf("Videopac+ G7400 bios ROM loaded\n");
		app_data.vpp = 1;
		app_data.bios = ROM_G7400;
	} else if (crc==0xA318E8D6) {
		if (!((!o2flag)&&(c52flag))) printf("C52 bios ROM loaded\n"); else printf("Ok\n");
		app_data.vpp = 0;
		app_data.bios = ROM_C52;
		
	} else if (crc==0x11647CA5) {
		if (g74flag) printf("Jopac bios ROM loaded\n"); else printf(" Ok\n");
		app_data.vpp = 1;
		app_data.bios = ROM_JOPAC;
	} else {
		printf("Bios ROM loaded (unknown version)\n");
		app_data.vpp = 0;
		app_data.bios = ROM_UNKNOWN;
	}
}
/****************************************************/                  
static void load_cart(char *file){
	FILE *fn;
	long l;
	int i, nb;

	app_data.crc = crc32_file(file);
	if (app_data.crc == 0xAFB23F89) app_data.exrom = 1;  /* Musician */
	if (app_data.crc == 0x3BFEF56B) app_data.exrom = 1;  /* Four in 1 Row! */
	if (app_data.crc == 0x9B5E9356) app_data.exrom = 1;  /* Four in 1 Row! (french) */

	if (((app_data.crc == 0x975AB8DA) || (app_data.crc == 0xE246A812)) && (!app_data.debug)) {
		fprintf(stderr,"Error: file %s is an incomplete ROM dump\n",file_v);
		exit(EXIT_FAILURE);
	}
	
    fn=fopen(file,"rb");
	if (!fn) {
		fprintf(stderr,"Error loading %s\n",file_v);
		exit(EXIT_FAILURE);
	}
	printf("Loading: \"%s\"  Size: ",file_v);
	l = filesize(fn);
	
    if ((l % 1024) != 0) {
		fprintf(stderr,"Error: file %s is an invalid ROM dump\n",file_v);
		exit(EXIT_FAILURE);
	}
		
  /* special MegaCART design by Soeren Gust */
	if ((l == 32768) || (l == 65536) || (l == 131072) || (l == 262144) || (l == 524288) || (l == 1048576)) {
		app_data.megaxrom = 1;
		app_data.bank = 1;
		megarom = malloc(1048576);
		if (megarom == NULL) {
			fprintf(stderr, "Out of memory loading %s\n", file);
			exit(EXIT_FAILURE);
			}
		if (fread(megarom, l, 1, fn) != 1) {
			fprintf(stderr,"Error loading %s\n",file);
			exit(EXIT_FAILURE);
		}
		
        /* mirror shorter files into full megabyte */
		if (l < 65536) memcpy(megarom+32768,megarom,32768);
		if (l < 131072) memcpy(megarom+65536,megarom,65536);
		if (l < 262144) memcpy(megarom+131072,megarom,131072);
		if (l < 524288) memcpy(megarom+262144,megarom,262144);
		if (l < 1048576) memcpy(megarom+524288,megarom,524288);
		/* start in bank 0xff */
		memcpy(&rom_table[0][1024], megarom + 4096*255 + 1024, 3072);
		printf("MegaCart %ldK", l / 1024);
		nb = 1;
	} else if (((l % 3072) == 0))
	  {
		app_data.three_k = 1;
		nb = l/3072;

		for (i=nb-1; i>=0; i--) {
			if (fread(&rom_table[i][1024],3072,1,fn) != 1) {
				fprintf(stderr,"Error loading %s\n",file);
				exit(EXIT_FAILURE);
			}
		}
		printf("%dK",nb*3);

	} else {

		nb = l/2048;

		if ((nb == 2) && (app_data.exrom)) {

			if (fread(&extROM[0], 1024,1,fn) != 1) {
				fprintf(stderr,"Error loading %s\n",file);
				exit(EXIT_FAILURE);
			}
			if (fread(&rom_table[0][1024],3072,1,fn) != 1) {
				fprintf(stderr,"Error loading %s\n",file);
				exit(EXIT_FAILURE);
			}
			printf("3K EXROM");

		} else {

			for (i=nb-1; i>=0; i--) {
				if (fread(&rom_table[i][1024],2048,1,fn) != 1) {
					fprintf(stderr,"Error loading %s\n",file);
					exit(EXIT_FAILURE);
				}
				memcpy(&rom_table[i][3072],&rom_table[i][2048],1024); /* simulate missing A10 */
			}
			printf("%dK",nb*2);

		}
	}
	fclose(fn);
	rom = rom_table[0];
	if (nb==1)
			app_data.bank = 1;
	else if (nb==2)
		app_data.bank = app_data.exrom ? 1 : 2;
	else if (nb==4)
		app_data.bank = 3;
	else 
		app_data.bank = 4;
	
    if ((rom_table[nb-1][1024+12]=='O') && (rom_table[nb-1][1024+13]=='P') && (rom_table[nb-1][1024+14]=='N') && (rom_table[nb-1][1024+15]=='B')) app_data.openb=1;
	
    printf("  CRC: %08lX\n",app_data.crc);
}
/*************************** Helpus*/
void helpus(void){
#ifndef __LIBRETRO__
allegro_init();
install_timer();
#endif
printf("here helpus\n");
init_display();
printf("here after init disp\n");
init_system();
#ifdef ALLEGRO_WINDOWS
FreeConsole();
#endif
help();
exit(EXIT_SUCCESS);
}
/*********************Open the directory `pathxŽ*/	
int file_name(char *pathx)
{
    DIR           *dir_p;
    struct dirent *dir_entry_p;
    contax=0;
    dir_p = opendir(pathx);

    if (dir_p == NULL) { fprintf(stderr,"dir '%s' not found !\n", pathx); exit(-1); }
while(0 != (dir_entry_p = readdir(dir_p)))
    {
#ifndef __LIBRETRO__
        strcpy(arkivo[contax], dir_entry_p->d_name); 
#else
	sprintf(arkivo[contax],"%s\0",dir_entry_p->d_name);
#endif
        contax++;                                    
    }
        closedir(dir_p);
        return(0);
}
/********************* Search BIOS */
int suck_bios()
{
    int i;
    for (i=0; i<contax; ++i)                         
        {
                 strcpy(biossux,biosdir);
                 strcat(biossux,arkivo[i]);
                 identify_bios(biossux);
        }
        return(0);
}
/********************* Search ROM */
int suck_roms()
{
    int i;
    rom_f = 1;
    for (i=0; i<contax; ++i)                    
        {
                 strcpy(romssux,romdir);
                 strcat(romssux,arkivo[i]);
                 app_data.crc = crc32_file(romssux);
                 if (app_data.crc == crcx)
                                  {
                                  if ((app_data.crc == 0xD7089068)||(app_data.crc == 0xB0A7D723)||
                                  (app_data.crc == 0x0CA26992)||(app_data.crc == 0x0B6EB25B)||
                                  (app_data.crc == 0x06861A9C)||(app_data.crc == 0xB2F0F0B4)||
                                  (app_data.crc == 0x68560DC7)||(app_data.crc == 0x0D2D721D)||
                                  (app_data.crc == 0xC4134DF8)||(app_data.crc == 0xA75C42F8))
                                  rom_f = 0;
                                  }
        }
return(0); 
}
/********************* Ready BIOS */
int identify_bios(char *biossux)
    {
    app_data.crc = crc32_file(biossux); 
                 if (app_data.crc == 0x8016A315){
                                                 strcpy(odyssey2, biossux);
                                                 o2flag = 1;
                                                }
                 if (app_data.crc == 0xE20A9F41){
                                                 strcpy(g7400, biossux);
                                                 g74flag = 1;
                                                }
                 if (app_data.crc == 0xA318E8D6){
                                                 strcpy(c52, biossux);
                                                 c52flag = 1;
                                                }
                 if (app_data.crc == 0x11647CA5){
                                                 strcpy(jopac, biossux);
                                                 jopflag = 1;
                                                }
    return(0);
    }
