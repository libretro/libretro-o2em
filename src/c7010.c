/* SPDX-License-Identifier: Artistic-2.0
 * Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG.
 *
 * Philips Videopac C7010 Chess Module: NSC800 instruction interpreter,
 * external memory and communication latches, and portable save states.
 * Developed as part of O2EM-NG and its libretro research implementation.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c7010.h"
#include <compat/msvc.h>

#define C7010_CHESS_CART_CRC 0x77066338UL
#define C7010_FIRMWARE_SIZE  0x2000u
#define C7010_RAM_SIZE       0x0800u

typedef struct
{
    unsigned short pc;
    unsigned short sp;
    unsigned short hl;
    unsigned short ix;
    unsigned short iy;
    uint8_t a, b, c, d, e, h, l;
    uint8_t f;
    uint8_t a2, f2, b2, c2, d2, e2, h2, l2;
    uint8_t i, r;
    uint8_t interrupt_mode;
    bool iff1;
    bool iff2;
    bool initialized;
    bool stopped;
} C7010Nsc800;

typedef struct
{
    bool enabled;
    bool firmware_loaded;
    bool cpu_reset;
    bool execution_needs_reset;

    uint32_t cartridge_crc;

    unsigned long interleave_credit;

    uint8_t p1;
    uint8_t to_nsc800;
    uint8_t to_8048;

    unsigned long to_nsc_write_generation;
    unsigned long to_nsc_read_generation;
    unsigned long to_8048_write_generation;
    unsigned long to_8048_read_generation;

    uint8_t firmware[C7010_FIRMWARE_SIZE];
    uint8_t ram[C7010_RAM_SIZE];

    C7010Nsc800 nsc;
} C7010State;

static C7010State gC7010;

static C7010_LogFn gLogFn = NULL;

static void C7010_Log(const char *fmt, ...)
{
    char buf[2048];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (gLogFn)
        gLogFn(buf);
    else
        printf("%s", buf);
}

static bool InterfaceSelected(void)
{
    /* C7010 uses P10 and P14 low to select the module interface. */
    return (gC7010.p1 & 0x11) == 0;
}

void C7010_SetLogCallback(C7010_LogFn fn)
{
    gLogFn = fn;
}

bool C7010_IsEnabled(void)
{
    return gC7010.enabled;
}

bool C7010_IsActive(void)
{
    return gC7010.enabled;
}

bool C7010_ConfigureForCartridge(uint32_t cartridgeCrc)
{
    gC7010.cartridge_crc = cartridgeCrc;
    gC7010.enabled = cartridgeCrc == C7010_CHESS_CART_CRC;

    if (gC7010.enabled)
    {
        C7010_Log("C7010: cartridge detected (CRC 0x%08lX), module enabled\n",
            (unsigned long)cartridgeCrc);
    }

    return gC7010.enabled;
}

bool C7010_LoadFirmware(const char *path)
{
    FILE *file;
    size_t count;

    gC7010.firmware_loaded = false;

    if (!gC7010.enabled || !path)
        return false;

    file = fopen(path, "rb");
    if (!file)
    {
        C7010_Log("C7010: firmware not found: %s\n", path);
        return false;
    }

    count = fread(gC7010.firmware, 1, C7010_FIRMWARE_SIZE, file);
    fclose(file);

    if (count != C7010_FIRMWARE_SIZE)
    {
        C7010_Log("C7010: firmware has wrong size (%lu, expected 8192)\n",
            (unsigned long)count);
        return false;
    }

    gC7010.firmware_loaded = true;
    C7010_Log("C7010: firmware loaded: %s (8192 bytes)\n", path);
    return true;
}

bool C7010_LoadFirmwareData(const uint8_t *data, size_t size)
{
    gC7010.firmware_loaded = false;

    if (!gC7010.enabled || !data || size != C7010_FIRMWARE_SIZE)
    {
        C7010_Log("C7010: firmware data invalid (size %lu, expected 8192)\n",
            (unsigned long)size);
        return false;
    }

    memcpy(gC7010.firmware, data, size);
    gC7010.firmware_loaded = true;
    C7010_Log("C7010: firmware loaded from system directory (8192 bytes)\n");
    return true;
}

void C7010_Unload(void)
{
    gC7010.enabled = false;
    gC7010.firmware_loaded = false;
    gC7010.cpu_reset = true;
    gC7010.execution_needs_reset = true;
    gC7010.interleave_credit = 0;
    gC7010.p1 = 0xFF;
    gC7010.to_nsc800 = 0;
    gC7010.to_8048 = 0;
    C7010_Log("C7010: unloaded\n");
}

void C7010_Reset(void)
{
    if (!gC7010.enabled)
        return;

    memset(gC7010.ram, 0, sizeof(gC7010.ram));
    gC7010.interleave_credit = 0;
    gC7010.to_nsc800 = 0;
    gC7010.to_8048 = 0;
    gC7010.p1 = 0xFF;
    gC7010.cpu_reset = true;
    gC7010.execution_needs_reset = true;
    gC7010.to_nsc_write_generation = 0;
    gC7010.to_nsc_read_generation = ~0UL;
    gC7010.to_8048_write_generation = 0;
    gC7010.to_8048_read_generation = ~0UL;

    memset(&gC7010.nsc, 0, sizeof(gC7010.nsc));
    gC7010.nsc.sp = 0xFFFF;

    C7010_Log("C7010: reset (firmware=%s)\n",
        gC7010.firmware_loaded ? "ready" : "missing");
}

void C7010_WriteP1(uint8_t value)
{
    bool oldReset;

    if (!gC7010.enabled)
        return;

    oldReset = gC7010.cpu_reset;
    gC7010.p1 = value;

    /*
     * C7010 P11 controls the external processor reset.
     * Low = reset asserted, high = processor released.
     */
    gC7010.cpu_reset = (value & 0x02) == 0;

    if (oldReset != gC7010.cpu_reset)
    {
        C7010_Log("C7010: NSC800 RESET %s\n",
            gC7010.cpu_reset ? "asserted" : "released");
        if (gC7010.cpu_reset)
            gC7010.execution_needs_reset = true;
    }
}

bool C7010_ExternalRead(uint8_t address, uint8_t *value)
{
    if (!gC7010.enabled || !InterfaceSelected() || !value)
        return false;

    /*
     * C7010 only drives the 8048 external data bus for the documented
     * A7/A5-selected read window. Do not swallow unrelated VDC/VPP reads.
     */
    if ((address & 0xA0) != 0xA0)
        return false;

    *value = gC7010.to_8048;
    if (gC7010.to_8048_read_generation != gC7010.to_8048_write_generation)
        gC7010.to_8048_read_generation = gC7010.to_8048_write_generation;
    return true;
}

bool C7010_ExternalWrite(uint8_t address, uint8_t value)
{
    /* 0030R: P16 high blocks cartridge writes, not VDC writes. */
    if (!gC7010.enabled || !InterfaceSelected() || (gC7010.p1 & 0x40))
        return false;

    /*
     * Hardware uses the cartridge address bus as part of the latch select.
     * Only accept the documented high-half write.
     */
    if ((address & 0x80) == 0)
        return false;

    gC7010.to_nsc800 = value;
    ++gC7010.to_nsc_write_generation;
    return true;
}

uint8_t C7010_NSC800_ReadMemory(unsigned short address)
{
    if (address < 0x2000)
        return gC7010.firmware[address];

    /* 2 KiB RAM is mirrored through the upper 8 KiB region. */
    if (address >= 0xE000)
        return gC7010.ram[address & 0x07FF];

    return 0xFF;
}

void C7010_NSC800_WriteMemory(unsigned short address, uint8_t value)
{
    if (address >= 0xE000)
        gC7010.ram[address & 0x07FF] = value;
}

uint8_t C7010_NSC800_In(uint8_t port)
{
    uint8_t value;

    (void)port;
    value = gC7010.to_nsc800;
    if (gC7010.to_nsc_read_generation != gC7010.to_nsc_write_generation)
        gC7010.to_nsc_read_generation = gC7010.to_nsc_write_generation;
    return value;
}

void C7010_NSC800_Out(uint8_t port, uint8_t value)
{
    (void)port;
    gC7010.to_8048 = value;
    ++gC7010.to_8048_write_generation;
}

/*
 * ---------------------------------------------------------------------------
 * NSC800 register, flag and instruction helpers
 *
 * Pure building blocks only: registers, flags, arithmetic, stack, fetch,
 * conditions and primitives used by the base and prefixed-opcode handlers.
 * ---------------------------------------------------------------------------
 */

static uint8_t NscFetch8(void)
{
    return C7010_NSC800_ReadMemory(gC7010.nsc.pc++);
}

static unsigned short NscFetch16(void)
{
    uint8_t lo = NscFetch8();
    uint8_t hi = NscFetch8();
    return (unsigned short)((unsigned int)lo | ((unsigned int)hi << 8));
}

static void NscPush16(unsigned short v)
{
    C7010_NSC800_WriteMemory(--gC7010.nsc.sp, (uint8_t)(v >> 8));
    C7010_NSC800_WriteMemory(--gC7010.nsc.sp, (uint8_t)v);
}

static unsigned short NscPop16(void)
{
    uint8_t lo = C7010_NSC800_ReadMemory(gC7010.nsc.sp++);
    uint8_t hi = C7010_NSC800_ReadMemory(gC7010.nsc.sp++);
    return (unsigned short)((unsigned int)lo | ((unsigned int)hi << 8));
}

static uint8_t NscInc8(uint8_t value)
{
    uint8_t result = (uint8_t)(value + 1);
    uint8_t flags = (uint8_t)(gC7010.nsc.f & 0x01); /* preserve carry */

    flags |= (uint8_t)(result & 0xA8);            /* S, Y(bit5), X(bit3) */
    if (result == 0)
        flags |= 0x40;                         /* Z */
    if ((value & 0x0F) == 0x0F)
        flags |= 0x10;                         /* H */
    if (value == 0x7F)
        flags |= 0x04;                         /* P/V */

    gC7010.nsc.f = flags;
    return result;
}

static uint8_t NscDec8(uint8_t value)
{
    uint8_t result = (uint8_t)(value - 1);
    uint8_t flags = (uint8_t)((gC7010.nsc.f & 0x01) | 0x02); /* preserve C, set N */

    flags |= (uint8_t)(result & 0xA8);                    /* S, Y, X */
    if (result == 0)
        flags |= 0x40;                                 /* Z */
    if ((value & 0x0F) == 0x00)
        flags |= 0x10;                                 /* H */
    if (value == 0x80)
        flags |= 0x04;                                 /* P/V */

    gC7010.nsc.f = flags;
    return result;
}

static bool NscEvenParity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1) == 0;
}

static void NscSetLogicFlags(uint8_t result, bool halfCarry)
{
    uint8_t flags = (uint8_t)(result & 0xA8);
    if (result == 0)
        flags |= 0x40;
    if (halfCarry)
        flags |= 0x10;
    if (NscEvenParity(result))
        flags |= 0x04;
    gC7010.nsc.f = flags;
}

static uint8_t NscAdd8(uint8_t lhs, uint8_t rhs, uint8_t carryIn)
{
    unsigned int sum = (unsigned int)lhs + (unsigned int)rhs + (unsigned int)carryIn;
    uint8_t result = (uint8_t)sum;
    uint8_t flags = (uint8_t)(result & 0xA8);

    if (result == 0)
        flags |= 0x40;
    if (((unsigned int)(lhs & 0x0F) + (unsigned int)(rhs & 0x0F) + (unsigned int)carryIn) > 0x0F)
        flags |= 0x10;
    if (((~(lhs ^ rhs) & (lhs ^ result)) & 0x80) != 0)
        flags |= 0x04;
    if (sum > 0xFF)
        flags |= 0x01;
    gC7010.nsc.f = flags;
    return result;
}

static uint8_t NscSub8(uint8_t lhs, uint8_t rhs, uint8_t carryIn)
{
    int diff = (int)lhs - (int)rhs - (int)carryIn;
    uint8_t result = (uint8_t)diff;
    uint8_t flags = (uint8_t)((result & 0xA8) | 0x02);

    if (result == 0)
        flags |= 0x40;
    if ((lhs & 0x0F) < ((rhs & 0x0F) + carryIn))
        flags |= 0x10;
    if ((((lhs ^ rhs) & (lhs ^ result)) & 0x80) != 0)
        flags |= 0x04;
    if (diff < 0)
        flags |= 0x01;
    gC7010.nsc.f = flags;
    return result;
}

static unsigned short NscGetBC(void)
{
    return (unsigned short)(((unsigned int)gC7010.nsc.b << 8) | (unsigned int)gC7010.nsc.c);
}

static void NscSetBC(unsigned short value)
{
    gC7010.nsc.b = (uint8_t)(value >> 8);
    gC7010.nsc.c = (uint8_t)value;
}

static unsigned short NscGetDE(void)
{
    return (unsigned short)(((unsigned int)gC7010.nsc.d << 8) | (unsigned int)gC7010.nsc.e);
}

static void NscSetDE(unsigned short value)
{
    gC7010.nsc.d = (uint8_t)(value >> 8);
    gC7010.nsc.e = (uint8_t)value;
}

static uint8_t NscReadReg(unsigned int code)
{
    switch (code & 7u)
    {
    case 0: return gC7010.nsc.b;
    case 1: return gC7010.nsc.c;
    case 2: return gC7010.nsc.d;
    case 3: return gC7010.nsc.e;
    case 4: return (uint8_t)(gC7010.nsc.hl >> 8);
    case 5: return (uint8_t)gC7010.nsc.hl;
    case 6: return C7010_NSC800_ReadMemory(gC7010.nsc.hl);
    default: return gC7010.nsc.a;
    }
}

static void NscWriteReg(unsigned int code, uint8_t value)
{
    switch (code & 7u)
    {
    case 0: gC7010.nsc.b = value; break;
    case 1: gC7010.nsc.c = value; break;
    case 2: gC7010.nsc.d = value; break;
    case 3: gC7010.nsc.e = value; break;
    case 4: gC7010.nsc.hl = (unsigned short)((gC7010.nsc.hl & 0x00FFu) | ((unsigned int)value << 8)); break;
    case 5: gC7010.nsc.hl = (unsigned short)((gC7010.nsc.hl & 0xFF00u) | (unsigned int)value); break;
    case 6: C7010_NSC800_WriteMemory(gC7010.nsc.hl, value); break;
    default: gC7010.nsc.a = value; break;
    }
}

static bool NscCondition(unsigned int code)
{
    switch (code & 7u)
    {
    case 0: return (gC7010.nsc.f & 0x40) == 0; /* NZ */
    case 1: return (gC7010.nsc.f & 0x40) != 0; /* Z */
    case 2: return (gC7010.nsc.f & 0x01) == 0; /* NC */
    case 3: return (gC7010.nsc.f & 0x01) != 0; /* C */
    case 4: return (gC7010.nsc.f & 0x04) == 0; /* PO */
    case 5: return (gC7010.nsc.f & 0x04) != 0; /* PE */
    case 6: return (gC7010.nsc.f & 0x80) == 0; /* P */
    default: return (gC7010.nsc.f & 0x80) != 0; /* M */
    }
}

static unsigned short NscAdd16(unsigned short lhs, unsigned short rhs)
{
    unsigned int sum = (unsigned int)lhs + (unsigned int)rhs;
    unsigned short result = (unsigned short)sum;
    uint8_t flags = (uint8_t)(gC7010.nsc.f & (0x80 | 0x40 | 0x04)); /* preserve S/Z/PV */

    flags |= (uint8_t)(((unsigned int)result >> 8) & 0x28);
    if (((unsigned int)(lhs & 0x0FFFu) + (unsigned int)(rhs & 0x0FFFu)) > 0x0FFFu)
        flags |= 0x10;
    if (sum > 0xFFFFu)
        flags |= 0x01;
    gC7010.nsc.f = flags;
    return result;
}

static unsigned short NscIndexedAddress(unsigned short index, signed char displacement)
{
    return (unsigned short)((unsigned int)index + (int)displacement);
}

static uint8_t NscReadIndexedReg(unsigned int code, unsigned short index,
                              bool memoryUsesDisplacement, signed char displacement)
{
    switch (code & 7u)
    {
    case 0: return gC7010.nsc.b;
    case 1: return gC7010.nsc.c;
    case 2: return gC7010.nsc.d;
    case 3: return gC7010.nsc.e;
    case 4: return (uint8_t)(index >> 8);
    case 5: return (uint8_t)index;
    case 6:
        return C7010_NSC800_ReadMemory(memoryUsesDisplacement
            ? NscIndexedAddress(index, displacement)
            : index);
    default: return gC7010.nsc.a;
    }
}

static void NscWriteIndexedReg(unsigned int code, uint8_t value, unsigned short *index,
                               bool memoryUsesDisplacement, signed char displacement)
{
    switch (code & 7u)
    {
    case 0: gC7010.nsc.b = value; break;
    case 1: gC7010.nsc.c = value; break;
    case 2: gC7010.nsc.d = value; break;
    case 3: gC7010.nsc.e = value; break;
    case 4: *index = (unsigned short)((*index & 0x00FFu) | ((unsigned int)value << 8)); break;
    case 5: *index = (unsigned short)((*index & 0xFF00u) | (unsigned int)value); break;
    case 6:
        C7010_NSC800_WriteMemory(memoryUsesDisplacement
            ? NscIndexedAddress(*index, displacement)
            : *index, value);
        break;
    default: gC7010.nsc.a = value; break;
    }
}

/* Rotate/shift primitive used by the CB and indexed-CB handlers. */
static uint8_t NscRotateShift(uint8_t value, unsigned int bit, uint8_t *carry)
{
    uint8_t result = value;
    uint8_t c = 0;

    switch (bit & 7u)
    {
    case 0: c = (uint8_t)(value >> 7); result = (uint8_t)((value << 1) | c); break;          /* RLC */
    case 1: c = (uint8_t)(value & 1);  result = (uint8_t)((value >> 1) | (c << 7)); break;   /* RRC */
    case 2: { uint8_t oldC = (uint8_t)(gC7010.nsc.f & 1); c = (uint8_t)(value >> 7); result = (uint8_t)((value << 1) | oldC); break; } /* RL */
    case 3: { uint8_t oldC = (uint8_t)(gC7010.nsc.f & 1); c = (uint8_t)(value & 1);  result = (uint8_t)((value >> 1) | (oldC << 7)); break; } /* RR */
    case 4: c = (uint8_t)(value >> 7); result = (uint8_t)(value << 1); break;                 /* SLA */
    case 5: c = (uint8_t)(value & 1);  result = (uint8_t)((value >> 1) | (value & 0x80)); break; /* SRA */
    case 6: c = (uint8_t)(value >> 7); result = (uint8_t)((value << 1) | 1); break;           /* SLL */
    default: c = (uint8_t)(value & 1); result = (uint8_t)(value >> 1); break;                 /* SRL */
    }

    *carry = c;
    return result;
}

/* CB-style flags for a shifted/rotated result. */
static void NscSetShiftFlags(uint8_t result, uint8_t carry)
{
    uint8_t flags = (uint8_t)(result & 0xA8);
    if (result == 0)
        flags |= 0x40;
    if (NscEvenParity(result))
        flags |= 0x04;
    flags |= carry;
    gC7010.nsc.f = flags;
}

/* BIT b,(IX/IY+d) flags; undocumented 5/3 come from the effective address. */
static void NscBitIndexedFlags(uint8_t value, unsigned int bit, unsigned short address)
{
    bool set = (value & (uint8_t)(1u << (bit & 7u))) != 0;
    uint8_t flags = (uint8_t)((gC7010.nsc.f & 0x01) | 0x10); /* preserve C, H=1, N=0 */

    if (!set)
        flags |= (uint8_t)(0x40 | 0x04);
    if ((bit & 7u) == 7u && set)
        flags |= 0x80;
    flags |= (uint8_t)(((unsigned int)address >> 8) & 0x28);
    gC7010.nsc.f = flags;
}

/*
 * Primitives used by the ED-prefix handler (ported from the reference
 * lambdas; no ED dispatch here).
 */
static unsigned short NscRead16(unsigned short address)
{
    uint8_t lo = C7010_NSC800_ReadMemory(address);
    uint8_t hi = C7010_NSC800_ReadMemory((unsigned short)(address + 1));
    return (unsigned short)((unsigned int)lo | ((unsigned int)hi << 8));
}

static void NscWrite16(unsigned short address, unsigned short value)
{
    C7010_NSC800_WriteMemory(address, (uint8_t)value);
    C7010_NSC800_WriteMemory((unsigned short)(address + 1), (uint8_t)(value >> 8));
}

static unsigned short NscGetPair(unsigned int pair)
{
    switch (pair & 3u)
    {
    case 0: return NscGetBC();
    case 1: return NscGetDE();
    case 2: return gC7010.nsc.hl;
    default: return gC7010.nsc.sp;
    }
}

static void NscSetPair(unsigned int pair, unsigned short value)
{
    switch (pair & 3u)
    {
    case 0: NscSetBC(value); break;
    case 1: NscSetDE(value); break;
    case 2: gC7010.nsc.hl = value; break;
    default: gC7010.nsc.sp = value; break;
    }
}

static void NscAdcSbcHl(unsigned short rhs, bool subtract)
{
    unsigned short lhs = gC7010.nsc.hl;
    unsigned int carry = (gC7010.nsc.f & 1) ? 1u : 0u;
    unsigned int wide = subtract
        ? (unsigned int)lhs - (unsigned int)rhs - carry
        : (unsigned int)lhs + (unsigned int)rhs + carry;
    unsigned short result = (unsigned short)wide;
    uint8_t f = (uint8_t)(((unsigned int)result >> 8) & 0xA8);

    if (result == 0)
        f |= 0x40;
    if (subtract)
    {
        f |= 0x02;
        if ((lhs & 0x0FFFu) < ((rhs & 0x0FFFu) + carry))
            f |= 0x10;
        if ((((lhs ^ rhs) & (lhs ^ result)) & 0x8000u) != 0)
            f |= 0x04;
        if ((unsigned int)lhs < (unsigned int)rhs + carry)
            f |= 0x01;
    }
    else
    {
        if (((unsigned int)(lhs & 0x0FFFu) + (unsigned int)(rhs & 0x0FFFu) + carry) > 0x0FFFu)
            f |= 0x10;
        if (((~(lhs ^ rhs) & (lhs ^ result)) & 0x8000u) != 0)
            f |= 0x04;
        if (wide > 0xFFFFu)
            f |= 0x01;
    }
    gC7010.nsc.hl = result;
    gC7010.nsc.f = f;
}

static void NscSetInFlags(uint8_t value)
{
    uint8_t f = (uint8_t)((gC7010.nsc.f & 0x01) | (value & 0xA8));

    if (value == 0)
        f |= 0x40;
    if (NscEvenParity(value))
        f |= 0x04;
    gC7010.nsc.f = f;
}

static void NscLdi(int direction)
{
    unsigned short bc = NscGetBC();
    unsigned short de = NscGetDE();
    uint8_t value = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
    uint8_t f;
    uint8_t sum;

    C7010_NSC800_WriteMemory(de, value);
    gC7010.nsc.hl = (unsigned short)((unsigned int)gC7010.nsc.hl + (unsigned int)direction);
    de = (unsigned short)((unsigned int)de + (unsigned int)direction);
    bc = (unsigned short)((unsigned int)bc - 1u);
    NscSetDE(de);
    NscSetBC(bc);

    f = (uint8_t)(gC7010.nsc.f & (0x80 | 0x40 | 0x01));
    if (bc != 0)
        f |= 0x04;
    sum = (uint8_t)(gC7010.nsc.a + value);
    f |= (uint8_t)(sum & 0x08);
    if (sum & 0x02)
        f |= 0x20;
    gC7010.nsc.f = f;
}

static void NscCpi(int direction)
{
    unsigned short bc = NscGetBC();
    uint8_t value = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
    uint8_t oldC = (uint8_t)(gC7010.nsc.f & 1);

    (void)NscSub8(gC7010.nsc.a, value, 0);
    gC7010.nsc.hl = (unsigned short)((unsigned int)gC7010.nsc.hl + (unsigned int)direction);
    bc = (unsigned short)((unsigned int)bc - 1u);
    NscSetBC(bc);
    gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & ~0x05) | oldC | (bc != 0 ? 0x04 : 0));
}

static void NscAndA(void)
{
    uint8_t result = gC7010.nsc.a;
    uint8_t flags = (uint8_t)((result & 0xA8) | 0x10); /* S,Y,X + H */
    uint8_t parity = result;

    if (result == 0)
        flags |= 0x40;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if ((parity & 1) == 0)
        flags |= 0x04;
    gC7010.nsc.f = flags;
}

static void NscConditionalRet(unsigned int cc)
{
    if (NscCondition(cc))
        gC7010.nsc.pc = NscPop16();
}

static void NscConditionalCall(unsigned int cc)
{
    unsigned short target = NscFetch16();
    if (NscCondition(cc))
    {
        NscPush16(gC7010.nsc.pc);
        gC7010.nsc.pc = target;
    }
}

static void NscConditionalJump(unsigned int cc)
{
    unsigned short target = NscFetch16();
    if (NscCondition(cc))
        gC7010.nsc.pc = target;
}

static void NscRst(unsigned short vector)
{
    NscPush16(gC7010.nsc.pc);
    gC7010.nsc.pc = vector;
}

static bool NscExecuteRegisterMatrix(uint8_t op)
{
    uint8_t value;

    if (op >= 0x40 && op <= 0x7F)
    {
        if (op == 0x76)
            return false; /* HALT stays in the explicit switch. */
        NscWriteReg((op >> 3) & 7u, NscReadReg(op & 7u));
        return true;
    }

    if (op < 0x80 || op > 0xBF)
        return false;

    value = NscReadReg(op & 7u);
    switch ((op >> 3) & 7u)
    {
    case 0: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, value, 0); break;
    case 1: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, value, (gC7010.nsc.f & 1) ? 1 : 0); break;
    case 2: gC7010.nsc.a = NscSub8(gC7010.nsc.a, value, 0); break;
    case 3: gC7010.nsc.a = NscSub8(gC7010.nsc.a, value, (gC7010.nsc.f & 1) ? 1 : 0); break;
    case 4: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a & value); NscSetLogicFlags(gC7010.nsc.a, true); break;
    case 5: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a ^ value); NscSetLogicFlags(gC7010.nsc.a, false); break;
    case 6: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a | value); NscSetLogicFlags(gC7010.nsc.a, false); break;
    case 7: (void)NscSub8(gC7010.nsc.a, value, 0); break;
    }
    return true;
}

static bool NscExecuteCB(unsigned short prefixPc)
{
    const uint8_t op2 = NscFetch8();
    const unsigned int group = op2 >> 6;
    const unsigned int bit = (op2 >> 3) & 7u;
    const unsigned int reg = op2 & 7u;
    uint8_t value = NscReadReg(reg);

    (void)prefixPc; /* Kept for parity with the reference signature. */

    if (group == 0)
    {
        uint8_t carry = 0;
        uint8_t result = NscRotateShift(value, bit, &carry);
        NscWriteReg(reg, result);
        NscSetShiftFlags(result, carry);
        return true;
    }

    if (group == 1) /* BIT b,r */
    {
        const bool set = (value & (uint8_t)(1u << bit)) != 0;
        uint8_t flags = (uint8_t)((gC7010.nsc.f & 0x01) | 0x10); /* preserve C, set H, clear N */
        if (!set)
            flags |= (uint8_t)(0x40 | 0x04); /* Z and P/V */
        if (bit == 7 && set)
            flags |= 0x80;
        flags |= (uint8_t)(value & 0x28);
        gC7010.nsc.f = flags;
        return true;
    }

    if (group == 2) /* RES b,r */
        value = (uint8_t)(value & ~(uint8_t)(1u << bit));
    else            /* SET b,r */
        value = (uint8_t)(value | (uint8_t)(1u << bit));

    NscWriteReg(reg, value);
    return true;
}

static bool NscExecuteED(unsigned short prefixPc)
{
    const uint8_t op2 = NscFetch8();

    /* IN r,(C) / OUT (C),r */
    if ((op2 & 0xC7) == 0x40)
    {
        const unsigned int r = (op2 >> 3) & 7u;
        const uint8_t value = C7010_NSC800_In(gC7010.nsc.c);
        if (r != 6u)
            NscWriteReg(r, value); /* ED70 discards the input value. */
        NscSetInFlags(value);
        return true;
    }
    if ((op2 & 0xC7) == 0x41)
    {
        const unsigned int r = (op2 >> 3) & 7u;
        C7010_NSC800_Out(gC7010.nsc.c, r == 6u ? 0 : NscReadReg(r));
        return true;
    }

    /* SBC/ADC HL,rr */
    if ((op2 & 0xCF) == 0x42)
    {
        NscAdcSbcHl(NscGetPair((op2 >> 4) & 3u), true);
        return true;
    }
    if ((op2 & 0xCF) == 0x4A)
    {
        NscAdcSbcHl(NscGetPair((op2 >> 4) & 3u), false);
        return true;
    }

    /* LD (nn),rr -- ED43/53/63/73. */
    if ((op2 & 0xCF) == 0x43)
    {
        const unsigned short address = NscFetch16();
        NscWrite16(address, NscGetPair((op2 >> 4) & 3u));
        return true;
    }
    /* LD rr,(nn) -- ED4B/5B/6B/7B. */
    if ((op2 & 0xCF) == 0x4B)
    {
        const unsigned short address = NscFetch16();
        NscSetPair((op2 >> 4) & 3u, NscRead16(address));
        return true;
    }

    switch (op2)
    {
    /* NEG aliases. */
    case 0x44: case 0x4C: case 0x54: case 0x5C:
    case 0x64: case 0x6C: case 0x74: case 0x7C:
        gC7010.nsc.a = NscSub8(0, gC7010.nsc.a, 0);
        return true;

    /* RETN aliases and RETI. */
    case 0x45: case 0x55: case 0x65: case 0x75:
        gC7010.nsc.pc = NscPop16();
        gC7010.nsc.iff1 = gC7010.nsc.iff2;
        return true;
    case 0x4D: case 0x5D: case 0x6D: case 0x7D:
        gC7010.nsc.pc = NscPop16();
        gC7010.nsc.iff1 = gC7010.nsc.iff2;
        return true;

    /* Interrupt modes and their documented aliases. */
    case 0x46: case 0x4E: case 0x66: case 0x6E:
        gC7010.nsc.interrupt_mode = 0;
        return true;
    case 0x56: case 0x76:
        gC7010.nsc.interrupt_mode = 1;
        return true;
    case 0x5E: case 0x7E:
        gC7010.nsc.interrupt_mode = 2;
        return true;

    case 0x47: gC7010.nsc.i = gC7010.nsc.a; return true; /* LD I,A */
    case 0x4F: gC7010.nsc.r = gC7010.nsc.a; return true; /* LD R,A */
    case 0x57: /* LD A,I */
        gC7010.nsc.a = gC7010.nsc.i;
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0x01) | (gC7010.nsc.a & 0xA8) |
            (gC7010.nsc.a == 0 ? 0x40 : 0) | (gC7010.nsc.iff2 ? 0x04 : 0));
        return true;
    case 0x5F: /* LD A,R */
        gC7010.nsc.a = gC7010.nsc.r;
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0x01) | (gC7010.nsc.a & 0xA8) |
            (gC7010.nsc.a == 0 ? 0x40 : 0) | (gC7010.nsc.iff2 ? 0x04 : 0));
        return true;

    case 0x67: /* RRD */
    {
        const uint8_t mem = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
        const uint8_t newMem = (uint8_t)((gC7010.nsc.a << 4) | (mem >> 4));
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a & 0xF0) | (mem & 0x0F));
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, newMem);
        NscSetInFlags(gC7010.nsc.a);
        return true;
    }
    case 0x6F: /* RLD */
    {
        const uint8_t mem = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
        const uint8_t newMem = (uint8_t)((mem << 4) | (gC7010.nsc.a & 0x0F));
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a & 0xF0) | (mem >> 4));
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, newMem);
        NscSetInFlags(gC7010.nsc.a);
        return true;
    }

    case 0xA0: NscLdi(+1); return true; /* LDI */
    case 0xA8: NscLdi(-1); return true; /* LDD */
    case 0xA1: NscCpi(+1); return true; /* CPI */
    case 0xA9: NscCpi(-1); return true; /* CPD */

    case 0xA2: case 0xAA: /* INI / IND */
    {
        const int dir = op2 == 0xA2 ? +1 : -1;
        const uint8_t value = C7010_NSC800_In(gC7010.nsc.c);
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, value);
        gC7010.nsc.hl = (unsigned short)(gC7010.nsc.hl + dir);
        gC7010.nsc.b = (uint8_t)(gC7010.nsc.b - 1);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.b & 0xA8) |
            (gC7010.nsc.b == 0 ? 0x40 : 0) | 0x02);
        return true;
    }
    case 0xA3: case 0xAB: /* OUTI / OUTD */
    {
        const int dir = op2 == 0xA3 ? +1 : -1;
        C7010_NSC800_Out(gC7010.nsc.c, C7010_NSC800_ReadMemory(gC7010.nsc.hl));
        gC7010.nsc.hl = (unsigned short)(gC7010.nsc.hl + dir);
        gC7010.nsc.b = (uint8_t)(gC7010.nsc.b - 1);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.b & 0xA8) |
            (gC7010.nsc.b == 0 ? 0x40 : 0) | 0x02);
        return true;
    }

    /* Repeating block instructions. */
    case 0xB0: /* LDIR */
        do { NscLdi(+1); } while (NscGetBC() != 0);
        return true;
    case 0xB8: /* LDDR */
        do { NscLdi(-1); } while (NscGetBC() != 0);
        return true;
    case 0xB1: /* CPIR */
        do { NscCpi(+1); } while (NscGetBC() != 0 && (gC7010.nsc.f & 0x40) == 0);
        return true;
    case 0xB9: /* CPDR */
        do { NscCpi(-1); } while (NscGetBC() != 0 && (gC7010.nsc.f & 0x40) == 0);
        return true;
    case 0xB2: case 0xBA: /* INIR / INDR */
    {
        const int dir = op2 == 0xB2 ? +1 : -1;
        do
        {
            const uint8_t value = C7010_NSC800_In(gC7010.nsc.c);
            C7010_NSC800_WriteMemory(gC7010.nsc.hl, value);
            gC7010.nsc.hl = (unsigned short)(gC7010.nsc.hl + dir);
            gC7010.nsc.b = (uint8_t)(gC7010.nsc.b - 1);
        } while (gC7010.nsc.b != 0);
        gC7010.nsc.f = 0x42; /* Z,N after the repeat terminates. */
        return true;
    }
    case 0xB3: case 0xBB: /* OTIR / OTDR */
    {
        const int dir = op2 == 0xB3 ? +1 : -1;
        do
        {
            C7010_NSC800_Out(gC7010.nsc.c, C7010_NSC800_ReadMemory(gC7010.nsc.hl));
            gC7010.nsc.hl = (unsigned short)(gC7010.nsc.hl + dir);
            gC7010.nsc.b = (uint8_t)(gC7010.nsc.b - 1);
        } while (gC7010.nsc.b != 0);
        gC7010.nsc.f = 0x42;
        return true;
    }

    default:
        C7010_Log("C7010: NSC800 unsupported ED%02X at PC=%04X\n", op2, prefixPc);
        gC7010.nsc.stopped = true;
        return false;
    }
}

static bool NscExecuteIndexedCB(unsigned short prefixPc, unsigned short *index,
                                const char *name)
{
    const signed char displacement = (signed char)NscFetch8();
    const uint8_t op = NscFetch8();
    const unsigned short address = NscIndexedAddress(*index, displacement);
    uint8_t value = C7010_NSC800_ReadMemory(address);
    const unsigned int group = op >> 6;
    const unsigned int bit = (op >> 3) & 7u;
    const unsigned int reg = op & 7u;

    (void)prefixPc;
    (void)name;

    if (group == 0)
    {
        uint8_t carry = 0;
        const uint8_t result = NscRotateShift(value, bit, &carry);
        C7010_NSC800_WriteMemory(address, result);
        if (reg != 6u)
            NscWriteReg(reg, result);
        NscSetShiftFlags(result, carry);
        return true;
    }

    if (group == 1) /* BIT b,(IX/IY+d) */
    {
        const bool set = (value & (uint8_t)(1u << bit)) != 0;
        uint8_t flags = (uint8_t)((gC7010.nsc.f & 0x01) | 0x10); /* preserve C, H=1, N=0 */
        if (!set)
            flags |= (uint8_t)(0x40 | 0x04);
        if (bit == 7 && set)
            flags |= 0x80;
        /* Indexed BIT gets undocumented 5/3 from the effective-address high byte. */
        flags |= (uint8_t)(((unsigned int)address >> 8) & 0x28);
        gC7010.nsc.f = flags;
        return true;
    }

    value = (group == 2)
        ? (uint8_t)(value & ~(uint8_t)(1u << bit))
        : (uint8_t)(value | (uint8_t)(1u << bit));
    C7010_NSC800_WriteMemory(address, value);
    if (reg != 6u)
        NscWriteReg(reg, value);
    return true;
}

static bool NscExecuteIndexed(unsigned short prefixPc, unsigned short *index,
                              const char *name)
{
    const uint8_t op = NscFetch8();

    switch (op)
    {
    case 0x09: *index = NscAdd16(*index, NscGetBC()); return true;
    case 0x19: *index = NscAdd16(*index, NscGetDE()); return true;
    case 0x21: *index = NscFetch16(); return true;
    case 0x22:
    {
        const unsigned short address = NscFetch16();
        C7010_NSC800_WriteMemory(address, (uint8_t)*index);
        C7010_NSC800_WriteMemory((unsigned short)(address + 1), (uint8_t)(*index >> 8));
        return true;
    }
    case 0x23: ++(*index); return true;
    case 0x24:
    {
        const uint8_t hi = NscInc8((uint8_t)(*index >> 8));
        *index = (unsigned short)(((unsigned short)hi << 8) | (*index & 0x00FFu));
        return true;
    }
    case 0x25:
    {
        const uint8_t hi = NscDec8((uint8_t)(*index >> 8));
        *index = (unsigned short)(((unsigned short)hi << 8) | (*index & 0x00FFu));
        return true;
    }
    case 0x26:
    {
        const uint8_t hi = NscFetch8();
        *index = (unsigned short)(((unsigned short)hi << 8) | (*index & 0x00FFu));
        return true;
    }
    case 0x29: *index = NscAdd16(*index, *index); return true;
    case 0x2A:
    {
        const unsigned short address = NscFetch16();
        *index = (unsigned short)(C7010_NSC800_ReadMemory(address) |
            ((unsigned short)C7010_NSC800_ReadMemory((unsigned short)(address + 1)) << 8));
        return true;
    }
    case 0x2B: --(*index); return true;
    case 0x2C:
    {
        const uint8_t lo = NscInc8((uint8_t)*index);
        *index = (unsigned short)((*index & 0xFF00u) | lo);
        return true;
    }
    case 0x2D:
    {
        const uint8_t lo = NscDec8((uint8_t)*index);
        *index = (unsigned short)((*index & 0xFF00u) | lo);
        return true;
    }
    case 0x2E:
    {
        const uint8_t lo = NscFetch8();
        *index = (unsigned short)((*index & 0xFF00u) | lo);
        return true;
    }
    case 0x34:
    {
        const signed char d = (signed char)NscFetch8();
        const unsigned short address = NscIndexedAddress(*index, d);
        const uint8_t value = C7010_NSC800_ReadMemory(address);
        C7010_NSC800_WriteMemory(address, NscInc8(value));
        return true;
    }
    case 0x35:
    {
        const signed char d = (signed char)NscFetch8();
        const unsigned short address = NscIndexedAddress(*index, d);
        const uint8_t value = C7010_NSC800_ReadMemory(address);
        C7010_NSC800_WriteMemory(address, NscDec8(value));
        return true;
    }
    case 0x36:
    {
        const signed char d = (signed char)NscFetch8();
        const uint8_t value = NscFetch8();
        C7010_NSC800_WriteMemory(NscIndexedAddress(*index, d), value);
        return true;
    }
    case 0x39: *index = NscAdd16(*index, gC7010.nsc.sp); return true;
    case 0xCB: return NscExecuteIndexedCB(prefixPc, index, name);
    case 0xE1: *index = NscPop16(); return true;
    case 0xE3:
    {
        const unsigned short stackValue = NscPop16();
        NscPush16(*index);
        *index = stackValue;
        return true;
    }
    case 0xE5: NscPush16(*index); return true;
    case 0xE9: gC7010.nsc.pc = *index; return true;
    case 0xF9: gC7010.nsc.sp = *index; return true;
    case 0xDD: /* repeated DD/FD prefixes: last prefix wins */
        return NscExecuteIndexed(prefixPc, &gC7010.nsc.ix, "DD");
    case 0xFD:
        return NscExecuteIndexed(prefixPc, &gC7010.nsc.iy, "FD");
    default:
        break;
    }

    /* LD r,r' and ALU r groups: H/L become IXH/IXL (or IYH/IYL), and
     * any (HL) operand becomes (IX/IY+d), consuming one displacement byte. */
    if ((op >= 0x40 && op <= 0x7F && op != 0x76) || (op >= 0x80 && op <= 0xBF))
    {
        const unsigned int dst = (op >> 3) & 7u;
        const unsigned int src = op & 7u;
        const bool memoryOperand = (dst == 6u || src == 6u);
        const signed char displacement = memoryOperand ? (signed char)NscFetch8() : 0;
        uint8_t value;

        if (op >= 0x40 && op <= 0x7F)
        {
            value = NscReadIndexedReg(src, *index, memoryOperand, displacement);
            NscWriteIndexedReg(dst, value, index, memoryOperand, displacement);
            return true;
        }

        value = NscReadIndexedReg(src, *index, memoryOperand, displacement);
        switch ((op >> 3) & 7u)
        {
        case 0: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, value, 0); break;
        case 1: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, value, (gC7010.nsc.f & 1) ? 1 : 0); break;
        case 2: gC7010.nsc.a = NscSub8(gC7010.nsc.a, value, 0); break;
        case 3: gC7010.nsc.a = NscSub8(gC7010.nsc.a, value, (gC7010.nsc.f & 1) ? 1 : 0); break;
        case 4: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a & value); NscSetLogicFlags(gC7010.nsc.a, true); break;
        case 5: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a ^ value); NscSetLogicFlags(gC7010.nsc.a, false); break;
        case 6: gC7010.nsc.a = (uint8_t)(gC7010.nsc.a | value); NscSetLogicFlags(gC7010.nsc.a, false); break;
        case 7: (void)NscSub8(gC7010.nsc.a, value, 0); break;
        }
        return true;
    }

    /* For opcodes where DD/FD has no architectural effect, the Z80 treats
     * the prefix as ignored. */
    switch (op)
    {
    case 0x00: return true; /* NOP */
    case 0x18:
    {
        const signed char d = (signed char)NscFetch8();
        gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        return true;
    }
    case 0x20:
    {
        const signed char d = (signed char)NscFetch8();
        if (!(gC7010.nsc.f & 0x40))
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        return true;
    }
    case 0x28:
    {
        const signed char d = (signed char)NscFetch8();
        if (gC7010.nsc.f & 0x40)
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        return true;
    }
    case 0x30:
    {
        const signed char d = (signed char)NscFetch8();
        if (!(gC7010.nsc.f & 1))
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        return true;
    }
    case 0x38:
    {
        const signed char d = (signed char)NscFetch8();
        if (gC7010.nsc.f & 1)
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        return true;
    }
    case 0xC3: gC7010.nsc.pc = NscFetch16(); return true;
    case 0xC9: gC7010.nsc.pc = NscPop16(); return true;
    case 0xCD:
    {
        const unsigned short address = NscFetch16();
        NscPush16(gC7010.nsc.pc);
        gC7010.nsc.pc = address;
        return true;
    }
    case 0xD3:
    {
        const uint8_t p = NscFetch8();
        C7010_NSC800_Out(p, gC7010.nsc.a);
        return true;
    }
    case 0xDB:
    {
        const uint8_t p = NscFetch8();
        gC7010.nsc.a = C7010_NSC800_In(p);
        return true;
    }
    case 0xED: return NscExecuteED(prefixPc);
    case 0xF3: gC7010.nsc.iff1 = false; gC7010.nsc.iff2 = false; return true;
    case 0xFB: gC7010.nsc.iff1 = true; gC7010.nsc.iff2 = true; return true;
    default:
        C7010_Log("C7010: NSC800 unsupported %s%02X at PC=%04X (%s=%04X)\n",
            name, op, prefixPc, name + 1, *index);
        gC7010.nsc.stopped = true;
        return false;
    }
}

static void NscResetExecution(void)
{
    memset(&gC7010.nsc, 0, sizeof(gC7010.nsc));
    gC7010.nsc.sp = 0xFFFF;
    gC7010.nsc.initialized = true;
    C7010_Log("C7010: NSC800 execution started at PC=0000 SP=FFFF\n");
}

/*
 * NSC800 opcode decoder (base, CB, DD, FD and ED instruction groups).
 */
static void C7010_ExecuteOneStep(void)
{
    unsigned short pc0;
    uint8_t op;

    if (!gC7010.enabled || !gC7010.firmware_loaded || gC7010.cpu_reset)
        return;

    if (!gC7010.nsc.initialized || gC7010.execution_needs_reset)
    {
        NscResetExecution();
        gC7010.execution_needs_reset = false;
    }
    if (gC7010.nsc.stopped)
        return;

    pc0 = gC7010.nsc.pc;
    op = NscFetch8();

    switch (op)
    {
    case 0x00: break; /* NOP */
    case 0x01:
    {
        unsigned short v = NscFetch16();
        gC7010.nsc.b = (uint8_t)(v >> 8);
        gC7010.nsc.c = (uint8_t)v;
        break;
    }
    case 0x02: C7010_NSC800_WriteMemory(NscGetBC(), gC7010.nsc.a); break;
    case 0x03: NscSetBC((unsigned short)(NscGetBC() + 1)); break;
    case 0x04: gC7010.nsc.b = NscInc8(gC7010.nsc.b); break;
    case 0x05: gC7010.nsc.b = NscDec8(gC7010.nsc.b); break;
    case 0x06: gC7010.nsc.b = NscFetch8(); break;
    case 0x07:
    {
        const uint8_t c = (uint8_t)(gC7010.nsc.a >> 7);
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a << 1) | c);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) | c);
        break;
    }
    case 0x08:
    {
        const uint8_t a = gC7010.nsc.a, f = gC7010.nsc.f;
        gC7010.nsc.a = gC7010.nsc.a2;
        gC7010.nsc.f = gC7010.nsc.f2;
        gC7010.nsc.a2 = a;
        gC7010.nsc.f2 = f;
        break;
    }
    case 0x09: gC7010.nsc.hl = NscAdd16(gC7010.nsc.hl, NscGetBC()); break;
    case 0x0A: gC7010.nsc.a = C7010_NSC800_ReadMemory(NscGetBC()); break;
    case 0x0B: NscSetBC((unsigned short)(NscGetBC() - 1)); break;
    case 0x0C: gC7010.nsc.c = NscInc8(gC7010.nsc.c); break;
    case 0x0D: gC7010.nsc.c = NscDec8(gC7010.nsc.c); break;
    case 0x0E: gC7010.nsc.c = NscFetch8(); break;
    case 0x0F:
    {
        const uint8_t c = (uint8_t)(gC7010.nsc.a & 1);
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a >> 1) | (c << 7));
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) | c);
        break;
    }
    case 0x10:
    {
        const signed char displacement = (signed char)NscFetch8();
        gC7010.nsc.b = (uint8_t)(gC7010.nsc.b - 1);
        if (gC7010.nsc.b != 0)
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + displacement);
        break;
    }
    case 0x11:
    {
        unsigned short v = NscFetch16();
        gC7010.nsc.d = (uint8_t)(v >> 8);
        gC7010.nsc.e = (uint8_t)v;
        break;
    }
    case 0x12: C7010_NSC800_WriteMemory(NscGetDE(), gC7010.nsc.a); break;
    case 0x13: NscSetDE((unsigned short)(NscGetDE() + 1)); break;
    case 0x14: gC7010.nsc.d = NscInc8(gC7010.nsc.d); break;
    case 0x15: gC7010.nsc.d = NscDec8(gC7010.nsc.d); break;
    case 0x16: gC7010.nsc.d = NscFetch8(); break;
    case 0x17:
    {
        const uint8_t oldC = (uint8_t)(gC7010.nsc.f & 1), c = (uint8_t)(gC7010.nsc.a >> 7);
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a << 1) | oldC);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) | c);
        break;
    }
    case 0x18:
    {
        const signed char d = (signed char)NscFetch8();
        gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        break;
    }
    case 0x19: gC7010.nsc.hl = NscAdd16(gC7010.nsc.hl, NscGetDE()); break;
    case 0x1A: gC7010.nsc.a = C7010_NSC800_ReadMemory(NscGetDE()); break;
    case 0x1B: NscSetDE((unsigned short)(NscGetDE() - 1)); break;
    case 0x1C: gC7010.nsc.e = NscInc8(gC7010.nsc.e); break;
    case 0x1D: gC7010.nsc.e = NscDec8(gC7010.nsc.e); break;
    case 0x1E: gC7010.nsc.e = NscFetch8(); break;
    case 0x1F:
    {
        const uint8_t oldC = (uint8_t)(gC7010.nsc.f & 1), c = (uint8_t)(gC7010.nsc.a & 1);
        gC7010.nsc.a = (uint8_t)((gC7010.nsc.a >> 1) | (oldC << 7));
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) | c);
        break;
    }
    case 0x20:
    {
        const signed char d = (signed char)NscFetch8();
        if (!(gC7010.nsc.f & 0x40))
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        break;
    }
    case 0x21: gC7010.nsc.hl = NscFetch16(); break;
    case 0x22:
    {
        const unsigned short address = NscFetch16();
        C7010_NSC800_WriteMemory(address, (uint8_t)gC7010.nsc.hl);
        C7010_NSC800_WriteMemory((unsigned short)(address + 1), (uint8_t)(gC7010.nsc.hl >> 8));
        break;
    }
    case 0x23: ++gC7010.nsc.hl; break;
    case 0x24:
    {
        const uint8_t h = NscInc8((uint8_t)(gC7010.nsc.hl >> 8));
        gC7010.nsc.hl = (unsigned short)(((unsigned short)h << 8) | (gC7010.nsc.hl & 0xFF));
        break;
    }
    case 0x25:
    {
        const uint8_t h = NscDec8((uint8_t)(gC7010.nsc.hl >> 8));
        gC7010.nsc.hl = (unsigned short)(((unsigned short)h << 8) | (gC7010.nsc.hl & 0xFF));
        break;
    }
    case 0x26:
    {
        const uint8_t h = NscFetch8();
        gC7010.nsc.hl = (unsigned short)(((unsigned short)h << 8) | (gC7010.nsc.hl & 0xFF));
        break;
    }
    case 0x27: /* DAA */
    {
        const uint8_t oldA = gC7010.nsc.a, oldF = gC7010.nsc.f;
        uint8_t correction = 0;
        bool carry = (oldF & 1) != 0;
        uint8_t f;
        if ((oldF & 0x10) || (!(oldF & 0x02) && (oldA & 0x0F) > 9))
            correction |= 0x06;
        if (carry || (!(oldF & 0x02) && oldA > 0x99))
        {
            correction |= 0x60;
            carry = true;
        }
        gC7010.nsc.a = (oldF & 0x02) ? (uint8_t)(oldA - correction) : (uint8_t)(oldA + correction);
        f = (uint8_t)((oldF & 0x02) | (gC7010.nsc.a & 0xA8));
        if (gC7010.nsc.a == 0)
            f |= 0x40;
        if (NscEvenParity(gC7010.nsc.a))
            f |= 0x04;
        if (((oldA ^ gC7010.nsc.a ^ correction) & 0x10) != 0)
            f |= 0x10;
        if (carry)
            f |= 0x01;
        gC7010.nsc.f = f;
        break;
    }
    case 0x28:
    {
        const signed char d = (signed char)NscFetch8();
        if (gC7010.nsc.f & 0x40)
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        break;
    }
    case 0x29: gC7010.nsc.hl = NscAdd16(gC7010.nsc.hl, gC7010.nsc.hl); break;
    case 0x2A:
    {
        const unsigned short address = NscFetch16();
        gC7010.nsc.hl = (unsigned short)(C7010_NSC800_ReadMemory(address) |
            (C7010_NSC800_ReadMemory((unsigned short)(address + 1)) << 8));
        break;
    }
    case 0x2B: --gC7010.nsc.hl; break;
    case 0x2C:
    {
        const uint8_t l = NscInc8((uint8_t)gC7010.nsc.hl);
        gC7010.nsc.hl = (unsigned short)((gC7010.nsc.hl & 0xFF00u) | l);
        break;
    }
    case 0x2D:
    {
        const uint8_t l = NscDec8((uint8_t)gC7010.nsc.hl);
        gC7010.nsc.hl = (unsigned short)((gC7010.nsc.hl & 0xFF00u) | l);
        break;
    }
    case 0x2E:
    {
        const uint8_t l = NscFetch8();
        gC7010.nsc.hl = (unsigned short)((gC7010.nsc.hl & 0xFF00u) | l);
        break;
    }
    case 0x2F:
        gC7010.nsc.a = (uint8_t)(~gC7010.nsc.a);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC5) | 0x12 | (gC7010.nsc.a & 0x28));
        break;
    case 0x30:
    {
        const signed char d = (signed char)NscFetch8();
        if (!(gC7010.nsc.f & 1))
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        break;
    }
    case 0x31: gC7010.nsc.sp = NscFetch16(); break;
    case 0x32:
    {
        const unsigned short address = NscFetch16();
        C7010_NSC800_WriteMemory(address, gC7010.nsc.a);
        break;
    }
    case 0x33: ++gC7010.nsc.sp; break;
    case 0x34:
    {
        const uint8_t v = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, NscInc8(v));
        break;
    }
    case 0x35:
    {
        const uint8_t v = C7010_NSC800_ReadMemory(gC7010.nsc.hl);
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, NscDec8(v));
        break;
    }
    case 0x36:
    {
        const uint8_t n = NscFetch8();
        C7010_NSC800_WriteMemory(gC7010.nsc.hl, n);
        break;
    }
    case 0x37:
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) | 0x01);
        break;
    case 0x38:
    {
        const signed char d = (signed char)NscFetch8();
        if (gC7010.nsc.f & 1)
            gC7010.nsc.pc = (unsigned short)(gC7010.nsc.pc + d);
        break;
    }
    case 0x39: gC7010.nsc.hl = NscAdd16(gC7010.nsc.hl, gC7010.nsc.sp); break;
    case 0x3A:
    {
        const unsigned short address = NscFetch16();
        gC7010.nsc.a = C7010_NSC800_ReadMemory(address);
        break;
    }
    case 0x3B: --gC7010.nsc.sp; break;
    case 0x3C: gC7010.nsc.a = NscInc8(gC7010.nsc.a); break;
    case 0x3D: gC7010.nsc.a = NscDec8(gC7010.nsc.a); break;
    case 0x3E: gC7010.nsc.a = NscFetch8(); break;
    case 0x3F:
    {
        const uint8_t oldCarry = (uint8_t)(gC7010.nsc.f & 1);
        gC7010.nsc.f = (uint8_t)((gC7010.nsc.f & 0xC4) | (gC7010.nsc.a & 0x28) |
            (oldCarry ? 0x10 : 0) | (oldCarry ? 0 : 1));
        break;
    }
    case 0x76:
        gC7010.nsc.stopped = true;
        C7010_Log("C7010: NSC800 HALT at PC=%04X\n", pc0);
        break;
    case 0x77: C7010_NSC800_WriteMemory(gC7010.nsc.hl, gC7010.nsc.a); break;
    case 0x7E: gC7010.nsc.a = C7010_NSC800_ReadMemory(gC7010.nsc.hl); break;
    case 0xA7: NscAndA(); break;
    case 0xAF: gC7010.nsc.a = 0; NscSetLogicFlags(gC7010.nsc.a, false); break;
    case 0xC0: NscConditionalRet(0); break;
    case 0xC1: { unsigned short v = NscPop16(); NscSetBC(v); break; }
    case 0xC2: NscConditionalJump(0); break;
    case 0xC3: gC7010.nsc.pc = NscFetch16(); break;
    case 0xC4: NscConditionalCall(0); break;
    case 0xC5: NscPush16(NscGetBC()); break;
    case 0xC6: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, NscFetch8(), 0); break;
    case 0xC7: NscRst(0x00); break;
    case 0xC8: NscConditionalRet(1); break;
    case 0xC9: gC7010.nsc.pc = NscPop16(); break;
    case 0xCA: NscConditionalJump(1); break;
    case 0xCB: (void)NscExecuteCB(pc0); break;
    case 0xCC: NscConditionalCall(1); break;
    case 0xCD:
    {
        const unsigned short address = NscFetch16();
        NscPush16(gC7010.nsc.pc);
        gC7010.nsc.pc = address;
        break;
    }
    case 0xCE: gC7010.nsc.a = NscAdd8(gC7010.nsc.a, NscFetch8(), (gC7010.nsc.f & 1) ? 1 : 0); break;
    case 0xCF: NscRst(0x08); break;
    case 0xD0: NscConditionalRet(2); break;
    case 0xD1: { unsigned short v = NscPop16(); NscSetDE(v); break; }
    case 0xD2: NscConditionalJump(2); break;
    case 0xD3: { const uint8_t p = NscFetch8(); C7010_NSC800_Out(p, gC7010.nsc.a); break; }
    case 0xD4: NscConditionalCall(2); break;
    case 0xD5: NscPush16(NscGetDE()); break;
    case 0xD6: gC7010.nsc.a = NscSub8(gC7010.nsc.a, NscFetch8(), 0); break;
    case 0xD7: NscRst(0x10); break;
    case 0xD8: NscConditionalRet(3); break;
    case 0xD9:
    {
        const uint8_t h = (uint8_t)(gC7010.nsc.hl >> 8), l = (uint8_t)gC7010.nsc.hl;
        const uint8_t b = gC7010.nsc.b, c = gC7010.nsc.c, d = gC7010.nsc.d, e = gC7010.nsc.e;
        gC7010.nsc.b = gC7010.nsc.b2;
        gC7010.nsc.c = gC7010.nsc.c2;
        gC7010.nsc.d = gC7010.nsc.d2;
        gC7010.nsc.e = gC7010.nsc.e2;
        gC7010.nsc.hl = (unsigned short)((gC7010.nsc.h2 << 8) | gC7010.nsc.l2);
        gC7010.nsc.b2 = b;
        gC7010.nsc.c2 = c;
        gC7010.nsc.d2 = d;
        gC7010.nsc.e2 = e;
        gC7010.nsc.h2 = h;
        gC7010.nsc.l2 = l;
        break;
    }
    case 0xDA: NscConditionalJump(3); break;
    case 0xDB: { const uint8_t p = NscFetch8(); gC7010.nsc.a = C7010_NSC800_In(p); break; }
    case 0xDC: NscConditionalCall(3); break;
    case 0xDD: (void)NscExecuteIndexed(pc0, &gC7010.nsc.ix, "DD"); break;
    case 0xDE: gC7010.nsc.a = NscSub8(gC7010.nsc.a, NscFetch8(), (gC7010.nsc.f & 1) ? 1 : 0); break;
    case 0xDF: NscRst(0x18); break;
    case 0xE0: NscConditionalRet(4); break;
    case 0xE1: gC7010.nsc.hl = NscPop16(); break;
    case 0xE2: NscConditionalJump(4); break;
    case 0xE3:
    {
        const unsigned short v = NscPop16();
        NscPush16(gC7010.nsc.hl);
        gC7010.nsc.hl = v;
        break;
    }
    case 0xE4: NscConditionalCall(4); break;
    case 0xE5: NscPush16(gC7010.nsc.hl); break;
    case 0xE6:
        gC7010.nsc.a = (uint8_t)(gC7010.nsc.a & NscFetch8());
        NscSetLogicFlags(gC7010.nsc.a, true);
        break;
    case 0xE7: NscRst(0x20); break;
    case 0xE8: NscConditionalRet(5); break;
    case 0xE9: gC7010.nsc.pc = gC7010.nsc.hl; break;
    case 0xEA: NscConditionalJump(5); break;
    case 0xED: (void)NscExecuteED(pc0); break;
    case 0xEB:
    {
        const unsigned short v = NscGetDE();
        NscSetDE(gC7010.nsc.hl);
        gC7010.nsc.hl = v;
        break;
    }
    case 0xEC: NscConditionalCall(5); break;
    case 0xEE:
        gC7010.nsc.a = (uint8_t)(gC7010.nsc.a ^ NscFetch8());
        NscSetLogicFlags(gC7010.nsc.a, false);
        break;
    case 0xEF: NscRst(0x28); break;
    case 0xF0: NscConditionalRet(6); break;
    case 0xF1:
    {
        const unsigned short v = NscPop16();
        gC7010.nsc.a = (uint8_t)(v >> 8);
        gC7010.nsc.f = (uint8_t)v;
        break;
    }
    case 0xF2: NscConditionalJump(6); break;
    case 0xF3:
        gC7010.nsc.iff1 = false;
        gC7010.nsc.iff2 = false;
        break;
    case 0xF4: NscConditionalCall(6); break;
    case 0xF5: NscPush16((unsigned short)((gC7010.nsc.a << 8) | gC7010.nsc.f)); break;
    case 0xF6:
        gC7010.nsc.a = (uint8_t)(gC7010.nsc.a | NscFetch8());
        NscSetLogicFlags(gC7010.nsc.a, false);
        break;
    case 0xF7: NscRst(0x30); break;
    case 0xF8: NscConditionalRet(7); break;
    case 0xF9: gC7010.nsc.sp = gC7010.nsc.hl; break;
    case 0xFA: NscConditionalJump(7); break;
    case 0xFB:
        gC7010.nsc.iff1 = true;
        gC7010.nsc.iff2 = true;
        break;
    case 0xFC: NscConditionalCall(7); break;
    case 0xFD: (void)NscExecuteIndexed(pc0, &gC7010.nsc.iy, "FD"); break;
    case 0xFE: (void)NscSub8(gC7010.nsc.a, NscFetch8(), 0); break;
    case 0xFF: NscRst(0x38); break;
    default:
        if (!NscExecuteRegisterMatrix(op))
        {
            C7010_Log("C7010: NSC800 unsupported OP=%02X at PC=%04X\n", op, pc0);
            gC7010.nsc.stopped = true;
        }
        break;
    }
}

void C7010_Run8048Cycles(unsigned int cycles, unsigned int frameCycles)
{
    if (!gC7010.enabled || !gC7010.firmware_loaded || gC7010.cpu_reset || frameCycles == 0)
    {
        gC7010.interleave_credit = 0;
        return;
    }
    gC7010.interleave_credit += (unsigned long)cycles * 10000UL;
    while (gC7010.interleave_credit >= frameCycles)
    {
        gC7010.interleave_credit -= frameCycles;
        C7010_ExecuteOneStep();
        if (gC7010.nsc.stopped)
        {
            gC7010.interleave_credit = 0;
            break;
        }
    }
}

/*
 * ---------------------------------------------------------------------------
 * Portable save-state block ("C7S1" version 1.0)
 *
 * All fields are written explicitly in little-endian order. The internal
 * C/C++ structures are never memcpy'd because their layouts differ (the C
 * struct has unused h/l fields) and because bool/padding/endian are ABI
 * dependent.
 * ---------------------------------------------------------------------------
 */

#define C7010_STATE_MAGIC          0x31533743u /* bytes 'C','7','S','1' */
#define C7010_STATE_VERSION_MAJOR  1u
#define C7010_STATE_VERSION_MINOR  0u
#define C7010_STATE_HEADER_SIZE    20u
#define C7010_STATE_PAYLOAD_SIZE   2142u
#define C7010_STATE_TOTAL_SIZE     \
    (C7010_STATE_HEADER_SIZE + C7010_STATE_PAYLOAD_SIZE + 4u)

typedef struct
{
    const uint8_t *data;
    size_t size;
    size_t pos;
    bool ok;
} C7010StateReader;

typedef struct
{
    uint8_t enabled, firmware_loaded, cpu_reset, execution_needs_reset;
    uint8_t initialized, stopped;
    uint16_t pc, sp, hl, ix, iy;
    uint8_t a, b, c, d, e, f;
    uint8_t a2, f2, b2, c2, d2, e2, h2, l2;
    uint8_t i, r, interrupt_mode;
    uint8_t iff1, iff2;
    uint8_t p1, to_nsc800, to_8048;
    uint64_t to_nsc_write_generation;
    uint64_t to_nsc_read_generation;
    uint64_t to_8048_write_generation;
    uint64_t to_8048_read_generation;
    uint64_t interleave_credit;
    uint32_t cartridge_crc;
    uint32_t firmware_size;
    uint64_t firmware_fnv1a;
    uint8_t ram[C7010_RAM_SIZE];
} C7010DecodedState;

static void C7010_WriteU8(uint8_t *data, size_t *pos, uint8_t value)
{
    data[(*pos)++] = value;
}

static void C7010_WriteU16(uint8_t *data, size_t *pos, uint16_t value)
{
    data[(*pos)++] = (uint8_t)(value & 0xFFu);
    data[(*pos)++] = (uint8_t)(value >> 8);
}

static void C7010_WriteU32(uint8_t *data, size_t *pos, uint32_t value)
{
    data[(*pos)++] = (uint8_t)(value & 0xFFu);
    data[(*pos)++] = (uint8_t)((value >> 8) & 0xFFu);
    data[(*pos)++] = (uint8_t)((value >> 16) & 0xFFu);
    data[(*pos)++] = (uint8_t)((value >> 24) & 0xFFu);
}

static void C7010_WriteU64(uint8_t *data, size_t *pos, uint64_t value)
{
    C7010_WriteU32(data, pos, (uint32_t)(value & 0xFFFFFFFFu));
    C7010_WriteU32(data, pos, (uint32_t)(value >> 32));
}

static uint8_t C7010_ReadU8(C7010StateReader *r)
{
    if (!r->ok || r->pos + 1u > r->size)
    {
        r->ok = false;
        return 0;
    }
    return r->data[r->pos++];
}

static uint16_t C7010_ReadU16(C7010StateReader *r)
{
    uint16_t value;
    if (!r->ok || r->pos + 2u > r->size)
    {
        r->ok = false;
        return 0;
    }
    value = (uint16_t)((uint16_t)r->data[r->pos] |
        ((uint16_t)r->data[r->pos + 1] << 8));
    r->pos += 2u;
    return value;
}

static uint32_t C7010_ReadU32(C7010StateReader *r)
{
    uint32_t value;
    if (!r->ok || r->pos + 4u > r->size)
    {
        r->ok = false;
        return 0;
    }
    value = (uint32_t)r->data[r->pos] |
        ((uint32_t)r->data[r->pos + 1] << 8) |
        ((uint32_t)r->data[r->pos + 2] << 16) |
        ((uint32_t)r->data[r->pos + 3] << 24);
    r->pos += 4u;
    return value;
}

static uint64_t C7010_ReadU64(C7010StateReader *r)
{
    uint64_t lo = (uint64_t)C7010_ReadU32(r);
    uint64_t hi = (uint64_t)C7010_ReadU32(r);
    return lo | (hi << 32);
}

static uint32_t C7010_StateCrc32(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    unsigned int bit;

    for (i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8u; ++bit)
        {
            if (crc & 1u)
                crc = (crc >> 1) ^ 0xEDB88320u;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

static uint64_t C7010_StateFnv1a64(const uint8_t *data, size_t size)
{
    uint64_t hash = 1469598103934665603ULL;
    size_t i;

    for (i = 0; i < size; ++i)
    {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static C7010Nsc800 C7010_StateCanonicalNsc(void)
{
    C7010Nsc800 nsc;

    if (!gC7010.nsc.initialized || gC7010.execution_needs_reset)
    {
        memset(&nsc, 0, sizeof(nsc));
        nsc.sp = 0xFFFF;
    }
    else
    {
        nsc = gC7010.nsc;
    }
    return nsc;
}

size_t C7010_StateSize(void)
{
    return (size_t)C7010_STATE_TOTAL_SIZE;
}

bool C7010_StateSave(uint8_t *data, size_t size)
{
    C7010Nsc800 nsc;
    size_t pos = 0;
    uint32_t crc;

    if (!data || size < C7010_STATE_TOTAL_SIZE)
        return false;

    nsc = C7010_StateCanonicalNsc();

    C7010_WriteU32(data, &pos, C7010_STATE_MAGIC);
    C7010_WriteU16(data, &pos, C7010_STATE_VERSION_MAJOR);
    C7010_WriteU16(data, &pos, C7010_STATE_VERSION_MINOR);
    C7010_WriteU32(data, &pos, C7010_STATE_PAYLOAD_SIZE);
    C7010_WriteU32(data, &pos, 0u); /* required_flags */
    C7010_WriteU32(data, &pos, 0u); /* optional_flags */

    C7010_WriteU8(data, &pos, gC7010.enabled ? 1u : 0u);
    C7010_WriteU8(data, &pos, gC7010.firmware_loaded ? 1u : 0u);
    C7010_WriteU8(data, &pos, gC7010.cpu_reset ? 1u : 0u);
    C7010_WriteU8(data, &pos, gC7010.execution_needs_reset ? 1u : 0u);
    C7010_WriteU8(data, &pos, nsc.initialized ? 1u : 0u);
    C7010_WriteU8(data, &pos, nsc.stopped ? 1u : 0u);

    C7010_WriteU16(data, &pos, nsc.pc);
    C7010_WriteU16(data, &pos, nsc.sp);
    C7010_WriteU16(data, &pos, nsc.hl);
    C7010_WriteU16(data, &pos, nsc.ix);
    C7010_WriteU16(data, &pos, nsc.iy);

    C7010_WriteU8(data, &pos, nsc.a);
    C7010_WriteU8(data, &pos, nsc.b);
    C7010_WriteU8(data, &pos, nsc.c);
    C7010_WriteU8(data, &pos, nsc.d);
    C7010_WriteU8(data, &pos, nsc.e);
    C7010_WriteU8(data, &pos, nsc.f);

    C7010_WriteU8(data, &pos, nsc.a2);
    C7010_WriteU8(data, &pos, nsc.f2);
    C7010_WriteU8(data, &pos, nsc.b2);
    C7010_WriteU8(data, &pos, nsc.c2);
    C7010_WriteU8(data, &pos, nsc.d2);
    C7010_WriteU8(data, &pos, nsc.e2);
    C7010_WriteU8(data, &pos, nsc.h2);
    C7010_WriteU8(data, &pos, nsc.l2);

    C7010_WriteU8(data, &pos, nsc.i);
    C7010_WriteU8(data, &pos, nsc.r);
    C7010_WriteU8(data, &pos, nsc.interrupt_mode);
    C7010_WriteU8(data, &pos, nsc.iff1 ? 1u : 0u);
    C7010_WriteU8(data, &pos, nsc.iff2 ? 1u : 0u);

    C7010_WriteU8(data, &pos, gC7010.p1);
    C7010_WriteU8(data, &pos, gC7010.to_nsc800);
    C7010_WriteU8(data, &pos, gC7010.to_8048);

    C7010_WriteU64(data, &pos, (uint64_t)gC7010.to_nsc_write_generation);
    C7010_WriteU64(data, &pos, (uint64_t)gC7010.to_nsc_read_generation);
    C7010_WriteU64(data, &pos, (uint64_t)gC7010.to_8048_write_generation);
    C7010_WriteU64(data, &pos, (uint64_t)gC7010.to_8048_read_generation);
    C7010_WriteU64(data, &pos, (uint64_t)gC7010.interleave_credit);

    C7010_WriteU32(data, &pos, gC7010.cartridge_crc);
    C7010_WriteU32(data, &pos, (uint32_t)C7010_FIRMWARE_SIZE);
    C7010_WriteU64(data, &pos,
        C7010_StateFnv1a64(gC7010.firmware, C7010_FIRMWARE_SIZE));

    memcpy(data + pos, gC7010.ram, C7010_RAM_SIZE);
    pos += C7010_RAM_SIZE;

    crc = C7010_StateCrc32(data, pos);
    C7010_WriteU32(data, &pos, crc);

    return pos == C7010_STATE_TOTAL_SIZE;
}

static bool C7010_StateParse(const uint8_t *data, size_t size,
                             C7010DecodedState *decoded)
{
    C7010StateReader reader;
    uint32_t magic;
    uint16_t major;
    uint16_t minor;
    uint32_t payload_size;
    uint32_t required_flags;
    uint32_t optional_flags;
    uint32_t stored_crc;
    uint32_t calculated_crc;
    size_t payload_end;

    if (!data || !decoded || size < C7010_STATE_TOTAL_SIZE)
        return false;

    memset(decoded, 0, sizeof(*decoded));
    reader.data = data;
    reader.size = size;
    reader.pos = 0;
    reader.ok = true;

    magic = C7010_ReadU32(&reader);
    major = C7010_ReadU16(&reader);
    minor = C7010_ReadU16(&reader);
    payload_size = C7010_ReadU32(&reader);
    required_flags = C7010_ReadU32(&reader);
    optional_flags = C7010_ReadU32(&reader);
    (void)optional_flags;

    if (!reader.ok ||
        magic != C7010_STATE_MAGIC ||
        major != C7010_STATE_VERSION_MAJOR ||
        minor > C7010_STATE_VERSION_MINOR ||
        payload_size != C7010_STATE_PAYLOAD_SIZE ||
        required_flags != 0u)
        return false;

    payload_end = C7010_STATE_HEADER_SIZE + (size_t)payload_size;
    if (payload_end + 4u > size)
        return false;
    reader.size = payload_end;

    decoded->enabled = C7010_ReadU8(&reader);
    decoded->firmware_loaded = C7010_ReadU8(&reader);
    decoded->cpu_reset = C7010_ReadU8(&reader);
    decoded->execution_needs_reset = C7010_ReadU8(&reader);
    decoded->initialized = C7010_ReadU8(&reader);
    decoded->stopped = C7010_ReadU8(&reader);

    decoded->pc = C7010_ReadU16(&reader);
    decoded->sp = C7010_ReadU16(&reader);
    decoded->hl = C7010_ReadU16(&reader);
    decoded->ix = C7010_ReadU16(&reader);
    decoded->iy = C7010_ReadU16(&reader);

    decoded->a = C7010_ReadU8(&reader);
    decoded->b = C7010_ReadU8(&reader);
    decoded->c = C7010_ReadU8(&reader);
    decoded->d = C7010_ReadU8(&reader);
    decoded->e = C7010_ReadU8(&reader);
    decoded->f = C7010_ReadU8(&reader);

    decoded->a2 = C7010_ReadU8(&reader);
    decoded->f2 = C7010_ReadU8(&reader);
    decoded->b2 = C7010_ReadU8(&reader);
    decoded->c2 = C7010_ReadU8(&reader);
    decoded->d2 = C7010_ReadU8(&reader);
    decoded->e2 = C7010_ReadU8(&reader);
    decoded->h2 = C7010_ReadU8(&reader);
    decoded->l2 = C7010_ReadU8(&reader);

    decoded->i = C7010_ReadU8(&reader);
    decoded->r = C7010_ReadU8(&reader);
    decoded->interrupt_mode = C7010_ReadU8(&reader);
    decoded->iff1 = C7010_ReadU8(&reader);
    decoded->iff2 = C7010_ReadU8(&reader);

    decoded->p1 = C7010_ReadU8(&reader);
    decoded->to_nsc800 = C7010_ReadU8(&reader);
    decoded->to_8048 = C7010_ReadU8(&reader);

    decoded->to_nsc_write_generation = C7010_ReadU64(&reader);
    decoded->to_nsc_read_generation = C7010_ReadU64(&reader);
    decoded->to_8048_write_generation = C7010_ReadU64(&reader);
    decoded->to_8048_read_generation = C7010_ReadU64(&reader);
    decoded->interleave_credit = C7010_ReadU64(&reader);

    decoded->cartridge_crc = C7010_ReadU32(&reader);
    decoded->firmware_size = C7010_ReadU32(&reader);
    decoded->firmware_fnv1a = C7010_ReadU64(&reader);

    if (!reader.ok || reader.pos + C7010_RAM_SIZE > reader.size)
        return false;
    memcpy(decoded->ram, reader.data + reader.pos, C7010_RAM_SIZE);
    reader.pos += C7010_RAM_SIZE;

    if (!reader.ok || reader.pos != payload_end)
        return false;

    stored_crc = (uint32_t)data[payload_end] |
        ((uint32_t)data[payload_end + 1] << 8) |
        ((uint32_t)data[payload_end + 2] << 16) |
        ((uint32_t)data[payload_end + 3] << 24);
    calculated_crc = C7010_StateCrc32(data, payload_end);
    if (stored_crc != calculated_crc)
        return false;

    if (decoded->enabled > 1u || decoded->firmware_loaded > 1u ||
        decoded->cpu_reset > 1u || decoded->execution_needs_reset > 1u ||
        decoded->initialized > 1u || decoded->stopped > 1u ||
        decoded->iff1 > 1u || decoded->iff2 > 1u ||
        decoded->interrupt_mode > 2u)
        return false;

    if (decoded->firmware_size != C7010_FIRMWARE_SIZE)
        return false;
    if (decoded->firmware_fnv1a !=
        C7010_StateFnv1a64(gC7010.firmware, C7010_FIRMWARE_SIZE))
        return false;

    if (decoded->enabled != (gC7010.enabled ? 1u : 0u) ||
        decoded->firmware_loaded != (gC7010.firmware_loaded ? 1u : 0u) ||
        decoded->cartridge_crc != gC7010.cartridge_crc)
        return false;

    return true;
}

bool C7010_StateValidate(const uint8_t *data, size_t size)
{
    C7010DecodedState decoded;

    return C7010_StateParse(data, size, &decoded);
}

bool C7010_StateLoad(const uint8_t *data, size_t size)
{
    C7010DecodedState decoded;

    if (!C7010_StateParse(data, size, &decoded))
        return false;

    gC7010.cpu_reset = decoded.cpu_reset != 0;
    gC7010.execution_needs_reset = decoded.execution_needs_reset != 0;
    gC7010.p1 = decoded.p1;
    gC7010.to_nsc800 = decoded.to_nsc800;
    gC7010.to_8048 = decoded.to_8048;
    gC7010.to_nsc_write_generation =
        (unsigned long)decoded.to_nsc_write_generation;
    gC7010.to_nsc_read_generation =
        (unsigned long)decoded.to_nsc_read_generation;
    gC7010.to_8048_write_generation =
        (unsigned long)decoded.to_8048_write_generation;
    gC7010.to_8048_read_generation =
        (unsigned long)decoded.to_8048_read_generation;
    gC7010.interleave_credit = (unsigned long)decoded.interleave_credit;

    gC7010.nsc.pc = decoded.pc;
    gC7010.nsc.sp = decoded.sp;
    gC7010.nsc.hl = decoded.hl;
    gC7010.nsc.ix = decoded.ix;
    gC7010.nsc.iy = decoded.iy;
    gC7010.nsc.a = decoded.a;
    gC7010.nsc.b = decoded.b;
    gC7010.nsc.c = decoded.c;
    gC7010.nsc.d = decoded.d;
    gC7010.nsc.e = decoded.e;
    gC7010.nsc.f = decoded.f;
    gC7010.nsc.h = (uint8_t)(decoded.hl >> 8);
    gC7010.nsc.l = (uint8_t)decoded.hl;
    gC7010.nsc.a2 = decoded.a2;
    gC7010.nsc.f2 = decoded.f2;
    gC7010.nsc.b2 = decoded.b2;
    gC7010.nsc.c2 = decoded.c2;
    gC7010.nsc.d2 = decoded.d2;
    gC7010.nsc.e2 = decoded.e2;
    gC7010.nsc.h2 = decoded.h2;
    gC7010.nsc.l2 = decoded.l2;
    gC7010.nsc.i = decoded.i;
    gC7010.nsc.r = decoded.r;
    gC7010.nsc.interrupt_mode = decoded.interrupt_mode;
    gC7010.nsc.iff1 = decoded.iff1 != 0;
    gC7010.nsc.iff2 = decoded.iff2 != 0;
    gC7010.nsc.initialized = decoded.initialized != 0;
    gC7010.nsc.stopped = decoded.stopped != 0;

    memcpy(gC7010.ram, decoded.ram, C7010_RAM_SIZE);
    return true;
}
