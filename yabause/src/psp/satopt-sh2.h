/*  src/psp/satopt-sh2.h: Saturn-specific SH-2 optimization header
    Copyright 2009-2010 Andrew Church

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef PSP_SATOPT_SH2_H
#define PSP_SATOPT_SH2_H

/*************************************************************************/

/**
 * saturn_optimize_sh2:  Attempt to translate SH-2 code starting at the
 * given address into a hand-tuned RTL instruction stream.
 *
 * [Parameters]
 *       state: SH-2 processor state block
 *     address: Starting address of block
 *       fetch: Opcode fetch pointer corresponding to address
 *         rtl: RTLBlock into which to store translated code, if successful
 * [Return value]
 *     Length of block in 16-bit words (nonzero) if successfully translated,
 *     zero on error or if there is no suitable hand-tuned translation
 */
extern unsigned int saturn_optimize_sh2(SH2State *state, uint32_t address,
                                        const uint16_t *fetch, RTLBlock *rtl);

/*************************************************************************/

#endif  // PSP_SATOPT_SH2_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
