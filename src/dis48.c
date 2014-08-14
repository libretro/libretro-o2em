
/*
 *   O2/8048 disassembler
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "table.h"
#include "debug.h"

#define BIOSLEN 1024
#define MAXLEN 8192


Byte rom[MAXLEN+BIOSLEN];
ADDRESS pc;

int parse_option(char *attr, char *val);

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
	if (op == 0x83 || op == 0x93 || (((p-2+lookup[op].bytes) & 0xff) == 0xff)) printf("\n"); // Linebreak after ret, retr and end of page
	return lookup[op].bytes;
}


int main(int argc, char **argv){
	int n;
	int i,j;
	int start;
	long len;
	FILE *fn;
	static char file[MAXC], attr[MAXC], val[MAXC], *p;
	pc = BIOSLEN;
    if (argc<2) {
		printf("Usage: dis48 [file] [-bios]\n");
		exit(0);
	}

	/* load image */

	for(i=1; i<argc; i++)
	{
		if (argv[i][0] != '-')
		{
			strncat(file,argv[i],MAXC-1);
			file[MAXC-1]=0;
		}
		else
		{
			p=strtok(argv[i],"=");
			if (p)
			{
				strncpy(attr,p+1,MAXC-1);
				attr[MAXC-1]=0;
			} else 	strcpy(attr,"");


			p=strtok(NULL,"=");
			if (p){
				strncpy(val,p,MAXC-1);
				val[MAXC-1]=0;
			} else
				strcpy(val,"");
			for(j = 0; j < strlen(attr); j++)
				if(isupper(attr[j]))
				attr[j] = tolower(attr[j]);
 
			if (!parse_option(attr, val)) exit(EXIT_FAILURE);
		}
        }


	if (strlen(file)==0) {
		fprintf(stderr,"Error: file name missing\n");
		exit(EXIT_FAILURE);
	}
	
    fn=fopen(file,"rb");
	if (!fn) {
		printf("Error loading %s\n",argv[1]);
		exit(0);
	}
	fseek(fn, 0L, SEEK_END);
	len=ftell(fn);
	
	if (len>MAXLEN) {
		printf("Invalid image size");
		fclose(fn);
		exit(0);
	}
	
	rewind(fn);
	if (fread(&rom[pc],1,len,fn) != (size_t)len) {
		printf("Error loading %s\n",argv[1]);
		fclose(fn);
		exit(0);
	}
	fclose(fn);

	start = pc;
	while (pc < start+len) {
		n = disasm(pc);
		pc = pc + n;
	}
	exit(0);
}

int parse_option(char *attr, char *val){

	if (!strcmp(attr,"bios"))
	{
		pc = 0;
	}
	else
	{
		fprintf(stderr,"Invalid option : %s\n",attr);
		return 0;
	}
	return 1;
}



