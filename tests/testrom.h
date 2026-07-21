/* Shared synthetic test images for the in-tree tests.
 *
 * The BIOS is a hand-assembled 8048 program (no real BIOS or game
 * content involved):
 *   000: JMP 010                        ; reset vector
 *   007: MOV R0,#20; MOV @R0,A         ; timer ISR: RAM[20h] = ACC
 *        INC R7; RETR
 *   010: EN TCNTI; STRT T
 *        ANL P1,#B7                     ; enable VDC access
 *        MOV R0,#A9; MOV A,#5A; MOVX @R0,A  ; seed shift reg low byte
 *        MOV R0,#AA; MOV A,#9F; MOVX @R0,A  ; audio enable+noise+volume
 *   01E: INC A; JMP 01E                 ; free-running accumulator
 *
 * The ISR couples the accumulator, timer and interrupt state into
 * serialized RAM every interrupt; the sound writes keep the i8244
 * noise LFSR cycling so the audio path carries real signal.
 */
#ifndef O2EM_TESTROM_H
#define O2EM_TESTROM_H

#include <stdio.h>
#include <string.h>

#define TESTROM_CART_SIZE 2048

static void testrom_write_images(const char *dir)
{
   static const unsigned char isr[]   = { 0xB8, 0x20, 0xA0, 0x1F, 0x93 };
   static const unsigned char body[]  = {
      0x25, 0x55, 0x99, 0xB7,
      0xB8, 0xA9, 0x23, 0x5A, 0x90,
      0xB8, 0xAA, 0x23, 0x9F, 0x90,
      0x17, 0x04, 0x1E
   };
   unsigned char bios[1024];
   unsigned char cart[TESTROM_CART_SIZE];
   char path[512];
   FILE *f;

   memset(bios, 0, sizeof(bios));
   bios[0] = 0x04; bios[1] = 0x10;              /* JMP 010 */
   memcpy(bios + 0x007, isr, sizeof(isr));
   memcpy(bios + 0x010, body, sizeof(body));

   snprintf(path, sizeof(path), "%s/o2rom.bin", dir);
   f = fopen(path, "wb");
   fwrite(bios, 1, sizeof(bios), f);
   fclose(f);

   memset(cart, 0, sizeof(cart));
   snprintf(path, sizeof(path), "%s/cart.bin", dir);
   f = fopen(path, "wb");
   fwrite(cart, 1, sizeof(cart), f);
   fclose(f);
}

#endif /* O2EM_TESTROM_H */
