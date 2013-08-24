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
 *   O2EM built-in debugger
 */


#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "keyboard.h"
#include "vmachine.h"
#include "audio.h"
#include "vdc.h"
#include "table.h"
#include "debug.h"

#ifndef RETRO
#include "allegro.h"
#else
#include "wrapalleg.h"
#include <errno.h>
#endif 

#include "vpp.h"
#include "score.h"

#if defined(ALLEGRO_WINDOWS)
#include "winalleg.h"
#include <conio.h>
#define KBHIT _kbhit
#elif defined(ALLEGRO_DOS)
#include <conio.h>
#define KBHIT kbhit
#else
#define KBHIT() 0
#endif

static int assemble(char *opcode, int p);
static int disasm(ADDRESS p);
static int spriteprint(ADDRESS p);
static void show_reg(void);
int 	nMemDump;
int 	nDisAsm;
int	nSprite;
void decimal2binary(int decimal_value, char binary_value[32], short digits);


void debug(void) {
	int done,i,go,j;
	char inp[80];
	char *tok;
	ADDRESS ad,run_to;
	char cr[2],ff[2];
	Byte d,a;

	dbstick1=dbstick2=0;
	cr[0]=13;
	cr[1]=0;
	ff[0]=0x0c;
	ff[1]=0;

	done=go=0;

#ifndef RETRO
	set_display_switch_mode(SWITCH_BACKGROUND);
#endif
	if (sndlog) fclose(sndlog);

	#ifdef ALLEGRO_WINDOWS
	FreeConsole();
	AllocConsole();
	#endif

	printf("\nDebug mode\n\n");
	
    if (pc==app_data.breakpoint)
	{
		printf("Breakpoint reached at %x\n",pc);
	}

	nMemDump=pc;
	nDisAsm=pc;
	nSprite=0;
	show_reg();

	while (!done) {
		while (go > 0) {
			disasm(pc);
			cpu_exec();
			go--;
			if (KBHIT()) {
				go=0;
				cpu_exec();
                show_reg();
			}
		}
		printf("> ");
		fgets(inp,76,stdin);
		if ((strlen(inp)>0) && (inp[strlen(inp)-1]=='\n')) inp[strlen(inp)-1]=0;
		strlwr(inp);
		strcat(inp," \n");
		tok=strtok(inp," ");
		if (!tok) tok="";
		if (!strcmp(tok,"h") || !strcmp(tok,"?") || !strcmp(tok,"help")) {
			printf("display - show display\n");
			printf("viewsprite # - show memory # as sprite\n");
//			printf("editsprite # - edit memory # as sprite\n");
			printf("showfps - display the number of frames per second\n");

            #ifdef ALLEGRO_DOS
			printf("cls - clear screen\n");
			#endif
			
			printf("clrkey - clear keypresses\n");
			printf("setkey n - set key n pressed\n");
			printf("stick1 # - set stick1 to #\n");
			printf("stick2 # - set stick2 to #\n");
			
			printf("reg - display registers\n");
			printf("stack - display stack\n");
			printf("col - display collision table\n");
			printf("video (a d) - display VDC registers (or write data d to a)\n");
			printf("extram (a d) - display external RAM (or write data d to a)\n");
			printf("intram (a d) - display internal ram (or write data d to a)\n");
			
            printf("s - step\n");
			printf("so - step over\n");
			printf("go # - execute # instructions\n");
			printf("run # - run to address #\n");
			printf("runvb - run to VBL\n");
			printf("runnvb - run to no VBL\n");
			printf("bp # - breakpoint at address #\n");
			printf("bpc - clear breakpoint\n");
			printf("reset - reset the machine\n");
			
			printf("load file a - load file to memory to address\n");
			printf("save file a n - save memory to file from address n Bytes\n");
			printf("savedatah file a n - save memory to file from address a n Bytes format hex\n");
			printf("savedatab file a n - save memory to file from address a n Bytes format binary\n");
			printf("savevdc file a n - save VDC to file from address a n Bytes format hex\n");
			printf("saveext file a n - save ExtRam to file from address a n Bytes format hex\n");
			printf("savevpp file - save the VPP to file\n");
			
			printf("as # - assemble memory #\n");
			printf("di # - disassemble memory #\n");
			printf("md # - display memory #\n");
			printf("c # - change memory from location #\n");
			
			printf("sndlog [file] - log raw sound to file\n");
			printf("ss [file] - save state [to file]\n");
			printf("ls [file] - load state [from file]\n");
			
			printf("q - quit debugger and return to emulation\n");
			printf("\n");
		} else if (!strcmp(tok,"display")) {
			grmode();
			clip_low = 0;
			clip_high = 85000;
			draw_display();
			finish_display();
			init_keyboard();
			do {
				rest(1);
			} while((key_done==0) && (!keypressed()));
			set_textmode();
#ifndef RETRO
			set_display_switch_mode(SWITCH_BACKGROUND);
#endif
		} else if (!strcmp(tok,"viewsprite")) {
			if (tok){
				tok=strtok(NULL," ");
				ad=65535;
				sscanf(tok,"%hx",&ad);
				if (ad==65535) ad=nSprite;

				for(i=0; i<8; i++)
				{
					if (ad<32768) ad = ad + spriteprint(ad); else ad=0;
				}
				nSprite=ad;
				printf("\n");
			}
//TODO !!!! ----------------------------------------------------------------------------------------------------
		} else if (!strcmp(tok,"editsprite")) {
			if (tok){
				tok=strtok(NULL," ");
				ad=65535;
				sscanf(tok,"%hx",&ad);
				if (ad!=65535)
				{
					for(i=0; i<8; i++)
					{
						if (ad<32768) ad = ad + spriteprint(ad); else ad=0;
					}
					printf("\n");
				}
			}
//----------------------------------------------------------------------------------------------------
		} else if (!strcmp(tok,"cls")) {
			#ifdef ALLEGRO_DOS
			clrscr();
			#endif
		} else if (!strcmp(tok,"clrkey")) {
			for(i=0; i<128; i++) key2[i]=0;
		} else if (!strcmp(tok,"setkey")) {
			tok=strtok(NULL," ");
			if (tok){
				sscanf(tok,"%d",&i);
				if ((i>0) && (i<128)){
					printf("Key %d set to pressed\n",i);
					key2[i]=1;
				}
				key2vcnt=0;
			}
		} else if (!strcmp(tok,"stick1")) {
			tok=strtok(NULL," ");
			if (tok) {
				i=0;
				sscanf(tok,"%x",&i);
				printf("Stick 1 set to %d\n",i);
				dbstick1=i;
				key2vcnt=0;
			}
		} else if (!strcmp(tok,"stick2")) {
			tok=strtok(NULL," ");
			if (tok) {
				i=0;
				sscanf(tok,"%x",&i);
				printf("Stick 2 set to %d\n",i);
				dbstick2=i;
				key2vcnt=0;
			}
		} else if (!strcmp(tok,"s")) {
			cpu_exec();
			show_reg();
		} else if (!strcmp(tok,"so")) {
			run_to = pc+lookup[rom[pc]].bytes;
			printf("%hx\n",run_to);
			while (pc != run_to) {
				cpu_exec();
				if (KBHIT()) break;
			}
			show_reg();
		} else if (!strcmp(tok,"reset")) {
			init_cpu();
			init_roms();
			init_vpp();
		} else if (!strcmp(tok,"reg")) {
			show_reg();
		} else if (!strcmp(tok,"stack")) {
			for(i=8; i<24; i+=2) {
				ad = intRAM[i];
				d = intRAM[i+1];
				ad = ad | ((d & 0x0F) << 8);
				printf("%x: PC=%x PSW=%x",i,ad,d);
				if (i == sp) printf(" <<SP");
				printf("\n");
			}
			printf("\n");
		} else if (!strcmp(tok,"q")) {
			printf("\nReturning to emulation\n\n");
			done=1;			
		} else if (!strcmp(tok,"pvideo")) {
			j=0;
			fprintf(stderr,"%02x:",0);
			for(i=0; i<256; i++) {
				fprintf(stderr,"%02x,",VDCwrite[i]);
				j++;
				if (j == 16) {
					fprintf(stderr,"%s\n",cr);
					fprintf(stderr,"%02x:",i+1);
					j=0;
				}
			}
			fprintf(stderr,"%s\n",cr);
			fprintf(stderr,"%s",ff);
		} else if (!strcmp(tok,"video")) 
            {
			int t;
			tok=strtok(NULL," ");
			if (tok){
				sscanf(tok,"%x",&t);
				a = (Byte)t;
				tok=strtok(NULL," ");
				if (tok){
					sscanf(tok,"%x",&t);
					d = (Byte)t;
					VDCwrite[a] = d;
				}
			else
 	        {
			int j=0;
			printf("%02x:",0);
			for(i=0; i<256; i++) 
            {
				printf("%02x",VDCwrite[i]);
				if (j < 15) printf(",");
				j++;
				if (j == 16 && i != 255) 
                {
					printf("\n");
					printf("%02x:",i+1);
					j=0;
				}
			}
        }
    }
			printf("\n");
		} else if (!strcmp(tok,"save"))
			{
			int t;
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						tok=strtok(NULL," ");
						if (tok)
						{
							sscanf(tok,"%i",&t);
							fn = fopen(file,"wb");
							if (fn==NULL) {
								fprintf(stderr,"Error opening save-file %s: %i\n",file,errno);
							}
							else
							{
								fwrite ((rom+adr),t,1,fn);
								fclose(fn);
								printf("File %s saved: %i bytes from address:%x\n",file,t,adr);
							}
						}
					}

				}
			}
		} else if (!strcmp(tok,"savedatah"))
			{
			int t;
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						tok=strtok(NULL," ");
						if (tok)
						{
							sscanf(tok,"%i",&t);
							fn = fopen(file,"wt");
							if (fn==NULL) {
								fprintf(stderr,"Error opening save-data file %s: %i\n",file,errno);
							}
							else
							{
								// write format "db 0XXh[,0XXh]" 8 Zeichen pro Zeile
								int j=0;
								for (i=0; i<((int) ((t-1)/8))+1; i++)
								{
									fprintf(fn,"db	");
									for (j=0;j<8 && i*8+j<t;j++)
									{
										fprintf(fn,"0%02Xh",rom[adr+i*8+j]);
										if (j<7 && i*8+j+1<t) fprintf(fn,",");
									}
									fprintf(fn,"\n");
								}

								fclose(fn);
								printf("File %s saved: %i bytes from address:%x\n",file,t,adr);
							}
						}
					}

				}
			}
		} else if (!strcmp(tok,"savevdc"))
			{
			int t;
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						tok=strtok(NULL," ");
						if (tok)
						{
							sscanf(tok,"%i",&t);
							fn = fopen(file,"wt");
							if (fn==NULL) {
								fprintf(stderr,"Error opening save-data file %s: %i\n",file,errno);
							}
							else
							{
								// write format "db 0XXh[,0XXh]" 8 Zeichen pro Zeile
								int j=0;
								for (i=0; i<((int) ((t-1)/8))+1; i++)
								{
									fprintf(fn,"db	");
									for (j=0;j<8 && i*8+j<t;j++)
									{
										fprintf(fn,"0%02Xh",VDCwrite[adr+i*8+j]);
										if (j<7 && i*8+j+1<t) fprintf(fn,",");
									}
									fprintf(fn,"\n");
								}

								fclose(fn);
								printf("File %s saved: %i bytes from address:%x\n",file,t,adr);
							}
						}
					}

				}
			}
		} else if (!strcmp(tok,"saveext"))
			{
			int t;
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						tok=strtok(NULL," ");
						if (tok)
						{
							sscanf(tok,"%i",&t);
							fn = fopen(file,"wt");
							if (fn==NULL) {
								fprintf(stderr,"Error opening save-data file %s: %i\n",file,errno);
							}
							else
							{
								// write format "db 0XXh[,0XXh]" 8 Zeichen pro Zeile
								int j=0;
								for (i=0; i<((int) ((t-1)/8))+1; i++)
								{
									fprintf(fn,"db	");
									for (j=0;j<8 && i*8+j<t;j++)
									{
										fprintf(fn,"0%02Xh",extRAM[adr+i*8+j]);
										if (j<7 && i*8+j+1<t) fprintf(fn,",");
									}
									fprintf(fn,"\n");
								}

								fclose(fn);
								printf("File %s saved: %i bytes from address:%x\n",file,t,adr);
							}
						}
					}

				}
			}
		} else if (!strcmp(tok,"savedatab"))
			{
			int t;
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						tok=strtok(NULL," ");
						if (tok)
						{
							sscanf(tok,"%i",&t);
							fn = fopen(file,"wt");
							if (fn==NULL) {
								fprintf(stderr,"Error opening save-data file %s: %i\n",file,errno);
							}
							else
							{
								char binary_value[32];
								// write format "db xxxxxxxxb" 1 Zeichen pro Zeile
								for (i=0; i<t; i++)
								{
									decimal2binary(rom[adr+i],binary_value,8);
									fprintf(fn,"db	%sb\n",binary_value);
								}

								fclose(fn);
								printf("File %s saved: %i bytes from address:%x\n",file,t,adr);
							}
						}
					}

				}
			}
		} else if (!strcmp(tok,"savevpp")) {
			extern Byte vpp_mem[40][32][4];
			extern Byte dchars[2][960];
			Byte mchars[2][960]; /* bit-mirrored dchars */
			FILE *fn;
			char file[80]="";
			int i, j;

			tok=strtok(NULL," ");
			if (tok) {
				sscanf(tok,"%79s",file);
				if (strlen(file)>0) {
					fn = fopen(file,"wt");
					if (fn==NULL) {
						fprintf(stderr,"Error opening vpp data file %s: %i\n",file,errno);
					} else {
						/* write char/attribute data */
						for(i = 0; i < 25; i++) {
						/* service row is 24 in file, 31 in memory */
							if (i == 24) i = 31;
							for(j = 0; j < 40; j++) {
							if (fwrite(&vpp_mem[j][i][0], 1, 1, fn) != 1) {
							fprintf(stderr,"Error writing vpp data file %s: %i\n",file,errno);
							break;
							}
			        	if (fwrite(&vpp_mem[j][i][1], 1, 1, fn) != 1) {
        					fprintf(stderr,"Error writing vpp data file %s: %i\n",file,errno);
         					break;
        				}
      				}
      				/* mirror slice bits */
					for(i = 0; i < 2; i++) for(j = 0; j < 960; j++) {
					mchars[i][j] = ((dchars[i][j] & 0x80) >> 7) | ((dchars[i][j] & 0x40) >> 5) | ((dchars[i][j] & 0x20) >> 3) | ((dchars[i][j] & 0x10) >> 1) | ((dchars[i][j] & 0x08) << 1) | ((dchars[i][j] & 0x04) << 3) | ((dchars[i][j] & 0x02) << 5) | ((dchars[i][j] & 0x01) << 7);
					}
						/* write slice data */
						if (fwrite(mchars, 960, 2, fn) != 2) {
							fprintf(stderr,"Error Writing vpp data file %s: %i\n",file,errno);
							break;
						}
					      	fclose(fn);
					     }

    					}
				}
   			}
		} else if (!strcmp(tok,"load"))
			{
			int adr;
			FILE *fn;
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0)
				{
					tok=strtok(NULL," ");
					if (tok)
					{
						sscanf(tok,"%x",&adr);
						fn = fopen(file,"rb");
						if (fn==NULL) {
							fprintf(stderr,"Error opening load-file %s: %i\n",file,errno);
						}
						else
						{
							int length;
							fseek(fn,0L,SEEK_END);
							length = ftell(fn);
							rewind(fn);
							fread ((rom+adr),length,1,fn);
							fclose(fn);
							printf("File %s loaded to address:%x (%i bytes)\n",file,adr,length);
						}
					}

				}
			}
		} else if (!strcmp(tok,"keys")) {
			j=0;
			printf("%02x:",0);
			for(i=0; i<128; i++) {
				printf("%02x,",key2[i]);
				j++;
				if (j == 16) {
					printf("\n");
					printf("%02x:",i+1);

					j=0;
				}
			}
			printf("\n");
		} else if (!strcmp(tok,"col")) {
			j=0;
			printf("%02x:",0);
			for(i=0; i<256; i++) {
				printf("%02x,",coltab[i]);
				j++;
				if (j == 16) {
					printf("\n");
					printf("%02x:",i+1);
					j=0;
				}
			}
			printf("\n");
			printf("Colreg: %x\n ",ext_read(0xA2));
		} else if (!strcmp(tok,"extram")) {
			int t;
			tok=strtok(NULL," ");
			if (tok){
				sscanf(tok,"%x",&t);
				a = (Byte)t;
				tok=strtok(NULL," ");
				if (tok){
					sscanf(tok,"%x",&t);
					d = (Byte)t;
					extRAM[a] = d;
            }
        else 
        {
			j=0;
			printf("%02x:",0);
			for(i=0; i<256; i++) {
				printf("%02x",extRAM[i]);
				if (j < 15) printf(",");
				j++;
				if (j == 16 && i !=255) {
					printf("\n");
					printf("%02x:",i+1);

					j=0;
				}
			}
}
}
			printf("\n\n");
		} else if (!strcmp(tok,"intram")) {
			int t;
			tok=strtok(NULL," ");
			if (tok){
				sscanf(tok,"%x",&t);
				a = (Byte)t;
				tok=strtok(NULL," ");
				if (tok)
				{
					sscanf(tok,"%x",&t);
					d = (Byte)t;
					intRAM[a] = d;
				}
			else
			{
				j=0;
				printf("%02x:",0);
				for(i=0; i<64; i++) {
					printf("%02x",intRAM[i]);
					if (j < 15) printf(",");
					j++;
					if (j == 16 && i != 63) {
						printf("\n");
						printf("%02x:",i+1);
						j=0;
					}
				}
				printf("\n\n");
			}
        }
		} else if (!strcmp(tok,"run")) {
			tok=strtok(NULL," ");
			if (tok){
				sscanf(tok,"%hx",&run_to);
				printf("Run to %x\n",run_to);
				while (pc != run_to) {
					cpu_exec();
					if (KBHIT()) break;
				}
				show_reg();
			}
		} else if (!strcmp(tok,"bp")) {
			int bp;
			tok=strtok(NULL," ");
			bp=65535;
			sscanf(tok,"%x",&bp);
			if (bp==65535)
			{
				if (app_data.breakpoint==65535)
				printf("No Breakpoint set.\n");
				else printf("Breakpoint at %x\n",app_data.breakpoint);
			}
			else
			{
				sscanf(tok,"%x",&bp);
				app_data.breakpoint = bp;
				printf("Breakpoint set to %x\n",app_data.breakpoint);
			}
		} else if (!strcmp(tok,"bpc")) {
				if (app_data.breakpoint==65535)
				printf("No Breakpoint set.\n");
				else
				{
					app_data.breakpoint = 65535;
					printf("Breakpoint cleared.\n");
				}
        } else if (!strcmp(tok,"runvb")) {
			printf("Run to VBL\n");
			while (master_clk < 5500) {
				cpu_exec();
				if (KBHIT()) break;
			}
			show_reg();
		} else if (!strcmp(tok,"runnvb")) {
			printf("Run to no VBL\n");
			while (master_clk > 5500) {
				cpu_exec();
				if (KBHIT()) break;
			}
			show_reg();
		} else if (!strcmp(tok,"go")) {
			tok=strtok(NULL," ");
			if (tok) sscanf(tok,"%d",&go);
		} else if (!strcmp(tok,"di")) {
			if (tok){
				tok=strtok(NULL," ");
				ad=65535;
				sscanf(tok,"%hx",&ad);
				if (ad==65535) ad=nDisAsm;

				for(i=0; i<20; i++)
				{
					if (ad<32768) ad = ad + disasm(ad); else ad=0;
				}
				nDisAsm=ad;
				printf("\n");
			}
		} else if (!strcmp(tok,"md")) {
			tok=strtok(NULL," ");
			if (tok){
				ad=65535;
				sscanf(tok,"%hx",&ad);
				if (ad==65535) ad=nMemDump;
				printf("\n");

				for (j=0; j<10; j++){
					if (ad+j*16<32768) {
						printf("%04x : ",ad+j*16);
						for (i=0; i<16; i++) printf("%02x ",rom[ad+j*16+i]);
						printf("\n");
					}
					else
					{
						ad = 0;
					}
				}
				nMemDump = ad+10*16;
				printf("\n\n");
			}
		} else if (!strcmp(tok,"sndlog")) {
			if (app_data.sound_en) {
				tok=strtok(NULL," ");
				if (tok){
					char file[80]="";
					sscanf(tok,"%79s",file);
					if (strlen(file)>0){
						sndlog = fopen(file,"wb");
						if (sndlog)
							printf("Sound log file created\n");
						else
							printf("Sound log file creation failed\n");
					}
				}
				printf("\n");
			} else printf("Sound emulation disabled\n\n");

		} else if (!strcmp(tok,"ss")) {
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0){
					savestate(file);
				}
			}
			printf("\n");

		} else if (!strcmp(tok,"ls")) {
			tok=strtok(NULL," ");
			if (tok){
				char file[80]="";
				sscanf(tok,"%79s",file);
				if (strlen(file)>0){
					loadstate(file);
				}
			}
			printf("\n");

		} else if (!strcmp(tok,"showfps")) {
			show_fps=1;

		} else if (!strcmp(tok,"c"))
		{
			int t;
			tok=strtok(NULL," ");
			if (tok)
			{
				sscanf(tok,"%hx",&ad);
				printf("Addr   Old  New (enter to exit)\n");   // Header

				do
				{
					printf("%.4X   %.2X   ?",ad,rom[ad]);   // Memory Position, Old Value

					fgets(inp,80,stdin);
					if (inp[0]!=10)
					{
						t=65535;
						sscanf(inp,"%x",&t);
						if (t<256)
						{
							d = (Byte) t;
							rom[ad]=d;
							ad=(ad==32768)?0:ad+1;
						}
						else
						{
							printf("wrong value, try again\n");
						}
					}
				}
				while (inp[0]!=10);
			}
			printf("\n");

		} else if (!strcmp(tok,"as")) {
			tok=strtok(NULL," ");
			if (tok)
			{
				sscanf(tok,"%hx",&ad);

			        printf("Addr Instruction (enter to exit)\n");   // Header
		         	do
         			{
               				printf("%.4X  ",ad);   // Memory Position
					fgets(inp,80,stdin);

					if (inp[0]!=10)
					{
						ad = ad + assemble(inp,ad);
					}
				}
				while (inp[0]!=10);
			}
			printf("\n");


		} else if ((strlen(tok)>0) && (strcmp(tok,"\n"))) {
			printf("Unknown command\n\n");
		} 
	
	}
	
	#ifdef ALLEGRO_WINDOWS
	FreeConsole();
	#endif

}


static void show_reg(void) {
	int i;

		make_psw_debug();
		printf("PC=%.3X  A=%.2X  PSW=%.2X  P1=%.2X  P2=%.2X",pc,acc,psw,p1,p2);
		printf("  clk: %d  reg:%d  count:%x  f:%x\n",master_clk,reg_pnt,itimer,frame);
		disasm(pc);
		printf("\n");

		printf("R%d: %02x   R%d': %02x   PSW: ",0,intRAM[0],0,intRAM[24]);
		printf("%sC %sAC %sF0 %sF1 SP:%.2X\n",psw&0x80?"":"!",psw&0x40?"":"!",psw&0x20?"":"!",psw&0x10?"":"!",psw&0x07);

		printf("R%d: %02x   R%d': %02x   P1:  ",1,intRAM[1],1,intRAM[25]);
		printf("%sLum %sCopy %sVP+ %sExtRam %sVDC %sKeyB Bank:%i\n",p1&0x80?"":"!",p1&0x40?"":"!",p1&0x20?"":"!",p1&0x10?"":"!",p1&0x08?"":"!",p1&0x04?"":"!",p1&0x03);

		printf("R%d: %02x   R%d': %02x   ",2,intRAM[2],2,intRAM[26]);
		printf("MB%i RB%i\n",A11ff>>11,reg_pnt>4);

		for (i=3; i<8; i++) {
			printf("R%d: %02x   R%d': %02x\n",i,intRAM[i],i,intRAM[i+24]);
		}
}


static int disasm(ADDRESS p) {
	Byte op,d;
	ADDRESS adr;

	op=rom[p++];
	printf("%04x  %02x",p-1,op);
	if (lookup[op].bytes == 2) printf(" %02x",rom[p]); else printf("   ");
	printf("   %s",lookup[op].mnemonic);
	switch(lookup[op].type) {
		case 0:
			printf("\n");
			break;
		case 1:
			d=rom[p];
			printf(" #%02x\n",d);
			break;
		case 2:
			adr=rom[p];
			adr = adr | ((op & 0xE0) << 3);
			printf(" $%03x\n",adr);
			break;
		case 3:
			printf(" $%02x\n",rom[p]);
			break;
	}
	return lookup[op].bytes;
}

static int assemble(char *opcode, int p)
{
	char *str_value;
	int op=0;
	int value;
	int t;

        while((op<256)&&(strncasecmp(opcode,lookup[op].mnemonic,strlen(lookup[op].mnemonic))!=0)) op++;

	if (op==256)
	{
		printf("Wrong opcode, please try again.\n");
		return(0);
	}

	str_value = opcode+strlen(lookup[op].mnemonic);
	while (str_value[0]==' ') str_value++;

	switch(lookup[op].type)
	{
		case 0:			// no other value needed
			rom[p]=op;
			break;
		case 1:			// #value needed
			if (str_value[0]!='#')
			{
				printf("Error: #xx expected\n");
				return(0);
			}
			t=65335;
			sscanf(str_value+1,"%2x",&t);
			if (t>=256)
			{
				printf("Error: #xx expected (value #00-#FF)\n");
				return(0);
			}
			else value = (Byte) t;
			rom[p]=op;
			rom[p+1]=value;
			break;
		case 2:			// $addresss needed (max. 7FF)
			if (str_value[0]!='$')
			{
				printf("Error: $xxx expected %s\n",str_value);
				return(0);
			}
			t=65335;
			sscanf(str_value+1,"%x",&t);
			if (t>=2048)
			{
				printf("Error: $xxx expected (value below $800)\n");
				return(0);
			}
			else value = (Byte) t;
			rom[p]=op | ((t&1792)>>3);
			rom[p+1]=value;
			break;

		case 3:			// $address needed (max. FF)
			if (str_value[0]!='$')
			{
				printf("Error: $xx expected\n");
				return(0);
			}
			t=65335;
			sscanf(str_value+1,"%x",&t);
			if (t>=256)
			{
				printf("Error: $xx expected (value $00-$FF)\n");
				return(0);
			}
			else value = (Byte) t;
			rom[p]=op;
			rom[p+1]=value;
			break;

	}
	return lookup[op].bytes;
}

void decimal2binary(int decimal_value, char binary_value[32], short digits)
{
	short i;
	for(i=digits;i>=0;i--)
    	{
		binary_value[i]=(char)('0'+(int) (decimal_value/(1<<i)));
		decimal_value=decimal_value%(1<<i);
	}
	binary_value[digits]=0;
}

static int spriteprint(ADDRESS p)
{
	char binary_value[32];
	int i;

	decimal2binary(rom[p],binary_value,8);
	printf("%04x  %sb  ",p, binary_value);

	for (i=7;i>=0;i--)
	{
		if (binary_value[i]=='0') printf(" ");
			else printf("%c",178);
	}

	printf("\n");
	return 1;
}
