# Source provenance and licensing review

The contribution retains the repository's Artistic License 2.0 (`COPYING`)
and existing O2EM/libretro copyright notices. The changes are documented and
offered to the upstream maintainers under that license (source redistribution,
section 4(a)). No replacement license is asserted for existing code.

C7010's interpreter and integration are the implementation developed by
Bengt-Ove Peltz / O2EM-NG, as credited in their source headers and
`docs/C7010.md`; no external Z80 interpreter library is imported.
The Videopac+ changes modify the existing O2EM `src/vpp.c` directly: cached
layer initialization and handling of the missing border samples through
O2EM's existing transparency lookup. The new test exercises that same code.
No MAME source blocks, algorithms translated line-for-line, character ROMs,
device classes, or lookup tables were imported. MAME was consulted to check
hardware behavior, alongside the Thomson-EFCIS EF9340/EF9341 datasheet.

MAME reference snapshot: `a7230f9f3b6b29b491fb2d4a8e44f1452c5ecc3a`.
The inspected EF9340/EF9341 and Odyssey2/G7400 source files identify
`BSD-3-Clause` individually; this review does not substitute MAME's aggregate
project licensing for those per-file licenses.

Reference credits, without implying endorsement:

- `src/devices/video/ef9340_1.cpp`: Wilbert Pol, hap.
- `src/mame/philips/odyssey2.cpp`: Peter Trauner, Wilbert Pol, hap.
- C7010's earlier PAL reference is described separately in `docs/C7010.md`.
  `src/devices/video/i8244.cpp` also identifies BSD-3-Clause and credits
  Peter Trauner, Wilbert Pol, hap (with thanks to Kevin Horton).

Fixed source links:

- https://github.com/mamedev/mame/blob/a7230f9f3b6b29b491fb2d4a8e44f1452c5ecc3a/src/devices/video/ef9340_1.cpp
- https://github.com/mamedev/mame/blob/a7230f9f3b6b29b491fb2d4a8e44f1452c5ecc3a/src/mame/philips/odyssey2.cpp
- https://github.com/mamedev/mame/blob/a7230f9f3b6b29b491fb2d4a8e44f1452c5ecc3a/src/devices/video/i8244.cpp
- https://www.retronik.fr/DOCUMENTS/Composants/THOMSON-EFCIS/1981_THOMSON-EFCIS_EF9340-9341.pdf

No MAME code redistribution is claimed or needed for this independently
written change. For clarity and conservative attribution, the BSD-3-Clause
terms applicable to the cited files are reproduced below. They do not
relicense O2EM or this repository.

## BSD-3-Clause reference notice

Copyright (c) Peter Trauner, Wilbert Pol, hap.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

## Excluded assets

No cartridge ROM, BIOS, module firmware, save-state memory image, or compiled
DLL is included in this publication. Small before/after framebuffer captures
are supplied only as visual evidence of the reported rendering defects;
their inclusion does not assert ownership or a source-code license over the
depicted game artwork. Existing unrelated repository assets are unchanged.
