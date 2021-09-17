
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



