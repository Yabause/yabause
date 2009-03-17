/*  src/psp/sh2-core.i: SH-2 instruction decoding core for PSP
    Copyright 2009 Andrew Church

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

/*
 * This file is intended to be #included as part of a routine to decode
 * SH-2 instructions for interpretation or translation.  It expects the
 * following variables (which may in fact be macros) to be defined:
 *
 *     uint32_t cur_PC;  // Address of the instruction to be decoded
 *     int cur_cycles;   // Cycle accumulator
 *
 * Additionally, the following variables will be declared at the same block
 * level as the #include:
 *
 *     uint16_t opcode;     // Opcode read from the instruction stream
 *     int invalid_opcode;  // Nonzero if opcode was invalid, else zero
 *
 * Each instruction is decoded into a series of "micro-ops", which should
 * be defined as macros before this file is included.  See psp-sh2.c for
 * details on the micro-ops used and their meanings.
 */

/*************************************************************************/

uint16_t opcode;
int invalid_opcode = 0;

{

    /* Get a fetch pointer for the current PC, if we can */

    const uint16_t *fetch_base = (const uint16_t *)direct_pages[cur_PC >> 19];
    const uint16_t *fetch;
    if (fetch_base) {
        fetch = (const uint16_t *)((uintptr_t)fetch_base + (cur_PC & 0x7FFFF));
    } else {
        fetch = NULL;
    }

    /* Load the opcode to decode */

    if (LIKELY(fetch)) {
        opcode = *fetch++;
    } else {
        opcode = MappedMemoryReadWord(cur_PC);
    }

    /* Perform any early processing required */

    OPCODE_INIT(opcode);

    /* Clear the delay flag if this is a delay slot (this needs to be done
     * needs to be done here, not at the end of the previous iteration, so
     * (1) the translator knows not to interrupt the pair of instructions
     * and (2) the delay flag can be accessed by OPCODE_INIT() if needed) */

    if (state->psp.branch_type != SH2BRTYPE_NONE && state->delay) {
        state->delay = 0;
    }

    /* Increment the PC and cycle counter (any necessary additional cycles
     * are added by the individual opcode handlers) */

    INC_PC();
    cur_cycles++;

    /**** The Big Honkin' Opcode Switch Begins Here ****/

    /* Extract these early for convenience */
    int n = opcode>>8 & 0xF;
    int m = opcode>>4 & 0xF;

    /* Note: Mnemonics listed here for some instructions differ slightly
     * from the official Hitachi specs.  Here, register Rn _always_ refers
     * to the register specified by bits 8-11 of the opcode, and register
     * Rm _always_ refers to the register specified by bits 4-7 of the
     * opcode, regardless of the register's role (source or destination). */

    switch (opcode>>12 & 0xF) {

      case 0x0: {
        switch (opcode & 0xF) {

          case 0x2: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STC SR,Rn
                MOVE(REG_R(n), REG_SR);
                break;
              case 1:  // STC GBR,Rn
                MOVE(REG_R(n), REG_GBR);
                break;
              case 2:  // STC VBR,Rn
                STATE_LOAD(REG_R(n), regs.VBR);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx2

          case 0x3: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // BSRF Rn
                ADDI(REG_VAL0, REG_PC, 2);
                STATE_STORE(REG_VAL0, regs.PR);
                state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                ADD(REG_VAL0, REG_VAL0, REG_R(n));
                STATE_STORE(REG_VAL0, psp.branch_target);
                state->delay = 1;
                cur_cycles += 1;
                break;
              case 0x2:  // BRAF Rn
                state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                ADDI(REG_VAL0, REG_PC, 2);
                ADD(REG_VAL0, REG_VAL0, REG_R(n));
                STATE_STORE(REG_VAL0, psp.branch_target);
                state->delay = 1;
                cur_cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx3

          case 0x4:  // MOV.B Rm,@(R0,Rn)
            ADD(REG_VAL2, REG_R(0), REG_R(n));
            STORE_B(REG_R(m), REG_VAL2);
            break;

          case 0x5:  // MOV.W Rm,@(R0,Rn)
            ADD(REG_VAL2, REG_R(0), REG_R(n));
            STORE_W(REG_R(m), REG_VAL2);
            break;

          case 0x6:  // MOV.L Rm,@(R0,Rn)
            ADD(REG_VAL2, REG_R(0), REG_R(n));
            STORE_L(REG_R(m), REG_VAL2);
            break;

          case 0x7:  // MUL.L Rm,Rn
            MUL_START(REG_R(n), REG_R(m));
            MUL_END(REG_VAL0);
            STATE_STORE(REG_VAL0, regs.MACL);
            cur_cycles += 1;  // Minimum number of cycles
            break;

          case 0x8: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // CLRT
                INSBITS(REG_SR, REG_ZERO, SR_T_SHIFT, 1);
                break;
              case 0x1:  // SETT
                ORI(REG_SR, REG_SR, SR_T);
                break;
              case 0x2:  // CLRMAC
                STATE_STORE(REG_ZERO, regs.MACH);
                STATE_STORE(REG_ZERO, regs.MACL);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx8

          case 0x9: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // NOP
                /* No operation */
                break;
              case 0x1: {  // DIV0U
#ifdef OPTIMIZE_DIVISION
                int can_optimize = 0;
                int Rlo = -1, Rhi = -1, Rdiv = -1;   // Register numbers
                /* Only optimize when we can do direct fetching (otherwise
                 * our reads might have side effects) */
                if (LIKELY(fetch)) {
                    can_optimize = can_optimize_div0u(fetch, cur_PC,
                                                      &Rhi, &Rlo, &Rdiv);
                }
# ifndef DECODE_JIT
                /* When interpreting, we check immediately whether the high
                 * 32 bits are nonzero or the divisor is zero and skip
                 * optimization if so */
                if (can_optimize) {
                    can_optimize &= state->regs.R[Rhi] == 0;
                    can_optimize &= state->regs.R[Rdiv] != 0;
                }
# endif
                if (can_optimize) {
                    DIVU_START(REG_R(Rlo), REG_R(Rdiv));
# ifdef DECODE_JIT
                    /* When translating, we check the high 32 bits and
                     * divisor at runtime and skip over the optimized code
                     * for non-optimizable cases */
                    IFZ(0x00190, REG_R(Rhi),
                         MOVEI(REG_VAL0, ~(SR_T | SR_Q | SR_M)))
                      IFNZ(0x00190, REG_R(Rdiv), MOVEI(REG_VAL2, SR_Q)) {
# else
                        MOVEI(REG_VAL0, ~(SR_T | SR_Q | SR_M));
                        MOVEI(REG_VAL2, SR_Q);
# endif
# ifdef TRACE_ALL
                        state->psp.div_data.target_PC = cur_PC + 126;
                        state->psp.div_data.Rquo = Rlo;
                        state->psp.div_data.Rrem = Rhi;
                        /* Save the original values for restoration below */
                        STATE_STORE(REG_R(Rlo), psp.div_data.quo);
                        STATE_STORE(REG_R(Rhi), psp.div_data.rem);
                        STATE_STORE(REG_SR,     psp.div_data.SR);
# endif
                        AND(REG_SR, REG_SR, REG_VAL0);
                        DIV_END(REG_R(Rlo), REG_R(Rhi));
                        ANDI(REG_VAL1, REG_R(Rlo), 1);  // Low bit to T flag
                        SUB(REG_VAL0, REG_R(Rhi), REG_R(Rdiv));
                        SRLI(REG_R(Rlo), REG_R(Rlo), 1);
                        MOVZ(REG_R(Rhi), REG_VAL0, REG_VAL1);
                        MOVZ(REG_VAL1, REG_VAL2, REG_VAL1);
                        OR(REG_SR, REG_SR, REG_VAL1);
# ifdef TRACE_ALL
                        /* Save the result and restore the original values
                         * so we can trace the individual instructions */
                        MOVE(REG_VAL0, REG_R(Rlo));
                        MOVE(REG_VAL1, REG_R(Rhi));
                        MOVE(REG_VAL2, REG_SR);
                        STATE_LOAD(REG_R(Rlo), psp.div_data.quo);
                        STATE_LOAD(REG_R(Rhi), psp.div_data.rem);
                        STATE_LOAD(REG_SR,     psp.div_data.SR);
                        STATE_STORE(REG_VAL0,  psp.div_data.quo);
                        STATE_STORE(REG_VAL1,  psp.div_data.rem);
                        STATE_STORE(REG_VAL2,  psp.div_data.SR);
# else
                        ADDI(REG_VAL0, REG_PC, 128);
                        state->psp.branch_target = cur_PC+128;
#  ifdef DECODE_JIT
                        /* Skip over the step-by-step emulation */
                        int saved_cycles = cur_cycles;
                        cur_cycles += 64;
                        COMMIT_CYCLES(REG_VAL1);
                        FLUSH_REGS(~0);
                        BRANCH_STATIC(REG_VAL0);
                        cur_cycles = saved_cycles;
#  else
                        cur_cycles += 64;
                        BRANCH_STATIC(REG_VAL0);
                        break;
#  endif
# endif
# ifdef DECODE_JIT
                    } ENDIF(0x00190);
# endif
                }
#endif  // OPTIMIZE_DIVISION
                INSBITS(REG_SR, REG_ZERO, SR_Q_SHIFT, 2);
                INSBITS(REG_SR, REG_ZERO, SR_T_SHIFT, 1);
                break;
              }  // DIV0U
              case 0x2:  // MOVT Rn
                EXTBITS(REG_R(n), REG_SR, SR_T_SHIFT, 1);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx9

          case 0xA: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STS MACH,Rn
                STATE_LOAD(REG_R(n), regs.MACH);
                break;
              case 1:  // STS MACL,Rn
                STATE_LOAD(REG_R(n), regs.MACL);
                break;
              case 2:  // STS PR,Rn
                STATE_LOAD(REG_R(n), regs.PR);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xxA

          case 0xB: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // RTS
                state->psp.branch_type = SH2BRTYPE_RTS;
                STATE_LOAD(REG_VAL0, regs.PR);
                STATE_STORE(REG_VAL0, psp.branch_target);
                state->delay = 1;
                cur_cycles += 1;
                break;
              case 0x1:  // SLEEP
                MOVEI(REG_VAL0, 1);
                STATE_STOREB(REG_VAL0, isSleeping);
                cur_cycles += 2;
                break;
              case 0x2:  // RTE
                LOAD_L(REG_VAL0, REG_R(15));
                ADDI(REG_R(15), REG_R(15), 4);
                LOAD_L(REG_VAL1, REG_R(15));
                ADDI(REG_R(15), REG_R(15), 4);
                state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                STATE_STORE(REG_VAL0, psp.branch_target);
                ANDI(REG_SR, REG_VAL1, 0x3F3);
                MOVEI(REG_VAL0, 1);
                STATE_STORE(REG_VAL0, psp.need_interrupt_check);
                state->delay = 1;
                cur_cycles += 3;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xxB

          case 0xC:  // MOV.B @(R0,Rm),Rn
            ADD(REG_VAL2, REG_R(0), REG_R(m));
            LOAD_BS(REG_R(n), REG_VAL2);
            break;

          case 0xD:  // MOV.W @(R0,Rm),Rn
            ADD(REG_VAL2, REG_R(0), REG_R(m));
            LOAD_WS(REG_R(n), REG_VAL2);
            break;

          case 0xE:  // MOV.L @(R0,Rm),Rn
            ADD(REG_VAL2, REG_R(0), REG_R(m));
            LOAD_L(REG_R(n), REG_VAL2);
            break;

          case 0xF: {  // MAC.L @Rm+,@Rn+
            LOAD_L(REG_VAL0, REG_R(n));
            LOAD_L(REG_VAL1, REG_R(m));
            ADDI(REG_R(n), REG_R(n), 4);
            ADDI(REG_R(m), REG_R(m), 4);
            DMULS_START(REG_VAL0, REG_VAL1);
            STATE_LOAD(REG_VAL2, regs.MACL);
            DMUL_END(REG_VAL0, REG_VAL1);
            ADD(REG_VAL0, REG_VAL0, REG_VAL2);
            SLTU(REG_VAL2, REG_VAL0, REG_VAL2);  // Check for overflow
            STATE_STORE(REG_VAL0, regs.MACL);
            STATE_LOAD(REG_VAL0, regs.MACH);
            ADD(REG_VAL1, REG_VAL1, REG_VAL2);
            ANDI(REG_VAL2, REG_SR, SR_S);
            IFNZ(0x000F0, REG_VAL2, ADD(REG_VAL1, REG_VAL1, REG_VAL0)) {
                SLTI(REG_VAL2, REG_VAL1, -0x7FFF);
                IFNZ(0x000F1, REG_VAL2, ADDI(REG_VAL2, REG_VAL1, -0x8000)) {
                    MOVEI(REG_VAL1, -0x8000);
                    GOTO(0x000F0, STATE_STORE(REG_ZERO, regs.MACL));
                } ENDIF(0x000F1);
                IFGEZ(0x000F0, REG_VAL2, NOT(REG_VAL0, REG_ZERO)) {
                    MOVEI(REG_VAL1, 0x7FFF);
                    STATE_STORE(REG_VAL0, regs.MACL);
                }
            } ENDIF(0x000F0);
            STATE_STORE(REG_VAL1, regs.MACH);
            cur_cycles += 2;
            break;
          }  // MAC.L

          default:
            goto invalid;
        }
        break;
      }  // $0xxx

      case 0x1: {  // MOV.L Rm,@(disp,Rn)
        int disp = opcode & 0xF;
        ADDI(REG_VAL2, REG_R(n), disp*4);
        STORE_L(REG_R(m), REG_VAL2);
        break;
      }  // $1xxx

      case 0x2: {
        switch (opcode & 0xF) {
          case 0x0:  // MOV.B Rm,@Rn
            STORE_B(REG_R(m), REG_R(n));
            break;
          case 0x1:  // MOV.W Rm,@Rn
            STORE_W(REG_R(m), REG_R(n));
            break;
          case 0x2:  // MOV.L Rm,@Rn
            STORE_L(REG_R(m), REG_R(n));
            break;
          case 0x4:  // MOV.B Rm,@-Rn
            ADDI(REG_R(n), REG_R(n), -1);
            STORE_B(REG_R(m), REG_R(n));
            break;
          case 0x5:  // MOV.W Rm,@-Rn
            ADDI(REG_R(n), REG_R(n), -2);
            STORE_W(REG_R(m), REG_R(n));
            break;
          case 0x6:  // MOV.L Rm,@-Rn
            ADDI(REG_R(n), REG_R(n), -4);
            STORE_L(REG_R(m), REG_R(n));
            break;
          case 0x7: {  // DIV0S Rm,Rn
#ifdef OPTIMIZE_DIVISION
            int can_optimize = 0;
            int Rlo = -1, Rhi = n, Rdiv = m;   // Register numbers
            if (LIKELY(fetch)) {
                can_optimize = can_optimize_div0s(fetch, cur_PC,
                                                  Rhi, &Rlo, Rdiv);
            }
# ifndef DECODE_JIT
            /* When interpreting, we check immediately whether the dividend
             * is out of range of a 32-bit signed integer or the divisor is
             * zero and skip optimization if so */
            if (can_optimize) {
                can_optimize &=
                    (state->regs.R[Rhi] == (int32_t)state->regs.R[Rlo] >> 31);
                can_optimize &= state->regs.R[Rdiv] != 0;
            }
# endif
            if (can_optimize) {
                DIVS_START(REG_R(Rlo), REG_R(Rdiv));
                SRAI(REG_VAL0, REG_R(Rlo), 31);
                SRAI(REG_VAL1, REG_R(Rdiv), 31);
                XOR(REG_VAL2, REG_VAL0, REG_R(Rhi));
# ifdef DECODE_JIT
                /* When translating, we check the dividend and at runtime
                 * and skip over the optimized code for non-optimizable
                 * cases */
                IFZ(0x20070, REG_VAL2,
                    MOVEI(REG_VAL2, ~(SR_T | SR_Q | SR_M)))
                  IFNZ(0x20070, REG_R(Rdiv), ANDI(REG_VAL0, REG_VAL1, SR_M)) {
# else
                    MOVEI(REG_VAL2, ~(SR_T | SR_Q | SR_M));
                    ANDI(REG_VAL0, REG_VAL1, SR_M);
# endif
# ifdef TRACE_ALL
                    state->psp.div_data.target_PC = cur_PC + 126;
                    state->psp.div_data.Rquo = Rlo;
                    state->psp.div_data.Rrem = Rhi;
                    /* Save the original values for restoration below */
                    STATE_STORE(REG_R(Rlo), psp.div_data.quo);
                    STATE_STORE(REG_R(Rhi), psp.div_data.rem);
                    STATE_STORE(REG_SR,     psp.div_data.SR);
# endif
                    AND(REG_SR, REG_SR, REG_VAL2);  // Clear M/Q/T
                    OR(REG_SR, REG_SR, REG_VAL0);   // Set M appropriately
                    SRLI(REG_VAL0, REG_R(Rdiv), 1); // Get dividend sign bit
                    ANDI(REG_VAL1, REG_VAL1, 1);    // Get divisor sign bit (M)
                    DIV_END(REG_R(Rlo), REG_R(Rhi));
                    IFZ(0x20071, REG_VAL1, SUB(REG_VAL0,REG_R(Rlo),REG_VAL1)) {
                        /* Positive dividend */
                        ANDI(REG_VAL2, REG_VAL0, 1);   // Get value of T
                        OR(REG_SR, REG_SR, REG_VAL2);  // Copy T bit to SR
                        SRAI(REG_R(Rlo), REG_VAL0, 1); // Set quotient>>1
                        XOR(REG_VAL2, REG_VAL2, REG_VAL1); // Get value of Q
                        XORI(REG_VAL2, REG_VAL2, 1);
                        INSBITS(REG_SR, REG_VAL2, SR_Q_SHIFT, 1); // Copy to SR
                        NEG(REG_VAL0, REG_R(Rdiv));    // Get abs(divisor)
                        MOVZ(REG_VAL0, REG_R(Rdiv), REG_VAL1);
                        SUB(REG_VAL1, REG_R(Rhi), REG_VAL0); // Set remainder
                    } ELSE(0x20071, 0x20072,
                                    MOVN(REG_R(Rhi), REG_VAL1, REG_VAL2)) {
                        /* Negative dividend */
                        SLLI(REG_VAL2, REG_VAL1, 1);   // Adjust quotient
                        ADDI(REG_VAL2, REG_VAL2, -1);
                        MOVZ(REG_VAL2, REG_ZERO, REG_VAL2);
                        ADD(REG_VAL0, REG_VAL0, REG_VAL2);
                        ANDI(REG_VAL2, REG_VAL0, 1);   // Get value of T
                        OR(REG_SR, REG_SR, REG_VAL2);  // Copy T bit to SR
                        SRAI(REG_R(Rlo), REG_VAL0, 1); // Set quotient>>1
                        XOR(REG_VAL2, REG_VAL2, REG_VAL1); // Get value of Q
                        XORI(REG_VAL2, REG_VAL2, 1);
                        INSBITS(REG_SR, REG_VAL2, SR_Q_SHIFT, 1); // Copy to SR
                        NEG(REG_VAL0, REG_R(Rdiv));    // Get abs(divisor)
                        MOVZ(REG_VAL0, REG_R(Rdiv), REG_VAL1);
                        SUB(REG_VAL1, REG_R(Rhi), REG_VAL0); // Remainder if Q
                        MOVN(REG_VAL1, REG_R(Rhi), REG_R(Rhi));
                        ADD(REG_VAL0, REG_R(Rhi), REG_VAL0); // Remainder if !Q
                        MOVN(REG_R(Rhi), REG_VAL0, REG_R(Rhi)); // (inverted)
                        MOVN(REG_R(Rhi), REG_VAL1, REG_VAL2); // Final remainder
                    } ENDIF(0x20072);
# ifdef TRACE_ALL
                    /* Save the result and restore the original values
                     * so we can trace the individual instructions */
                    MOVE(REG_VAL0, REG_R(Rlo));
                    MOVE(REG_VAL1, REG_R(Rhi));
                    MOVE(REG_VAL2, REG_SR);
                    STATE_LOAD(REG_R(Rlo), psp.div_data.quo);
                    STATE_LOAD(REG_R(Rhi), psp.div_data.rem);
                    STATE_LOAD(REG_SR,     psp.div_data.SR);
                    STATE_STORE(REG_VAL0,  psp.div_data.quo);
                    STATE_STORE(REG_VAL1,  psp.div_data.rem);
                    STATE_STORE(REG_VAL2,  psp.div_data.SR);
# else
                    ADDI(REG_VAL0, REG_PC, 128);
                    state->psp.branch_target = cur_PC+128;
#  ifdef DECODE_JIT
                    /* Skip over the step-by-step emulation */
                    int saved_cycles = cur_cycles;
                    cur_cycles += 64;
                    COMMIT_CYCLES(REG_VAL1);
                    FLUSH_REGS(~0);
                    BRANCH_STATIC(REG_VAL0);
                    cur_cycles = saved_cycles;
#  else
                    cur_cycles += 64;
                    BRANCH_STATIC(REG_VAL0);
                    break;
#  endif
# endif
# ifdef DECODE_JIT
                } ENDIF(0x20070);
# endif
            }
#endif  // OPTIMIZE_DIVISION
            SRLI(REG_VAL0, REG_R(m), 31);
            INSBITS(REG_SR, REG_VAL0, SR_M_SHIFT, 1);
            SRLI(REG_VAL1, REG_R(n), 31);
            INSBITS(REG_SR, REG_VAL1, SR_Q_SHIFT, 1);
            XOR(REG_VAL2, REG_VAL0, REG_VAL1);
            INSBITS(REG_SR, REG_VAL2, SR_T_SHIFT, 1);
            break;
          }  // DIV0S
          case 0x8:  // TST Rm,Rn
            AND(REG_VAL0, REG_R(n), REG_R(m));
            SEQZ(REG_VAL1, REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x9:  // AND Rm,Rn
            AND(REG_R(n), REG_R(n), REG_R(m));
            break;
          case 0xA:  // XOR Rm,Rn
            XOR(REG_R(n), REG_R(n), REG_R(m));
            break;
          case 0xB:  // OR Rm,Rn
            OR(REG_R(n), REG_R(n), REG_R(m));
            break;
          case 0xC:  // CMP/ST Rm,Rn
            XOR(REG_VAL0, REG_R(n), REG_R(m));
            SRLI(REG_VAL1, REG_VAL0, 24);
            EXTBITS(REG_VAL2, REG_VAL0, 16, 8);
            SEQZ(REG_VAL1, REG_VAL1);
            SEQZ(REG_VAL2, REG_VAL2);
            OR(REG_VAL2, REG_VAL2, REG_VAL1);
            EXTBITS(REG_VAL1, REG_VAL0, 8, 8);
            ANDI(REG_VAL0, REG_VAL0, 0xFF);
            SEQZ(REG_VAL1, REG_VAL1);
            SEQZ(REG_VAL0, REG_VAL0);
            OR(REG_VAL1, REG_VAL1, REG_VAL0);
            OR(REG_VAL2, REG_VAL2, REG_VAL1);
            INSBITS(REG_SR, REG_VAL2, SR_T_SHIFT, 1);
            break;
          case 0xD:  // XTRCT Rm,Rn
            SRLI(REG_R(n), REG_R(n), 16);
            INSBITS(REG_R(n), REG_R(m), 16, 16);
            break;
          case 0xE:  // MULU.W Rm,Rn
            EXTZ_W(REG_VAL0, REG_R(m));
            EXTZ_W(REG_VAL1, REG_R(n));
            MUL_START(REG_VAL0, REG_VAL1);
            MUL_END(REG_VAL0);
            STATE_STORE(REG_VAL0, regs.MACL);
            break;
          case 0xF:  // MULS.W Rm,Rn
            EXTS_W(REG_VAL0, REG_R(m));
            EXTS_W(REG_VAL1, REG_R(n));
            MUL_START(REG_VAL0, REG_VAL1);
            MUL_END(REG_VAL0);
            STATE_STORE(REG_VAL0, regs.MACL);
            break;
          default:
            goto invalid;
        }
        break;
      }  // $2xxx

      case 0x3: {
        switch (opcode & 0xF) {
          case 0x0:  // CMP/EQ Rm,Rn
            XOR(REG_VAL0, REG_R(n), REG_R(m));
            SEQZ(REG_VAL1, REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x2:  // CMP/HS Rm,Rn
            SLTU(REG_VAL1, REG_R(n), REG_R(m));
            XORI(REG_VAL1, REG_VAL1, 1);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x3:  // CMP/GE Rm,Rn
            SLT(REG_VAL1, REG_R(n), REG_R(m));
            XORI(REG_VAL1, REG_VAL1, 1);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x4: {  // DIV1
#if defined(OPTIMIZE_DIVISON) && defined(TRACE_ALL)
            if (cur_PC - 2 == state->psp.div_data.target_PC) {
                /* Use optimized division result */
                int Rquo = state->psp.div_data.Rquo;
                int Rrem = state->psp.div_data.Rrem;
                STATE_LOAD(REG_R(Rquo), psp.div_data.quo);
                STATE_LOAD(REG_R(Rrem), psp.div_data.rem);
                STATE_LOAD(REG_SR,      psp.div_data.SR);
                state->psp.div_data.target_PC = 0;
                break;
            }
#endif
            SRLI(REG_VAL0, REG_R(n), 31);
            EXTBITS(REG_VAL2, REG_SR, SR_Q_SHIFT, 1);
            INSBITS(REG_SR, REG_VAL0, SR_Q_SHIFT, 1);
            SLLI(REG_R(n), REG_R(n), 1);
            EXTBITS(REG_VAL1, REG_SR, SR_T_SHIFT, 1);
            OR(REG_R(n), REG_R(n), REG_VAL1);
            EXTBITS(REG_VAL1, REG_SR, SR_M_SHIFT, 1);
            XOR(REG_VAL2, REG_VAL2, REG_VAL1);
            IFNZ(0x30040, REG_VAL2, MOVE(REG_VAL0, REG_R(n))) {
                ADD(REG_R(n), REG_R(n), REG_R(m));
            } ELSE(0x30040, 0x30041, SLTU(REG_VAL0, REG_R(n), REG_VAL0)) {
                SUB(REG_R(n), REG_R(n), REG_R(m));
                SLTU(REG_VAL0, REG_VAL0, REG_R(n));
            } ENDIF(0x30041);
            XOR(REG_VAL0, REG_VAL0, REG_VAL1);
            SLLI(REG_VAL0, REG_VAL0, SR_Q_SHIFT);
            XOR(REG_SR, REG_SR, REG_VAL0);
            EXTBITS(REG_VAL2, REG_SR, SR_Q_SHIFT, 1);
            XOR(REG_VAL2, REG_VAL2, REG_VAL1);
            XORI(REG_VAL2, REG_VAL2, 1);
            INSBITS(REG_SR, REG_VAL2, SR_T_SHIFT, 1);
            break;
          }  // DIV1 Rm,Rn
          case 0x5:  // DMULU.L Rm,Rn
            DMULU_START(REG_R(n), REG_R(m));
            DMUL_END(REG_VAL0, REG_VAL1);
            STATE_STORE(REG_VAL0, regs.MACL);
            STATE_STORE(REG_VAL1, regs.MACH);
            cur_cycles += 1;  // Minimum number of cycles
            break;
          case 0x6:  // CMP/HI Rm,Rn
            SLTU(REG_VAL1, REG_R(m), REG_R(n));
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x7:  // CMP/GT Rm,Rn
            SLT(REG_VAL1, REG_R(m), REG_R(n));
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x8:  // SUB Rm,Rn
            SUB(REG_R(n), REG_R(n), REG_R(m));
            break;
          case 0xA:  // SUBC Rm,Rn
            EXTBITS(REG_VAL2, REG_SR, SR_T_SHIFT, 1);
            SUB(REG_VAL0, REG_R(n), REG_R(m));
            SLTU(REG_VAL1, REG_R(n), REG_VAL0);
            SUB(REG_R(n), REG_VAL0, REG_VAL2);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0xB:  // SUBV Rm,Rn
            SUB(REG_VAL0, REG_R(n), REG_R(m));
            XOR(REG_VAL1, REG_R(n), REG_R(m));
            XOR(REG_VAL2, REG_R(n), REG_VAL0);
            AND(REG_VAL1, REG_VAL1, REG_VAL2);
            SRLI(REG_VAL1, REG_VAL1, 31);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            MOVE(REG_R(n), REG_VAL0);
            break;
          case 0xC:  // ADD Rm,Rn
            ADD(REG_R(n), REG_R(n), REG_R(m));
            break;
          case 0xD:  // DMULS.L Rm,Rn
            DMULS_START(REG_R(n), REG_R(m));
            DMUL_END(REG_VAL0, REG_VAL1);
            STATE_STORE(REG_VAL0, regs.MACL);
            STATE_STORE(REG_VAL1, regs.MACH);
            cur_cycles += 1;  // Minimum number of cycles
            break;
          case 0xE:  // ADDC Rm,Rn
            ADD(REG_R(n), REG_R(n), REG_R(m));
            EXTBITS(REG_VAL0, REG_SR, SR_T_SHIFT, 1);
            SLTU(REG_VAL1, REG_R(n), REG_R(m));
            ADD(REG_R(n), REG_R(n), REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0xF:  // ADDV Rm,Rn
            ADD(REG_VAL0, REG_R(n), REG_R(m));
            XOR(REG_VAL1, REG_R(n), REG_R(m));
            XORI(REG_VAL1, REG_VAL1, 1);
            XOR(REG_VAL2, REG_R(n), REG_VAL0);
            AND(REG_VAL1, REG_VAL1, REG_VAL2);
            SRLI(REG_VAL1, REG_VAL1, 31);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            MOVE(REG_R(n), REG_VAL0);
            break;
          default:
            goto invalid;
        }
        break;
      }  // $3xxx

      case 0x4: {
        switch (opcode & 0xF) {

          case 0x0: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLL Rn
                SRLI(REG_VAL1, REG_R(n), 31);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                SLLI(REG_R(n), REG_R(n), 1);
                break;
              case 0x1:  // DT Rn
                ADDI(REG_R(n), REG_R(n), -1);
                SEQZ(REG_VAL1, REG_R(n));
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                break;
              case 0x2:  // SHAL Rn
                SRLI(REG_VAL1, REG_R(n), 31);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                SLLI(REG_R(n), REG_R(n), 1);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx0

          case 0x1: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLR Rn
                INSBITS(REG_SR, REG_R(n), SR_T_SHIFT, 1);
                SRLI(REG_R(n), REG_R(n), 1);
                break;
              case 0x1:  // CMP/PZ Rn
                SLTZ(REG_VAL1, REG_R(n));
                XORI(REG_VAL1, REG_VAL1, 1);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                break;
              case 0x2:  // SHAR Rn
                INSBITS(REG_SR, REG_R(n), SR_T_SHIFT, 1);
                SRAI(REG_R(n), REG_R(n), 1);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx1

          case 0x2: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STS.L MACH,@-Rn
                STATE_LOAD(REG_VAL0, regs.MACH);
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_VAL0, REG_R(n));
                break;
              case 1:  // STS.L MACL,@-Rn
                STATE_LOAD(REG_VAL0, regs.MACL);
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_VAL0, REG_R(n));
                break;
              case 2:  // STS.L PR,@-Rn
                STATE_LOAD(REG_VAL0, regs.PR);
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_VAL0, REG_R(n));
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx2

          case 0x3: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STC.L SR,@-Rn
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_SR, REG_R(n));
                cur_cycles += 1;
                break;
              case 1:  // STC.L GBR,@-Rn
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_GBR, REG_R(n));
                cur_cycles += 1;
                break;
              case 2:  // STC.L VBR,@-Rn
                STATE_LOAD(REG_VAL0, regs.VBR);
                ADDI(REG_R(n), REG_R(n), -4);
                STORE_L(REG_VAL0, REG_R(n));
                cur_cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx3

          case 0x4: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // ROTL Rn
                SRLI(REG_VAL1, REG_R(n), 31);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                SLLI(REG_R(n), REG_R(n), 1);
                OR(REG_R(n), REG_R(n), REG_VAL1);
                break;
              case 0x2:  // ROTCL Rn
                EXTBITS(REG_VAL0, REG_SR, SR_T_SHIFT, 1);
                SRLI(REG_VAL1, REG_R(n), 31);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                SLLI(REG_R(n), REG_R(n), 1);
                OR(REG_R(n), REG_R(n), REG_VAL0);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx4

          case 0x5: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // ROTR Rn
                SLLI(REG_VAL1, REG_R(n), 31);
                INSBITS(REG_SR, REG_R(n), SR_T_SHIFT, 1);
                SRLI(REG_R(n), REG_R(n), 1);
                OR(REG_R(n), REG_R(n), REG_VAL1);
                break;
              case 0x1:  // CMP/PL Rn
                SGTZ(REG_VAL1, REG_R(n));
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                break;
              case 0x2:  // ROTCR Rn
                SLLI(REG_VAL0, REG_SR, 31);  // Saves a cycle over "ext; sll"
                INSBITS(REG_SR, REG_R(n), SR_T_SHIFT, 1);
                SRLI(REG_R(n), REG_R(n), 1);
                OR(REG_R(n), REG_R(n), REG_VAL0);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx5

          case 0x6: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDS.L @Rn+,MACH
                LOAD_L(REG_VAL0, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                STATE_STORE(REG_VAL0, regs.MACH);
                break;
              case 1:  // LDS.L @Rn+,MACL
                LOAD_L(REG_VAL0, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                STATE_STORE(REG_VAL0, regs.MACL);
                break;
              case 2:  // LDS.L @Rn+,PR
                LOAD_L(REG_VAL0, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                STATE_STORE(REG_VAL0, regs.PR);
                if (UNLIKELY(state->psp.branch_type == SH2BRTYPE_JSR)) {
                    /* The SH-2 core relies on the PR register to associate
                     * the return address with a location in the translated
                     * code, so if PR is modified in the delay slot, the
                     * accompanying RTS will jump to the wrong place; thus
                     * we change the branch to a standard dynamic one if we
                     * detect a PR-modifying instruction in the delay slot.
                     * Of course, modifying PR in a JSR/BSR delay slot is
                     * arguably broken, so this shouldn't be a problem in
                     * real life. */
                    state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                }
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx6

          case 0x7: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDC.L @Rn+,SR
                LOAD_L(REG_SR, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                ANDI(REG_SR, REG_SR, 0x3F3);
                MOVEI(REG_VAL0, 1);
                STATE_STORE(REG_VAL0, psp.need_interrupt_check);
                cur_cycles += 2;
                break;
              case 1:  // LDC.L @Rn+,GBR
                LOAD_L(REG_GBR, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                cur_cycles += 2;
                break;
              case 2:  // LDC.L @Rn+,VBR
                LOAD_L(REG_VAL0, REG_R(n));
                ADDI(REG_R(n), REG_R(n), 4);
                STATE_STORE(REG_VAL0, regs.VBR);
                cur_cycles += 2;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx7

          case 0x8: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLL2 Rn
                SLLI(REG_R(n), REG_R(n), 2);
                break;
              case 0x1:  // SHLL8 Rn
                SLLI(REG_R(n), REG_R(n), 8);
                break;
              case 0x2:  // SHLL16 Rn
                SLLI(REG_R(n), REG_R(n), 16);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx8

          case 0x9: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLR2 Rn
                SRLI(REG_R(n), REG_R(n), 2);
                break;
              case 0x1:  // SHLR8 Rn
                SRLI(REG_R(n), REG_R(n), 8);
                break;
              case 0x2:  // SHLR16 Rn
                SRLI(REG_R(n), REG_R(n), 16);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx9

          case 0xA: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDS Rn,MACH
                STATE_STORE(REG_R(n), regs.MACH);
                break;
              case 1:  // LDS Rn,MACL
                STATE_STORE(REG_R(n), regs.MACL);
                break;
              case 2:  // LDS Rn,PR
                STATE_STORE(REG_R(n), regs.PR);
                if (UNLIKELY(state->psp.branch_type == SH2BRTYPE_JSR)) {
                    /* See "LDS.L @Rn+,PR" for why we do this. */
                    state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                }
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxA

          case 0xB: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // JSR @Rn
                ADDI(REG_VAL0, REG_PC, 2);
                STATE_STORE(REG_VAL0, regs.PR);
                state->psp.branch_type = SH2BRTYPE_JSR;
                STATE_STORE(REG_R(n), psp.branch_target);
                state->delay = 1;
                cur_cycles += 1;
                break;
              case 0x1:  // TAS.B @Rn
                LOAD_BU(REG_VAL0, REG_R(n));
                SEQZ(REG_VAL1, REG_VAL0);
                INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
                ORI(REG_VAL0, REG_VAL0, 0x80);
                STORE_B(REG_VAL0, REG_R(n));
                cur_cycles += 3;
                break;
              case 0x2:  // JMP @Rn
                state->psp.branch_type = SH2BRTYPE_DYNAMIC;
                STATE_STORE(REG_R(n), psp.branch_target);
                state->delay = 1;
                cur_cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxB

          case 0xE: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDC Rn,SR
                ANDI(REG_SR, REG_R(n), 0x3F3);
                MOVEI(REG_VAL0, 1);
                STATE_STORE(REG_VAL0, psp.need_interrupt_check);
                break;
              case 1:  // LDC Rn,GBR
                MOVE(REG_GBR, REG_R(n));
                break;
              case 2:  // LDC Rn,VBR
                STATE_STORE(REG_R(n), regs.VBR);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxE

          case 0xF: {  // MAC.W @Rm+,@Rn+
            LOAD_WS(REG_VAL0, REG_R(n));
            LOAD_WS(REG_VAL1, REG_R(m));
            ADDI(REG_R(n), REG_R(n), 2);
            ADDI(REG_R(m), REG_R(m), 2);
            MUL_START(REG_VAL0, REG_VAL1);
            STATE_LOAD(REG_VAL2, regs.MACL);
            ANDI(REG_VAL0, REG_SR, SR_S);
            MUL_END(REG_VAL1);
            IFNZ(0x400F0, REG_VAL0, ADD(REG_VAL0, REG_VAL1, REG_VAL2)) {
                STATE_STORE(REG_VAL0, regs.MACL);
                SLTZ(REG_VAL0, REG_VAL0);  // sum < 0
                SLTZ(REG_VAL1, REG_VAL1);  // product < 0
                SLTZ(REG_VAL2, REG_VAL2);  // MACL < 0
                XOR(REG_VAL2, REG_VAL1, REG_VAL2);
                XORI(REG_VAL2, REG_VAL2, 1);        // (product<0) == (MACL<0)
                XOR(REG_VAL0, REG_VAL0, REG_VAL1);  // (sum<0) != (product<0)
                AND(REG_VAL0, REG_VAL0, REG_VAL2);  // Nonzero if overflow
                IFNZ(0x400F1, REG_VAL0, MOVEI(REG_VAL0, -0x80000000)) {
                    NOT(REG_VAL2, REG_VAL0);
                    MOVZ(REG_VAL0, REG_VAL2, REG_VAL1);
                    STATE_LOAD(REG_VAL1, regs.MACH);
                    STATE_STORE(REG_VAL0, regs.MACL);
                    ORI(REG_VAL1, REG_VAL1, 1);
                    STATE_STORE(REG_VAL1, regs.MACH);
                }
            } ELSE(0x400F0, 0x400F1, NOP()) {
                STATE_STORE(REG_VAL0, regs.MACL);
                SLTU(REG_VAL2, REG_VAL0, REG_VAL1); // (u32)sum < (u32)product
                STATE_LOAD(REG_VAL0, regs.MACH);
                SLTZ(REG_VAL1, REG_VAL1);           // (s32)product < 0
                SUB(REG_VAL2, REG_VAL2, REG_VAL1);
                ADD(REG_VAL0, REG_VAL0, REG_VAL2);
                STATE_STORE(REG_VAL0, regs.MACH);
            } ENDIF(0x400F1);
            cur_cycles += 2;
            break;
          }  // MAC.W

          default:
            goto invalid;
        }
        break;
      }  // $4xxx

      case 0x5: {  // MOV.L @(disp,Rm),Rn
        int disp = opcode & 0xF;
        ADDI(REG_VAL2, REG_R(m), disp*4);
        LOAD_L(REG_R(n), REG_VAL2);
        break;
      }  // $5xxx

      case 0x6: {
        switch (opcode & 0xF) {
          case 0x0:  // MOV.B @Rm,Rn
            LOAD_BS(REG_R(n), REG_R(m));
            break;
          case 0x1:  // MOV.W @Rm,Rn
            LOAD_WS(REG_R(n), REG_R(m));
            break;
          case 0x2:  // MOV.L @Rm,Rn
            LOAD_L(REG_R(n), REG_R(m));
            break;
          case 0x3:  // MOV Rm,Rn
            MOVE(REG_R(n), REG_R(m));
            break;
          case 0x4:  // MOV.B @Rm+,Rn
            LOAD_BS(REG_R(n), REG_R(m));
            ADDI(REG_R(m), REG_R(m), 1);
            break;
          case 0x5:  // MOV.W @Rm+,Rn
            LOAD_WS(REG_R(n), REG_R(m));
            ADDI(REG_R(m), REG_R(m), 2);
            break;
          case 0x6:  // MOV.L @Rm+,Rn
            LOAD_L(REG_R(n), REG_R(m));
            ADDI(REG_R(m), REG_R(m), 4);
            break;
          case 0x7:  // NOT Rm,Rn
            NOT(REG_R(n), REG_R(m));
            break;
          case 0x8:  // SWAP.B Rm,Rn
            SRLI(REG_VAL0, REG_R(m), 8);
            if (m != n) {
                MOVE(REG_R(n), REG_R(m));
            }
            INSBITS(REG_R(n), REG_R(m), 8, 8);
            INSBITS(REG_R(n), REG_VAL0, 0, 8);
            break;
          case 0x9:  // SWAP.W Rm,Rn
            SRLI(REG_VAL0, REG_R(m), 16);
            SLLI(REG_R(n), REG_R(m), 16);
            OR(REG_R(n), REG_R(n), REG_VAL0);
            break;
          case 0xA:  // NEGC Rm,Rn
            EXTBITS(REG_VAL0, REG_SR, SR_T_SHIFT, 1);
            SEQZ(REG_VAL1, REG_R(m));
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            NEG(REG_R(n), REG_R(m));
            SUB(REG_R(n), REG_R(n), REG_VAL0);
            break;
          case 0xB:  // NEG Rm,Rn
            NEG(REG_R(n), REG_R(m));
            break;
          case 0xC:  // EXTU.B Rm,Rn
            EXTZ_B(REG_R(n), REG_R(m));
            break;
          case 0xD:  // EXTU.W Rm,Rn
            EXTZ_W(REG_R(n), REG_R(m));
            break;
          case 0xE:  // EXTS.B Rm,Rn
            EXTS_B(REG_R(n), REG_R(m));
            break;
          case 0xF:  // EXTS.W Rm,Rn
            EXTS_W(REG_R(n), REG_R(m));
            break;
          default:   // impossible
            goto invalid;
        }
        break;
      }  // $6xxx

      case 0x7: {  // ADD #imm,Rn
        int8_t imm = opcode & 0xFF;
        ADDI(REG_R(n), REG_R(n), (int32_t)imm);
        break;
      }  // $7xxx

      case 0x8: {
        switch (opcode>>8 & 0xF) {
          case 0x0: {  // MOV.B R0,@(disp,Rm)
            int disp = opcode & 0xF;
            ADDI(REG_VAL2, REG_R(m), disp);
            STORE_B(REG_R(0), REG_VAL2);
            break;
          }
          case 0x1: {  // MOV.W R0,@(disp,Rm)
            int disp = opcode & 0xF;
            ADDI(REG_VAL2, REG_R(m), disp*2);
            STORE_W(REG_R(0), REG_VAL2);
            break;
          }
          case 0x4: {  // MOV.B @(disp,Rm),R0
            int disp = opcode & 0xF;
            ADDI(REG_VAL2, REG_R(m), disp);
            LOAD_BS(REG_R(0), REG_VAL2);
            break;
          }
          case 0x5: {  // MOV.W @(disp,Rm),R0
            int disp = opcode & 0xF;
            ADDI(REG_VAL2, REG_R(m), disp*2);
            LOAD_WS(REG_R(0), REG_VAL2);
            break;
          }
          case 0x8: {  // CMP/EQ #imm,R0
            int8_t imm = opcode & 0xFF;
            ADDI(REG_VAL0, REG_R(0), -(int32_t)imm);
            SEQZ(REG_VAL1, REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          }
          case 0x9: {  // BT label
            int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
            ADDI(REG_VAL0, REG_PC, 2 + disp);
            EXTBITS(REG_VAL1, REG_SR, SR_T_SHIFT, 1);
            state->psp.branch_type = SH2BRTYPE_BT;
            state->psp.branch_target = (cur_PC + 2) + disp;
            STATE_STORE(REG_VAL0, psp.branch_target);
            STATE_STORE(REG_VAL1, psp.branch_cond);
            break;
          }
          case 0xB: {  // BF label
            int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
            ADDI(REG_VAL0, REG_PC, 2 + disp);
            EXTBITS(REG_VAL1, REG_SR, SR_T_SHIFT, 1);
            state->psp.branch_type = SH2BRTYPE_BF;
            state->psp.branch_target = (cur_PC + 2) + disp;
            STATE_STORE(REG_VAL0, psp.branch_target);
            STATE_STORE(REG_VAL1, psp.branch_cond);
            break;
          }
          case 0xD: {  // BT/S label
            int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
            ADDI(REG_VAL0, REG_PC, 2 + disp);
            EXTBITS(REG_VAL1, REG_SR, SR_T_SHIFT, 1);
            state->psp.branch_type = SH2BRTYPE_BT_S;
            state->psp.branch_target = (cur_PC + 2) + disp;
            STATE_STORE(REG_VAL0, psp.branch_target);
            STATE_STORE(REG_VAL1, psp.branch_cond);
#ifdef TRACE_ALL
# ifdef TRACE_LIKE_SH2INT
            STATE_LOAD(REG_VAL2, cycles);
# endif
            IFNZ(0x8D000, REG_VAL1, MOVEI(REG_VAL0, 1)) {
                STATE_STORE(REG_VAL0, delay);
                /* For interpretation, the following line's effect is
                 * identical to the line above (if the compiler is smart
                 * enough, it will be optimized out).  For translation, the
                 * following line indicates that the next instruction is in
                 * a delay slot, so the branching logic is inserted at the
                 * proper point. */
                state->delay = 1;
# ifdef TRACE_LIKE_SH2INT
                ADDI(REG_VAL2, REG_VAL2, 1);
                STATE_STORE(REG_VAL2, cycles);
# endif
            } ENDIF(0x8D000);
#else
            /* Just need to tell the translator about the delay slot (it
             * will make sure the code doesn't return) */
            state->delay = 1;
#endif
            break;
          }
          case 0xF: {  // BF/S label
            int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
            ADDI(REG_VAL0, REG_PC, 2 + disp);
            EXTBITS(REG_VAL1, REG_SR, SR_T_SHIFT, 1);
            state->psp.branch_type = SH2BRTYPE_BF_S;
            state->psp.branch_target = (cur_PC + 2) + disp;
            STATE_STORE(REG_VAL0, psp.branch_target);
            STATE_STORE(REG_VAL1, psp.branch_cond);
#ifdef TRACE_ALL
# ifdef TRACE_LIKE_SH2INT
            STATE_LOAD(REG_VAL2, cycles);
# endif
            IFZ(0x8F000, REG_VAL1, MOVEI(REG_VAL0, 1)) {
                STATE_STORE(REG_VAL0, delay);
                state->delay = 1;
# ifdef TRACE_LIKE_SH2INT
                ADDI(REG_VAL2, REG_VAL2, 1);
                STATE_STORE(REG_VAL2, cycles);
# endif
            } ENDIF(0x8F000);
#else
            state->delay = 1;
#endif
            break;
          }
          default:
            goto invalid;
        }
        break;
      }  // $8xxx

      case 0x9: {  // MOV.W @(disp,PC),Rn
        int disp = opcode & 0xFF;
        ADDI(REG_VAL2, REG_PC, 2 + disp*2);
        LOAD_WS(REG_R(n), REG_VAL2);
        break;
      }  // $9xxx

      case 0xA: {  // BRA label
        int disp = ((int32_t)(opcode & 0xFFF) << 20) >> 19;
        state->psp.branch_type = SH2BRTYPE_STATIC;
        state->psp.branch_target = (cur_PC + 2) + disp;
        ADDI(REG_VAL0, REG_PC, 2 + disp);
        STATE_STORE(REG_VAL0, psp.branch_target);
        state->delay = 1;
        cur_cycles += 1;
        break;
      }  // $Axxx

      case 0xB: {  // BSR label
        int disp = ((int32_t)(opcode & 0xFFF) << 20) >> 19;
        ADDI(REG_VAL0, REG_PC, 2);
        STATE_STORE(REG_VAL0, regs.PR);
        state->psp.branch_type = SH2BRTYPE_JSR;
        state->psp.branch_target = (cur_PC + 2) + disp;
        ADDI(REG_VAL0, REG_VAL0, disp);
        STATE_STORE(REG_VAL0, psp.branch_target);
        state->delay = 1;
        cur_cycles += 1;
        break;
      }  // $Bxxx

      case 0xC: {
        int imm = opcode & 0xFF;

        switch (opcode>>8 & 0xF) {
          case 0x0:  // MOV.B R0,@(disp,GBR)
            ADDI(REG_VAL2, REG_GBR, imm);
            STORE_B(REG_R(0), REG_VAL2);
            break;
          case 0x1:  // MOV.W R0,@(disp,GBR)
            ADDI(REG_VAL2, REG_GBR, imm*2);
            STORE_W(REG_R(0), REG_VAL2);
            break;
          case 0x2:  // MOV.L R0,@(disp,GBR)
            ADDI(REG_VAL2, REG_GBR, imm*4);
            STORE_L(REG_R(0), REG_VAL2);
            break;
          case 0x3:  // TRAPA #imm
            ADDI(REG_R(15), REG_R(15), -4);
            STORE_L(REG_SR, REG_R(15));
            ADDI(REG_R(15), REG_R(15), -4);
            STORE_L(REG_PC, REG_R(15));
            state->psp.branch_type = SH2BRTYPE_STATIC;
            state->psp.branch_target = imm<<2;
            MOVEI(REG_VAL0, imm<<2);
            STATE_STORE(REG_VAL0, psp.branch_target);
            cur_cycles += 7;
            break;
          case 0x4:  // MOV.B @(disp,GBR),R0
            ADDI(REG_VAL2, REG_GBR, imm);
            LOAD_BS(REG_R(0), REG_VAL2);
            break;
          case 0x5:  // MOV.W @(disp,GBR),R0
            ADDI(REG_VAL2, REG_GBR, imm*2);
            LOAD_WS(REG_R(0), REG_VAL2);
            break;
          case 0x6:  // MOV.L @(disp,GBR),R0
            ADDI(REG_VAL2, REG_GBR, imm*4);
            LOAD_L(REG_R(0), REG_VAL2);
            break;
          case 0x7:  // MOVA @(disp,PC),R0
            ADDI(REG_R(0), REG_PC, 2 + imm*4);
            INSBITS(REG_R(0), REG_ZERO, 0, 2);
            break;
          case 0x8:  // TST #imm,R0
            ANDI(REG_VAL0, REG_R(0), imm);
            SEQZ(REG_VAL1, REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0x9:  // AND #imm,R0
            ANDI(REG_R(0), REG_R(0), imm);
            break;
          case 0xA:  // XOR #imm,R0
            XORI(REG_R(0), REG_R(0), imm);
            break;
          case 0xB:  // OR #imm,R0
            ORI(REG_R(0), REG_R(0), imm);
            break;
          case 0xC:  // TST.B #imm,@(R0,GBR)
            ADD(REG_VAL2, REG_R(0), REG_GBR);
            LOAD_BU(REG_VAL0, REG_VAL2);
            ANDI(REG_VAL0, REG_VAL0, imm);
            SEQZ(REG_VAL1, REG_VAL0);
            INSBITS(REG_SR, REG_VAL1, SR_T_SHIFT, 1);
            break;
          case 0xD:  // AND.B #imm,@(R0,GBR)
            ADD(REG_VAL2, REG_R(0), REG_GBR);
            LOAD_BU(REG_VAL0, REG_VAL2);
            ANDI(REG_VAL0, REG_VAL0, imm);
            STORE_B(REG_VAL0, REG_VAL2);
            cur_cycles += 2;
            break;
          case 0xE:  // XOR.B #imm,@(R0,GBR)
            ADD(REG_VAL2, REG_R(0), REG_GBR);
            LOAD_BU(REG_VAL0, REG_VAL2);
            XORI(REG_VAL0, REG_VAL0, imm);
            STORE_B(REG_VAL0, REG_VAL2);
            cur_cycles += 2;
            break;
          case 0xF: {  // OR.B #imm,@(R0,GBR)
            ADD(REG_VAL2, REG_R(0), REG_GBR);
            LOAD_BU(REG_VAL0, REG_VAL2);
            ORI(REG_VAL0, REG_VAL0, imm);
            STORE_B(REG_VAL0, REG_VAL2);
            cur_cycles += 2;
            break;
          }
          default:  // impossible
            goto invalid;
        }
        break;

      }  // $Cxxx

      case 0xD: {  // MOV.L @(disp,PC),Rn
        int disp = opcode & 0xFF;
        ADDI(REG_VAL2, REG_PC, 2 + disp*4);
        INSBITS(REG_VAL2, REG_ZERO, 0, 2);
        LOAD_L(REG_R(n), REG_VAL2);
        break;
      }  // $Dxxx

      case 0xE: {  // MOV #imm,Rn
        int8_t imm = opcode & 0xFF;
        MOVEI(REG_R(n), (int32_t)imm);
        break;
      }  // $Exxx

      case 0xF: {
        goto invalid;
      }  // $Fxxx

    }

    /**** The Big Honkin' Opcode Switch Ends Here ****/

    /* Handle any pending branches */

    if (UNLIKELY(state->psp.branch_type != SH2BRTYPE_NONE && !state->delay)) {
        FLUSH_REGS(~0);
#ifdef OPTIMIZE_IDLE
        int is_idle = 0;
        if ((state->psp.branch_type == SH2BRTYPE_STATIC
          || state->psp.branch_type == SH2BRTYPE_BT
          || state->psp.branch_type == SH2BRTYPE_BF
          || state->psp.branch_type == SH2BRTYPE_BT_S
          || state->psp.branch_type == SH2BRTYPE_BF_S)
         && state->psp.branch_target >= initial_PC
         && state->psp.branch_target < cur_PC
         && (cur_PC - state->psp.branch_target) / 2 < OPTIMIZE_IDLE_MAX_INSNS
        ) {
            const int num_insns = (cur_PC - state->psp.branch_target) / 2;
            is_idle = can_optimize_idle(fetch - num_insns,
                                        state->psp.branch_target, num_insns);
# ifdef JIT_DEBUG_VERBOSE
            if (is_idle) {
                DMSG("Found idle loop at 0x%08X (%d instructions)",
                     (int)state->psp.branch_target, num_insns);
            }
# endif
        }
        if (is_idle) {
            CONSUME_CYCLES(REG_VAL0);
        } else
#endif
        COMMIT_CYCLES(REG_VAL0);
        switch (state->psp.branch_type) {
          case SH2BRTYPE_NONE:  // Avoid a compiler warning
            break;
          case SH2BRTYPE_DYNAMIC:
            STATE_LOAD(REG_VAL0, psp.branch_target);
            BRANCH_DYNAMIC(REG_VAL0);
            break;
          case SH2BRTYPE_STATIC:
            STATE_LOAD(REG_VAL0, psp.branch_target);
            BRANCH_STATIC(REG_VAL0);
            break;
          case SH2BRTYPE_JSR:
            STATE_LOAD(REG_VAL0, psp.branch_target);
            BRANCH_CALL(REG_VAL0);
            break;
          case SH2BRTYPE_RTS:
            STATE_LOAD(REG_VAL0, psp.branch_target);
            BRANCH_RETURN(REG_VAL0);
            break;
          case SH2BRTYPE_BT:
          case SH2BRTYPE_BF:
          case SH2BRTYPE_BT_S:
          case SH2BRTYPE_BF_S:
            STATE_LOAD(REG_VAL1, psp.branch_cond);
            STATE_LOAD(REG_VAL2, cycles);
            STATE_LOAD(REG_VAL0, psp.branch_target);
            if (state->psp.branch_type == SH2BRTYPE_BT
             || state->psp.branch_type == SH2BRTYPE_BT_S
            ) {
                const int add_cycles =
                    (state->psp.branch_type == SH2BRTYPE_BT ? 2 : 1);
                IFNZ(0xFFFF1, REG_VAL1, ADDI(REG_VAL2, REG_VAL2, add_cycles)) {
#ifdef TRACE_LIKE_SH2INT
                    if (state->psp.branch_type == SH2BRTYPE_BT)
#endif
                    STATE_STORE(REG_VAL2, cycles);
                    BRANCH_STATIC(REG_VAL0);
                } ENDIF(0xFFFF1);
            } else {  // BF or BF/S
                const int add_cycles =
                    (state->psp.branch_type == SH2BRTYPE_BF ? 2 : 1);
                IFZ(0xFFFF0, REG_VAL1, ADDI(REG_VAL2, REG_VAL2, add_cycles)) {
#ifdef TRACE_LIKE_SH2INT
                    if (state->psp.branch_type == SH2BRTYPE_BF)
#endif
                    STATE_STORE(REG_VAL2, cycles);
                    BRANCH_STATIC(REG_VAL0);
                } ENDIF(0xFFFF0);
            }
            break;
        }
        state->psp.branch_type = SH2BRTYPE_NONE;
    }

    /* Invalid opcode handler (jumped to on detection of an invalid opcode) */

    if (0) {
      invalid:
        invalid_opcode = 1;
        state->instruction = opcode;
        /* Hack for translation so the error shows at least PC properly */
        uint32_t saved_PC = state->regs.PC;
        state->regs.PC = cur_PC - 2;
        YabSetError(YAB_ERR_SH2INVALIDOPCODE, state);
        state->regs.PC = saved_PC;

        /* Push SR and PC */
        ADDI(REG_R(15), REG_R(15), -4);
        STORE_L(REG_SR, REG_R(15));
        ADDI(REG_R(15), REG_R(15), -4);
        ADDI(REG_VAL0, REG_PC, -2);  // Since we incremented it earlier
        STORE_L(REG_VAL0, REG_R(15));

        /* Call the relevant exception vector (6 if instruction was in a
         * delay slot, else 4) */
        cur_cycles++;
        COMMIT_CYCLES(REG_VAL0);
        state->psp.branch_target = (state->delay ? 6 : 4) << 2;
        MOVEI(REG_VAL0, state->psp.branch_target);
        BRANCH_STATIC(REG_VAL0);
    }

}  // End of SH-2 decoding core

/*************************************************************************/

/*
 * For reference, the following is the original C code for the opcode
 * switch before conversion to micro-ops.  (Note that parts of it may be
 * out of date due to optimization or other changes.)
 */

#if 0

    switch (opcode>>12 & 0xF) {

      case 0x0: {
        switch (opcode & 0xF) {

          case 0x2: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STC SR,Rn
                state->regs.R[n] = state->regs.SR.all;
                break;
              case 1:  // STC GBR,Rn
                state->regs.R[n] = state->regs.GBR;
                break;
              case 2:  // STC VBR,Rn
                state->regs.R[n] = state->regs.VBR;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx2

          case 0x3: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // BSRF Rn
                state->regs.PR = state->regs.PC + 2;
                state->delay = (state->regs.PC+2) + state->regs.R[n];
                state->cycles += 1;
                break;
              case 0x2:  // BRAF Rn
                state->delay = (state->regs.PC+2) + state->regs.R[n];
                state->cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx3

          case 0x4: {  // MOV.B Rm,@(R0,Rn)
            uint32_t address = state->regs.R[0] + state->regs.R[n];
            MappedMemoryWriteByte(address, state->regs.R[m]);
            break;
          }

          case 0x5: {  // MOV.W Rm,@(R0,Rn)
            uint32_t address = state->regs.R[0] + state->regs.R[n];
            MappedMemoryWriteWord(address, state->regs.R[m]);
            break;
          }

          case 0x6: {  // MOV.L Rm,@(R0,Rn)
            uint32_t address = state->regs.R[0] + state->regs.R[n];
            MappedMemoryWriteLong(address, state->regs.R[m]);
            break;
          }

          case 0x7: {  // MUL.L Rm,Rn
            state->regs.MACL = state->regs.R[m] * state->regs.R[n];
            state->cycles += 1;  // Minimum number of cycles
            break;
          }

          case 0x8: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // CLRT
                state->regs.SR.part.T = 0;
                break;
              case 0x1:  // SETT
                state->regs.SR.part.T = 1;
                break;
              case 0x2:  // CLRMAC
                state->regs.MACH = 0;
                state->regs.MACL = 0;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx8

          case 0x9: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // NOP
                /* No operation */
                break;
              case 0x1:  // DIV0U
                state->regs.SR.part.M = 0;
                state->regs.SR.part.Q = 0;
                state->regs.SR.part.T = 0;
                break;
              case 0x2:  // MOVT Rn
                state->regs.R[n] = state->regs.SR.part.T;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xx9

          case 0xA: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STS MACH,Rn
                state->regs.R[n] = state->regs.MACH;
                break;
              case 1:  // STS MACL,Rn
                state->regs.R[n] = state->regs.MACL;
                break;
              case 2:  // STS PR,Rn
                state->regs.R[n] = state->regs.PR;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xxA

          case 0xB: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // RTS
                state->delay = state->regs.PR;
                state->cycles += 1;
                break;
              case 0x1:  // SLEEP
                state->PC -= 2;
                state->cycles += 2;
                break;
              case 0x2:  // RTE
                state->delay = MappedMemoryReadLong(state->regs.R[15]);
                state->regs.R[15] += 4;
                state->regs.SR.all =
                    MappedMemoryReadLong(state->regs.R[15]) & 0x000003F3;
                state->regs.R[15] += 4;
                state->cycles += 3;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $0xxB

          case 0xC: {  // MOV.B @(R0,Rm),Rn
            uint32_t address = state->regs.R[0] + state->regs.R[m];
            state->regs.R[n] = (int32_t)(int8_t)MappedMemoryReadByte(address);
            break;
          }

          case 0xD: {  // MOV.W @(R0,Rm),Rn
            uint32_t address = state->regs.R[0] + state->regs.R[m];
            state->regs.R[n] = (int32_t)(int16_t)MappedMemoryReadWord(address);
            break;
          }

          case 0xE: {  // MOV.L @(R0,Rm),Rn
            uint32_t address = state->regs.R[0] + state->regs.R[m];
            state->regs.R[n] = MappedMemoryReadLong(address);
            break;
          }

          case 0xF: {  // MAC.L @Rm+,@Rn+
            int32_t value_m = MappedMemoryReadLong(state->regs.R[m]);
            state->regs.R[m] += 4;
            int32_t value_n = MappedMemoryReadLong(state->regs.R[n]);
            state->regs.R[n] += 4;
#ifdef PSP
            asm("mult %[value_m], %[value_n]\n"
                "mflo %[dummy]\n"
                "addu %[MACL], %[MACL], %[dummy]\n"
                "sltu %[dummy], %[MACL], %[dummy]\n"
                "addu %[MACH], %[MACH], %[dummy]\n"
                "mfhi %[dummy]\n"
                "addu %[MACH], %[MACH], %[dummy]\n"
                : [MACH] "=r" (state->regs.MACH),
                  [MACL] "=r" (state->regs.MACL), [dummy] "=r" (value_m)
                : "0" (state->regs.MACH), "1" (state->regs.MACL),
                  [value_m] "r" (value_m), [value_n] "r" (value_n)
            );
#else
            {int64_t MAC = (int64_t)state->regs.MACH<<32 | state->regs.MACL;
            MAC += (int64_t)(int32_t)value_m * (int64_t)(int32_t)value_n;
            state->regs.MACH = MAC>>32;
            state->regs.MACL = MAC;}
#endif
            if (state->regs.SR.part.S) {
                if ((int32_t)state->regs.MACH < -0x7FFF) {
                    state->regs.MACH = -0x8000;
                    state->regs.MACL = 0;
                } else if ((int32_t)state->regs.MACH > 0x7FFF) {
                    state->regs.MACH = 0x7FFF;
                    state->regs.MACL = 0xFFFFFFFF;
                }
            }
            state->cycles += 2;
            break;
          }

          default:
            goto invalid;
        }
        break;
      }  // $0xxx

      case 0x1: {  // MOV.L Rm,@(disp,Rn)
        int disp = opcode & 0xF;
        uint32_t address = state->regs.R[n] + disp*4;
        MappedMemoryWriteLong(address, state->regs.R[m]);
        break;
      }  // $1xxx

      case 0x2: {
        switch (opcode & 0xF) {
          case 0x0:  // MOV.B Rm,@Rn
            MappedMemoryWriteByte(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x1:  // MOV.W Rm,@Rn
            MappedMemoryWriteWord(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x2:  // MOV.L Rm,@Rn
            MappedMemoryWriteLong(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x4:  // MOV.B Rm,@-Rn
            state->regs.R[n] -= 1;
            MappedMemoryWriteByte(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x5:  // MOV.W Rm,@-Rn
            state->regs.R[n] -= 2;
            MappedMemoryWriteWord(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x6:  // MOV.L Rm,@-Rn
            state->regs.R[n] -= 4;
            MappedMemoryWriteLong(state->regs.R[n], state->regs.R[m]);
            break;
          case 0x7:  // DIV0S Rm,Rn
            state->regs.SR.part.M = state->regs.R[m] >> 31;
            state->regs.SR.part.Q = state->regs.R[n] >> 31;
            state->regs.SR.part.T = (state->regs.R[m] ^ state->regs.R[n]) >> 31;
            break;
          case 0x8:  // TST Rm,Rn
            state->regs.SR.part.T =
                ((state->regs.R[n] & state->regs.R[m]) == 0);
            break;
          case 0x9:  // AND Rm,Rn
            state->regs.R[n] &= state->regs.R[m];
            break;
          case 0xA:  // XOR Rm,Rn
            state->regs.R[n] ^= state->regs.R[m];
            break;
          case 0xB:  // OR Rm,Rn
            state->regs.R[n] |= state->regs.R[m];
            break;
          case 0xC: {  // CMP/ST Rm,Rn
            uint32_t temp = state->regs.R[m] ^ state->regs.R[n];
            state->regs.SR.part.T = ((temp>>24 & 0xFF) == 0)
                                  | ((temp>>16 & 0xFF) == 0)
                                  | ((temp>> 8 & 0xFF) == 0)
                                  | ((temp>> 0 & 0xFF) == 0);
            break;
          }
          case 0xD:  // XTRCT Rm,Rn
            state->regs.R[n] = state->regs.R[m]<<16 | state->regs.R[n]>>16;
            break;
          case 0xE:  // MULU.W Rm,Rn
            state->regs.MACL =
                (uint16_t)state->regs.R[m] * (uint16_t)state->regs.R[n];
            break;
          case 0xF:  // MULS.W Rm,Rn
            state->regs.MACL =
                (int16_t)state->regs.R[m] * (int16_t)state->regs.R[n];
            break;
          default:
            goto invalid;
        }
        break;
      }  // $2xxx

      case 0x3: {
        switch (opcode & 0xF) {
          case 0x0:  // CMP/EQ Rm,Rn
            state->regs.SR.part.T = (state->regs.R[n] == state->regs.R[m]);
            break;
          case 0x2:  // CMP/HS Rm,Rn
            state->regs.SR.part.T = (state->regs.R[n] >= state->regs.R[m]);
            break;
          case 0x3:  // CMP/GE Rm,Rn
            state->regs.SR.part.T =
                ((int32_t)state->regs.R[n] >= (int32_t)state->regs.R[m]);
            break;
          case 0x4: {  // DIV1 Rm,Rn
            int old_q = state->regs.SR.part.Q;
            state->regs.SR.part.Q = state->regs.R[n] >> 31;
            state->regs.R[n] <<= 1;
            state->regs.R[n] |= state->regs.SR.part.T;
            uint32_t temp = state->regs.R[n];
#if 0  // Logic according to Hitachi specs
            if (!old_q) {
                if (!state->regs.SR.part.M) {
                    state->regs.R[n] -= state->regs.R[m];
                    int borrow = (state->regs.R[n] > temp);
                    state->regs.SR.part.Q ^= borrow;
                } else {
                    state->regs.R[n] += state->regs.R[m];
                    int carry = state->regs.R[n] < temp;
                    state->regs.SR.part.Q ^= !carry;
                }
            } else {
                if (!state->regs.SR.part.M) {
                    state->regs.R[n] += state->regs.R[m];
                    int carry = state->regs.R[n] < temp;
                    state->regs.SR.part.Q ^= carry;
                } else {
                    state->regs.R[n] -= state->regs.R[m];
                    int borrow = (state->regs.R[n] > temp);
                    state->regs.SR.part.Q ^= !borrow;
                }
            }
#else  // Shorter version (still correct)
            int carry;
            if (old_q ^ state->regs.SR.part.M) {
                state->regs.R[n] += state->regs.R[m];
                carry = state->regs.R[n] < temp;
            } else {
                state->regs.R[n] -= state->regs.R[m];
                carry = state->regs.R[n] > temp;
            }
            state->regs.SR.part.Q ^= carry ^ state->regs.SR.part.M;
#endif
            state->regs.SR.part.T = (state->regs.SR.part.Q == state->regs.SR.part.M);
            break;
          }  // DIV1
          case 0x5:  // DMULU.L Rm,Rn
#ifdef PSP
            asm("multu %[Rm], %[Rn]\n"
                : "=h" (state->regs.MACH), "=l" (state->regs.MACL)
                : [Rm] "r" (state->regs.R[m]), [Rn] "r" (state->regs.R[n])
            );
#else
            {uint64_t MAC = (uint64_t)state->regs.R[m] * (uint64_t)state->regs.R[n];
            state->regs.MACH = MAC>>32;
            state->regs.MACL = MAC;}
#endif
            state->cycles += 1;  // Minimum number of cycles
            break;
          case 0x6:  // CMP/HI Rm,Rn
            state->regs.SR.part.T = (state->regs.R[n] > state->regs.R[m]);
            break;
          case 0x7:  // CMP/GT Rm,Rn
            state->regs.SR.part.T =
                ((int32_t)state->regs.R[n] > (int32_t)state->regs.R[m]);
            break;
          case 0x8:  // SUB Rm,Rn
            state->regs.R[n] -= state->regs.R[m];
            break;
          case 0xA: {  // SUBC Rm,Rn
            uint32_t difference = state->regs.R[n] - state->regs.R[m];
            int newT = (difference > state->regs.R[n]);
            state->regs.R[n] = difference - state->regs.SR.part.T;
            state->regs.SR.part.T = newT;
            break;
          }
          case 0xB: {  // SUBV Rm,Rn
            uint32_t difference = state->regs.R[n] - state->regs.R[m];
            int diff_sign = (state->regs.R[m]>>31 ^ state->regs.R[n]>>31);
            int sign_change = state->regs.R[n]>>31 ^ difference>>31;
            state->regs.SR.part.T = diff_sign & sign_change;
            break;
          }
          case 0xC:  // ADD Rm,Rn
            state->regs.R[n] += state->regs.R[m];
            break;
          case 0xD:  // DMULS.L Rm,Rn
#ifdef PSP
            asm("mult %[Rm], %[Rn]\n"
                : "=h" (state->regs.MACH), "=l" (state->regs.MACL)
                : [Rm] "r" (state->regs.R[m]), [Rn] "r" (state->regs.R[n])
            );
#else
            {int64_t MAC = (int64_t)(int32_t)state->regs.R[m] * (int64_t)(int32_t)state->regs.R[n];
            state->regs.MACH = MAC>>32;
            state->regs.MACL = MAC;}
#endif
            state->cycles += 1;  // Minimum number of cycles
            break;
          case 0xE: {  // ADDC Rm,Rn
            state->regs.R[n] += state->regs.R[m];
            int newT = (state->regs.R[n] < state->regs.R[m]);
            state->regs.R[n] += state->regs.SR.part.T;
            state->regs.SR.part.T = newT;
            break;
          }
          case 0xF: {  // ADDV Rm,Rn
            uint32_t sum = state->regs.R[n] + state->regs.R[m];
            int same_sign = !(state->regs.R[n]>>31 ^ state->regs.R[m]>>31);
            int sign_change = state->regs.R[n]>>31 ^ sum>>31;
            state->regs.SR.part.T = same_sign & sign_change;
            break;
          }
          default:
            goto invalid;
        }
        break;
      }  // $3xxx

      case 0x4: {
        switch (opcode & 0xF) {

          case 0x0: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLL Rn
                state->regs.SR.part.T = state->regs.R[n] >> 31;
                state->regs.R[n] <<= 1;
                break;
              case 0x1:  // DT Rn
                state->regs.R[n]--;
                state->regs.SR.part.T = (state->regs.R[n] == 0);
                break;
              case 0x2:  // SHAL Rn
                state->regs.SR.part.T = state->regs.R[n] >> 31;
                state->regs.R[n] <<= 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx0

          case 0x1: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLR Rn
                state->regs.SR.part.T = state->regs.R[n] & 1;
                state->regs.R[n] >>= 1;
                break;
              case 0x1:  // CMP/PZ Rn
                state->regs.SR.part.T = ((int32_t)state->regs.R[n] >= 0);
                break;
              case 0x2:  // SHAR Rn
                state->regs.SR.part.T = state->regs.R[n] & 1;
                state->regs.R[n] = (int32_t)state->regs.R[n] >> 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx1

          case 0x2: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STS.L MACH,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.MACH);
                break;
              case 1:  // STS.L MACL,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.MACL);
                break;
              case 2:  // STS.L PR,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.PR);
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx2

          case 0x3: {
            switch (opcode>>4 & 0xF) {
              case 0:  // STC.L SR,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.SR.all);
                state->cycles += 1;
                break;
              case 1:  // STC.L GBR,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.GBR);
                state->cycles += 1;
                break;
              case 2:  // STC.L VBR,@-Rn
                state->regs.R[n] -= 4;
                MappedMemoryWriteLong(state->regs.R[n], state->regs.VBR);
                state->cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx3

          case 0x4: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // ROTL Rn
                state->regs.SR.part.T = state->regs.R[n] >> 31;
                state->regs.R[n] <<= 1;
                state->regs.R[n] |= state->regs.SR.part.T;
                break;
              case 0x2: {  // ROTCL Rn
                int newT = state->regs.R[n] >> 31;
                state->regs.R[n] <<= 1;
                state->regs.R[n] |= state->regs.SR.part.T;
                state->regs.SR.part.T = newT;
                break;
              }
              default:
                goto invalid;
            }
            break;
          }  // $4xx4

          case 0x5: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // ROTR Rn
                state->regs.SR.part.T = state->regs.R[n] & 1;
                state->regs.R[n] >>= 1;
                state->regs.R[n] |= (uint32_t)state->regs.SR.part.T << 31;
                break;
              case 0x1:  // CMP/PL Rn
                state->regs.SR.part.T = ((int32_t)state->regs.R[n] > 0);
                break;
              case 0x2: {  // ROTCR Rn
                int newT = state->regs.R[n] & 1;
                state->regs.R[n] >>= 1;
                state->regs.R[n] |= (uint32_t)state->regs.SR.part.T << 31;
                state->regs.SR.part.T = newT;
                break;
              }
              default:
                goto invalid;
            }
            break;
          }  // $4xx5

          case 0x6: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDS.L @Rn+,MACH
                state->regs.MACH = MappedMemoryReadLong(state->regs.R[n]);
                state->regs.R[n] += 4;
                break;
              case 1:  // LDS.L @Rn+,MACL
                state->regs.MACL = MappedMemoryReadLong(state->regs.R[n]);
                state->regs.R[n] += 4;
                break;
              case 2:  // LDS.L @Rn+,PR
                state->regs.PR = MappedMemoryReadLong(state->regs.R[n]);
                state->regs.R[n] += 4;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx6

          case 0x7: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDC.L @Rn+,SR
                state->regs.SR.all =
                    MappedMemoryReadLong(state->regs.R[n]) & 0x000003F3;
                state->regs.R[n] += 4;
                state->cycles += 2;
                break;
              case 1:  // LDC.L @Rn+,GBR
                state->regs.GBR = MappedMemoryReadLong(state->regs.R[n]);
                state->regs.R[n] += 4;
                state->cycles += 2;
                break;
              case 2:  // LDC.L @Rn+,VBR
                state->regs.VBR = MappedMemoryReadLong(state->regs.R[n]);
                state->regs.R[n] += 4;
                state->cycles += 2;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx7

          case 0x8: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLL2 Rn
                state->regs.R[n] <<= 2;
                break;
              case 0x1:  // SHLL8 Rn
                state->regs.R[n] <<= 8;
                break;
              case 0x2:  // SHLL16 Rn
                state->regs.R[n] <<= 16;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx8

          case 0x9: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // SHLR2 Rn
                state->regs.R[n] >>= 2;
                break;
              case 0x1:  // SHLR8 Rn
                state->regs.R[n] >>= 8;
                break;
              case 0x2:  // SHLR16 Rn
                state->regs.R[n] >>= 16;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xx9

          case 0xA: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDS Rn,MACH
                state->regs.MACH = state->regs.R[n];
                break;
              case 1:  // LDS Rn,MACL
                state->regs.MACL = state->regs.R[n];
                break;
              case 2:  // LDS Rn,PR
                state->regs.PR = state->regs.R[n];
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxA

          case 0xB: {
            switch (opcode>>4 & 0xF) {
              case 0x0:  // JSR @Rn
                state->regs.PR = state->regs.PC + 2;
                state->delay = state->regs.R[n];
                state->cycles += 1;
                break;
              case 0x1: {  // TAS.B @Rn
                uint8_t value = MappedMemoryReadByte(state->regs.R[n]);
                state->regs.SR.part.T = (value == 0);
                value |= 0x80;
                MappedMemoryWriteByte(state->regs.R[n], value);
                state->cycles += 3;
                break;
              }
              case 0x2:  // JMP @Rn
                state->delay = state->regs.R[n];
                state->cycles += 1;
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxB

          case 0xE: {
            switch (opcode>>4 & 0xF) {
              case 0:  // LDC Rn,SR
                state->regs.SR.all = state->regs.R[n] & 0x000003F3;
                break;
              case 1:  // LDC Rn,GBR
                state->regs.GBR = state->regs.R[n];
                break;
              case 2:  // LDC Rn,VBR
                state->regs.VBR = state->regs.R[n];
                break;
              default:
                goto invalid;
            }
            break;
          }  // $4xxE

          case 0xF: {  // MAC.W @Rm+,@Rn+
            int16_t value_m = MappedMemoryReadWord(state->regs.R[m]);
            state->regs.R[m] += 2;
            int16_t value_n = MappedMemoryReadWord(state->regs.R[n]);
            state->regs.R[n] += 2;
            int32_t product = (int32_t)value_m * (int32_t)value_n;
            int32_t sum = state->regs.MACL + product;
            if (state->regs.SR.part.S) {
                if ((int32_t)state->regs.MACL < 0 && product < 0 && sum >= 0) {
                    sum = -0x80000000;
                    state->regs.MACH |= 1;
                } else if ((int32_t)state->regs.MACL >= 0 && product >= 0 && sum < 0) {
                    sum = 0x7FFFFFFF;
                    state->regs.MACH |= 1;
                }
            } else {
                if (product < 0 && (uint32_t)sum >= (uint32_t)product) {
                    state->regs.MACH--;
                } else if (product > 0 && (uint32_t)sum < (uint32_t)product) {
                    state->regs.MACH++;
                }
            }
            state->regs.MACL = sum;
            state->cycles += 2;
            break;
          }

          default:
            goto invalid;
        }
        break;
      }  // $4xxx

      case 0x5: {  // MOV.L @(disp,Rm),Rn
        int disp = opcode & 0xF;
        uint32_t address = state->regs.R[m] + disp*4;
        state->regs.R[n] = MappedMemoryReadLong(address);
        break;
      }  // $5xxx

      case 0x6: {
        switch (opcode & 0xF) {
          case 0x0:  // MOV.B @Rm,Rn
            state->regs.R[n] =
                (int32_t)(int8_t)MappedMemoryReadByte(state->regs.R[m]);
            break;
          case 0x1:  // MOV.W @Rm,Rn
            state->regs.R[n] =
                (int32_t)(int16_t)MappedMemoryReadWord(state->regs.R[m]);
            break;
          case 0x2:  // MOV.L @Rm,Rn
            state->regs.R[n] = MappedMemoryReadLong(state->regs.R[m]);
            break;
          case 0x3:  // MOV Rm,Rn
            state->regs.R[n] = state->regs.R[m];
            break;
          case 0x4:  // MOV.B @Rm+,Rn
            state->regs.R[n] =
                (int32_t)(int8_t)MappedMemoryReadByte(state->regs.R[m]);
            state->regs.R[m] += 1;
            break;
          case 0x5:  // MOV.W @Rm+,Rn
            state->regs.R[n] =
                (int32_t)(int16_t)MappedMemoryReadWord(state->regs.R[m]);
            state->regs.R[m] += 2;
            break;
          case 0x6:  // MOV.L @Rm+,Rn
            state->regs.R[n] = MappedMemoryReadLong(state->regs.R[m]);
            state->regs.R[m] += 4;
            break;
          case 0x7:  // NOT Rm,Rn
            state->regs.R[n] = ~state->regs.R[m];
            break;
          case 0x8: {  // SWAP.B Rm,Rn
            uint32_t result = state->regs.R[m] & 0xFFFF0000;
            result |= (state->regs.R[m] & 0xFF) << 8;
            result |= (state->regs.R[m] >> 8) & 0xFF;
            state->regs.R[n] = result;
            break;
          }
          case 0x9:  // SWAP.W Rm,Rn
            state->regs.R[n] = state->regs.R[m]<<16 | state->regs.R[m]>>16;
            break;
          case 0xA: {  // NEGC Rm,Rn
            state->regs.R[n] = -state->regs.R[m];
            int newT = (state->regs.R[n] != 0);
            state->regs.R[n] -= state->regs.SR.part.T;
            state->regs.SR.part.T = newT;
            break;
          }
          case 0xB:  // NEG Rm,Rn
            state->regs.R[n] = -state->regs.R[m];
            break;
          case 0xC:  // EXTU.B Rm,Rn
            state->regs.R[n] = (uint32_t)(uint8_t)state->regs.R[m];
            break;
          case 0xD:  // EXTU.W Rm,Rn
            state->regs.R[n] = (uint32_t)(uint16_t)state->regs.R[m];
            break;
          case 0xE:  // EXTS.B Rm,Rn
            state->regs.R[n] = (int32_t)(int8_t)state->regs.R[m];
            break;
          case 0xF:  // EXTS.W Rm,Rn
            state->regs.R[n] = (int32_t)(int16_t)state->regs.R[m];
            break;
          default:
            goto invalid;
        }
        break;
      }  // $6xxx

      case 0x7: {  // ADD #imm,Rn
        int8_t imm = opcode & 0xFF;
        state->regs.R[n] += (int32_t)imm;
        break;
      }  // $7xxx

      case 0x8: {
        switch (opcode>>8 & 0xF) {
          case 0x0: {  // MOV.B R0,@(disp,Rm)
            int disp = opcode & 0xF;
            uint32_t address = state->regs.R[m] + disp;
            MappedMemoryWriteByte(address, state->regs.R[0]);
            break;
          }
          case 0x1: {  // MOV.W R0,@(disp,Rm)
            int disp = opcode & 0xF;
            uint32_t address = state->regs.R[m] + disp*2;
            MappedMemoryWriteWord(address, state->regs.R[0]);
            break;
          }
          case 0x4: {  // MOV.B @(disp,Rm),R0
            int disp = opcode & 0xF;
            uint32_t address = state->regs.R[m] + disp;
            state->regs.R[0] = (int32_t)(int8_t)MappedMemoryReadByte(address);
            break;
          }
          case 0x5: {  // MOV.W @(disp,Rm),R0
            int disp = opcode & 0xF;
            uint32_t address = state->regs.R[m] + disp*2;
            state->regs.R[0] = (int32_t)(int16_t)MappedMemoryReadWord(address);
            break;
          }
          case 0x8: {  // CMP/EQ #imm,R0
            int8_t imm = opcode & 0xFF;
            state->regs.SR.part.T = (state->regs.R[0] == (int32_t)imm);
            break;
          }
          case 0x9:  // BT label
            if (state->regs.SR.part.T) {
                int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
                state->regs.PC = (state->regs.PC+2) + disp;
                state->cycles += 2;
            }
            break;
          case 0xB:  // BF label
            if (!state->regs.SR.part.T) {
                int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
                state->regs.PC = (state->regs.PC+2) + disp;
                state->cycles += 2;
            }
            break;
          case 0xD:  // BT/S label
            if (state->regs.SR.part.T) {
                int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
                state->delay = (state->regs.PC+2) + disp;
                state->cycles += 1;
            }
            break;
          case 0xF:  // BF/S label
            if (!state->regs.SR.part.T) {
                int disp = ((int32_t)(opcode & 0xFF) << 24) >> 23;
                state->delay = (state->regs.PC+2) + disp;
                state->cycles += 1;
            }
            break;
          default:
            goto invalid;
        }
        break;
      }  // $8xxx

      case 0x9: {  // MOV.W @(disp,PC),Rn
        int disp = opcode & 0xFF;
        uint32_t address = (state->regs.PC + 2) + disp*2;
        state->regs.R[n] = (int32_t)(int16_t)MappedMemoryReadWord(address);
        break;
      }  // $9xxx

      case 0xA: {  // BRA label
        int disp = ((int32_t)(opcode & 0xFFF) << 20) >> 19;
        state->delay = (state->regs.PC+2) + disp;
        state->cycles += 1;
        break;
      }  // $Axxx

      case 0xB: {  // BSR label
        int disp = ((int32_t)(opcode & 0xFFF) << 20) >> 19;
        state->regs.PR = state->regs.PC + 2;
        state->delay = (state->regs.PC+2) + disp;
        state->cycles += 1;
        break;
      }  // $Bxxx

      case 0xC: {
        int imm = opcode & 0xFF;

        switch (opcode>>8 & 0xF) {
          case 0x0: {  // MOV.B R0,@(disp,GBR)
            uint32_t address = state->regs.GBR + imm;
            MappedMemoryWriteByte(address, state->regs.R[0]);
            break;
          }
          case 0x1: {  // MOV.W R0,@(disp,GBR)
            uint32_t address = state->regs.GBR + imm*2;
            MappedMemoryWriteWord(address, state->regs.R[0]);
            break;
          }
          case 0x2: {  // MOV.L R0,@(disp,GBR)
            uint32_t address = state->regs.GBR + imm*4;
            MappedMemoryWriteLong(address, state->regs.R[0]);
            break;
          }
          case 0x3: {  // TRAPA #imm
            state->regs.R[15] -= 4;
            MappedMemoryWriteLong(state->regs.R[15], state->regs.SR.all);
            state->regs.R[15] -= 4;
            MappedMemoryWriteLong(state->regs.R[15], state->regs.PC);
            state->regs.PC = imm<<2;
            state->cycles += 7;
            break;
          }
          case 0x4: {  // MOV.B @(disp,GBR),R0
            uint32_t address = state->regs.GBR + imm;
            state->regs.R[0] = (int8_t)(int32_t)MappedMemoryReadByte(address);
            break;
          }
          case 0x5: {  // MOV.W @(disp,GBR),R0
            uint32_t address = state->regs.GBR + imm*2;
            state->regs.R[0] = (int16_t)(int32_t)MappedMemoryReadWord(address);
            break;
          }
          case 0x6: {  // MOV.L @(disp,GBR),R0
            uint32_t address = state->regs.GBR + imm*4;
            state->regs.R[0] = MappedMemoryReadLong(address);
            break;
          }
          case 0x7: {  // MOVA @(disp,PC),R0
            state->regs.R[0] = ((state->regs.PC+2) & ~3) + imm*4;
            break;
          }
          case 0x8:  // TST #imm,R0
            state->regs.SR.part.T = ((state->regs.R[0] & imm) == 0);
            break;
          case 0x9:  // AND #imm,R0
            state->regs.R[0] &= imm;
            break;
          case 0xA:  // XOR #imm,R0
            state->regs.R[0] ^= imm;
            break;
          case 0xB:  // OR #imm,R0
            state->regs.R[0] |= imm;
            break;
          case 0xC: {  // TST.B #imm,@(R0,GBR)
            uint32_t address = state->regs.GBR + state->regs.R[0];
            uint8_t value = MappedMemoryReadByte(address);
            state->regs.SR.part.T = ((value & imm) == 0);
            state->cycles += 2;
            break;
          }
          case 0xD: {  // AND.B #imm,@(R0,GBR)
            uint32_t address = state->regs.GBR + state->regs.R[0];
            uint8_t value = MappedMemoryReadByte(address);
            value &= imm;
            MappedMemoryWriteByte(address, value);
            state->cycles += 2;
            break;
          }
          case 0xE: {  // XOR.B #imm,@(R0,GBR)
            uint32_t address = state->regs.GBR + state->regs.R[0];
            uint8_t value = MappedMemoryReadByte(address);
            value ^= imm;
            MappedMemoryWriteByte(address, value);
            state->cycles += 2;
            break;
          }
          case 0xF: {  // OR.B #imm,@(R0,GBR)
            uint32_t address = state->regs.GBR + state->regs.R[0];
            uint8_t value = MappedMemoryReadByte(address);
            value |= imm;
            MappedMemoryWriteByte(address, value);
            state->cycles += 2;
            break;
          }
          default:  // impossible
            goto invalid;
        }
        break;

      }  // $Cxxx

      case 0xD: {  // MOV.L @(disp,PC),Rn
        int disp = opcode & 0xFF;
        uint32_t address = ((state->regs.PC + 2) & ~3) + disp*4;
        state->regs.R[n] = MappedMemoryReadLong(address);
        break;
      }  // $Dxxx

      case 0xE: {  // MOV #imm,Rn
        int8_t imm = opcode & 0xFF;
        state->regs.R[n] = (int32_t)imm;
        break;
      }  // $Exxx

      case 0xF: {
        goto invalid;
      }  // $Fxxx

    }


#endif  // 0

/*************************************************************************/

/*
 * Local variables:
 *   mode: c
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
