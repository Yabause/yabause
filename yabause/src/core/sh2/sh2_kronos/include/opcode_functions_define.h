/*  Copyright 2018 Francois Caron

    This file is part of Kronos.

    Kronos is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kronos is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kronos; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#ifndef OPCODE_FUNCTIONS_DEFINE_H
#define OPCODE_FUNCTIONS_DEFINE_H

#define func_a_b_c_d(i, f, a, b, c, d) \
static void FASTCALL call_ ## a ## _ ## b ## _ ## c ## _ ## d(SH2_struct* sh) \
{ \
  func ## i(f, a, b, c, d); \
}

#define func_a_b_d(i, f, a, b, d) \
func_a_b_c_d(i, f, a, b, 0, d) \
func_a_b_c_d(i, f, a, b, 1, d) \
func_a_b_c_d(i, f, a, b, 2, d) \
func_a_b_c_d(i, f, a, b, 3, d) \
func_a_b_c_d(i, f, a, b, 4, d) \
func_a_b_c_d(i, f, a, b, 5, d) \
func_a_b_c_d(i, f, a, b, 6, d) \
func_a_b_c_d(i, f, a, b, 7, d) \
func_a_b_c_d(i, f, a, b, 8, d) \
func_a_b_c_d(i, f, a, b, 9, d) \
func_a_b_c_d(i, f, a, b, 10, d) \
func_a_b_c_d(i, f, a, b, 11, d) \
func_a_b_c_d(i, f, a, b, 12, d) \
func_a_b_c_d(i, f, a, b, 13, d) \
func_a_b_c_d(i, f, a, b, 14, d) \
func_a_b_c_d(i, f, a, b, 15, d) \

#define func_a_b(i, f, a, b) \
func_a_b_d(i, f, a, b, 0) \
func_a_b_d(i, f, a, b, 1) \
func_a_b_d(i, f, a, b, 2) \
func_a_b_d(i, f, a, b, 3) \
func_a_b_d(i, f, a, b, 4) \
func_a_b_d(i, f, a, b, 5) \
func_a_b_d(i, f, a, b, 6) \
func_a_b_d(i, f, a, b, 7) \
func_a_b_d(i, f, a, b, 8) \
func_a_b_d(i, f, a, b, 9) \
func_a_b_d(i, f, a, b, 10) \
func_a_b_d(i, f, a, b, 11) \
func_a_b_d(i, f, a, b, 12) \
func_a_b_d(i, f, a, b, 13) \
func_a_b_d(i, f, a, b, 14) \
func_a_b_d(i, f, a, b, 15) 

//FuncAD
#define func2(f, a, b, c, d) \
   f(sh, b, c)

#define func_AD(f, A, D) \
func_a_b_d(2, f, A, 0, D) \
func_a_b_d(2, f, A, 1, D) \
func_a_b_d(2, f, A, 2, D) \
func_a_b_d(2, f, A, 3, D) \
func_a_b_d(2, f, A, 4, D) \
func_a_b_d(2, f, A, 5, D) \
func_a_b_d(2, f, A, 6, D) \
func_a_b_d(2, f, A, 7, D) \
func_a_b_d(2, f, A, 8, D) \
func_a_b_d(2, f, A, 9, D) \
func_a_b_d(2, f, A, 10, D) \
func_a_b_d(2, f, A, 11, D) \
func_a_b_d(2, f, A, 12, D) \
func_a_b_d(2, f, A, 13, D) \
func_a_b_d(2, f, A, 14, D) \
func_a_b_d(2, f, A, 15, D)

//FuncACD
#define func3(f, a, b, c, d) \
   f(sh, b)

#define func_ACD(f, A, C, D) \
func_a_b_c_d(3, f, A, 0, C, D) \
func_a_b_c_d(3, f, A, 1, C, D) \
func_a_b_c_d(3, f, A, 2, C, D) \
func_a_b_c_d(3, f, A, 3, C, D) \
func_a_b_c_d(3, f, A, 4, C, D) \
func_a_b_c_d(3, f, A, 5, C, D) \
func_a_b_c_d(3, f, A, 6, C, D) \
func_a_b_c_d(3, f, A, 7, C, D) \
func_a_b_c_d(3, f, A, 8, C, D) \
func_a_b_c_d(3, f, A, 9, C, D) \
func_a_b_c_d(3, f, A, 10, C, D) \
func_a_b_c_d(3, f, A, 11, C, D) \
func_a_b_c_d(3, f, A, 12, C, D) \
func_a_b_c_d(3, f, A, 13, C, D) \
func_a_b_c_d(3, f, A, 14, C, D) \
func_a_b_c_d(3, f, A, 15, C, D)

//FuncAB
#define func1(f, a, b, c, d) \
   f(sh, (c<<4)+d)

#define func_AB(f, A, B) \
func_a_b_d(1, f, A, B, 0) \
func_a_b_d(1, f, A, B, 1) \
func_a_b_d(1, f, A, B, 2) \
func_a_b_d(1, f, A, B, 3) \
func_a_b_d(1, f, A, B, 4) \
func_a_b_d(1, f, A, B, 5) \
func_a_b_d(1, f, A, B, 6) \
func_a_b_d(1, f, A, B, 7) \
func_a_b_d(1, f, A, B, 8) \
func_a_b_d(1, f, A, B, 9) \
func_a_b_d(1, f, A, B, 10) \
func_a_b_d(1, f, A, B, 11) \
func_a_b_d(1, f, A, B, 12) \
func_a_b_d(1, f, A, B, 13) \
func_a_b_d(1, f, A, B, 14) \
func_a_b_d(1, f, A, B, 15) \

//FuncAB
#define func5(f, a, b, c, d) \
   f(sh,c,d)

#define func_AB_n_d(f, A, B) \
func_a_b_d(5, f, A, B, 0) \
func_a_b_d(5, f, A, B, 1) \
func_a_b_d(5, f, A, B, 2) \
func_a_b_d(5, f, A, B, 3) \
func_a_b_d(5, f, A, B, 4) \
func_a_b_d(5, f, A, B, 5) \
func_a_b_d(5, f, A, B, 6) \
func_a_b_d(5, f, A, B, 7) \
func_a_b_d(5, f, A, B, 8) \
func_a_b_d(5, f, A, B, 9) \
func_a_b_d(5, f, A, B, 10) \
func_a_b_d(5, f, A, B, 11) \
func_a_b_d(5, f, A, B, 12) \
func_a_b_d(5, f, A, B, 13) \
func_a_b_d(5, f, A, B, 14) \
func_a_b_d(5, f, A, B, 15) \

//FuncA
#define func0(f, a, b, c, d) \
   f(sh, b, (c<<4)+d)

#define func_A(f, A) \
func_a_b(0, f, A, 0) \
func_a_b(0, f, A, 1) \
func_a_b(0, f, A, 2) \
func_a_b(0, f, A, 3) \
func_a_b(0, f, A, 4) \
func_a_b(0, f, A, 5) \
func_a_b(0, f, A, 6) \
func_a_b(0, f, A, 7) \
func_a_b(0, f, A, 8) \
func_a_b(0, f, A, 9) \
func_a_b(0, f, A, 10) \
func_a_b(0, f, A, 11) \
func_a_b(0, f, A, 12) \
func_a_b(0, f, A, 13) \
func_a_b(0, f, A, 14) \
func_a_b(0, f, A, 15) \

//FuncA_disp
#define func4(f, a, b, c, d) \
   f(sh, (b<<8)+(c<<4)+d)

#define func_A_disp(f, A) \
func_a_b(4, f, A, 0) \
func_a_b(4, f, A, 1) \
func_a_b(4, f, A, 2) \
func_a_b(4, f, A, 3) \
func_a_b(4, f, A, 4) \
func_a_b(4, f, A, 5) \
func_a_b(4, f, A, 6) \
func_a_b(4, f, A, 7) \
func_a_b(4, f, A, 8) \
func_a_b(4, f, A, 9) \
func_a_b(4, f, A, 10) \
func_a_b(4, f, A, 11) \
func_a_b(4, f, A, 12) \
func_a_b(4, f, A, 13) \
func_a_b(4, f, A, 14) \
func_a_b(4, f, A, 15) \

#define func6(f, a, b, c, d) \
   f(sh, b, c, d)

#define func_A_NM_DISP(f, A) \
func_a_b(6, f, A, 0) \
func_a_b(6, f, A, 1) \
func_a_b(6, f, A, 2) \
func_a_b(6, f, A, 3) \
func_a_b(6, f, A, 4) \
func_a_b(6, f, A, 5) \
func_a_b(6, f, A, 6) \
func_a_b(6, f, A, 7) \
func_a_b(6, f, A, 8) \
func_a_b(6, f, A, 9) \
func_a_b(6, f, A, 10) \
func_a_b(6, f, A, 11) \
func_a_b(6, f, A, 12) \
func_a_b(6, f, A, 13) \
func_a_b(6, f, A, 14) \
func_a_b(6, f, A, 15) \

//FuncABCD
#define func7(f, a, b, c, d) \
   f(sh)

#define func_ABCD(f, A, B, C, D) \
func_a_b_c_d(7, f, A, B, C, D)

#endif
