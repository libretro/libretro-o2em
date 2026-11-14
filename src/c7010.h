/* SPDX-License-Identifier: Artistic-2.0 */
/* Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG. */
#ifndef C7010_H
#define C7010_H

#include <stddef.h>
#include <stdint.h>
#include <boolean.h>



/* Portable C7010 hardware and save-state interface. */


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*C7010_LogFn)(const char *message);

void C7010_SetLogCallback(C7010_LogFn fn);

bool C7010_IsEnabled(void);
bool C7010_IsActive(void);

bool C7010_ConfigureForCartridge(uint32_t cartridgeCrc);
bool C7010_LoadFirmware(const char *path);
bool C7010_LoadFirmwareData(const uint8_t *data, size_t size);

void C7010_Reset(void);
void C7010_Unload(void);

void C7010_WriteP1(uint8_t value);
void C7010_Run8048Cycles(unsigned int cycles, unsigned int frameCycles);

bool C7010_ExternalRead(uint8_t address, uint8_t *value);
bool C7010_ExternalWrite(uint8_t address, uint8_t value);

/*
 * Portable C7010 save-state block, format "C7S1" v1.0.
 *
 * The buffer is self-contained (header, payload, CRC32) and uses explicit
 * little-endian u8/u16/u32/u64 fields. Immutable firmware is identified by
 * size and FNV-1a 64, never copied. StateLoad validates the complete buffer
 * before mutating any emulation state.
 *
 * C7010_StateSize() is constant for a given format version.
 */
size_t C7010_StateSize(void);
bool C7010_StateSave(uint8_t *data, size_t size);
bool C7010_StateValidate(const uint8_t *data, size_t size);
bool C7010_StateLoad(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* C7010_H */
