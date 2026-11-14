/* SPDX-License-Identifier: Artistic-2.0
 * Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG.
 * Public-API state validation using generated data, not C7010 firmware.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c7010.h"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

static void quiet_log(const char *message) { (void)message; }

int main(void)
{
    unsigned char firmware[8192];
    unsigned char *saved, *roundtrip, *corrupt;
    size_t i, size;

    C7010_SetLogCallback(quiet_log);
    C7010_Unload();
    CHECK(!C7010_ConfigureForCartridge(0));
    CHECK(!C7010_IsActive());
    CHECK(C7010_ConfigureForCartridge(0x77066338u));
    CHECK(C7010_IsActive());
    for (i = 0; i < sizeof(firmware); ++i)
        firmware[i] = (unsigned char)(i * 37u + 11u);
    CHECK(!C7010_LoadFirmwareData(firmware, sizeof(firmware) - 1));
    CHECK(C7010_LoadFirmwareData(firmware, sizeof(firmware)));
    C7010_Reset();

    size = C7010_StateSize();
    saved = (unsigned char *)malloc(size);
    roundtrip = (unsigned char *)malloc(size);
    corrupt = (unsigned char *)malloc(size);
    CHECK(saved && roundtrip && corrupt);
    CHECK(C7010_StateSave(saved, size));
    CHECK(memcmp(saved, "C7S1", 4) == 0);
    CHECK(C7010_StateValidate(saved, size));
    CHECK(C7010_StateLoad(saved, size));
    CHECK(C7010_StateSave(roundtrip, size));
    CHECK(memcmp(saved, roundtrip, size) == 0);

    memcpy(corrupt, saved, size);
    corrupt[size - 1] ^= 1;
    CHECK(!C7010_StateValidate(corrupt, size));
    CHECK(!C7010_StateLoad(corrupt, size));
    CHECK(!C7010_StateLoad(saved, size - 1));
    CHECK(C7010_StateSave(roundtrip, size));
    CHECK(memcmp(saved, roundtrip, size) == 0);

    firmware[0] ^= 1;
    CHECK(C7010_LoadFirmwareData(firmware, sizeof(firmware)));
    CHECK(!C7010_StateValidate(saved, size));
    CHECK(!C7010_StateLoad(saved, size));
    firmware[0] ^= 1;
    CHECK(C7010_LoadFirmwareData(firmware, sizeof(firmware)));
    CHECK(C7010_StateLoad(saved, size));

    free(corrupt);
    free(roundtrip);
    free(saved);
    C7010_Unload();
    CHECK(!C7010_IsActive());
    puts("PASS C7010 detection, state round-trip, corruption/short-buffer rejection, firmware binding");
    return 0;
}
