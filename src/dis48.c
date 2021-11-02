
/*
 *   O2/8048 disassembler
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "table.h"

#define BIOSLEN 1024
#define MAXLEN 8192

Byte rom[MAXLEN+BIOSLEN];
ADDRESS pc;

int parse_option(char *attr, char *val)
{
   if (!strcmp(attr,"bios"))
   {
      pc = 0;
      return 1;
   }
   return 0;
}



