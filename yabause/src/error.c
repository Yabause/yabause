#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "yui.h"

//////////////////////////////////////////////////////////////////////////////

void AllocAmendPrintString(const char *string1, const char *string2)
{
   char *string;

   if ((string = (char *)malloc(strlen(string1) + strlen(string2) + 2)) == NULL)
      return;

   sprintf(string, "%s%s\n", string1, string2);
   YuiErrorMsg(string);

   free(string);
}

//////////////////////////////////////////////////////////////////////////////

void YabSetError(int type, void *extra)
{
   switch (type)
   {
      case YAB_ERR_FILENOTFOUND:
         AllocAmendPrintString("File not found: ", extra);
         break;
      case YAB_ERR_MEMORYALLOC:
         YuiErrorMsg("Error allocating memory\n");
         break;
      case YAB_ERR_FILEREAD:
         AllocAmendPrintString("Error reading file: ", extra);
         break;
      case YAB_ERR_FILEWRITE:
         AllocAmendPrintString("Error writing file: ", extra);
         break;
      case YAB_ERR_CANNOTINIT:
         AllocAmendPrintString("Cannot initialize ", extra);
         break;
      case YAB_ERR_SH2INVALIDOPCODE:
         YuiErrorMsg("SH2 invalid opcode\n"); // fix me
/*
         fprintf(stderr, "R0 = %08X\tR12 = %08X\n", sh2opcodes->regs.R[0], sh2opcodes->regs.R[12]);
         fprintf(stderr, "R1 = %08X\tR13 = %08X\n", sh2opcodes->regs.R[1], sh2opcodes->regs.R[13]);
         fprintf(stderr, "R2 = %08X\tR14 = %08X\n", sh2opcodes->regs.R[2], sh2opcodes->regs.R[14]);
         fprintf(stderr, "R3 = %08X\tR15 = %08X\n", sh2opcodes->regs.R[3], sh2opcodes->regs.R[15]);
         fprintf(stderr, "R4 = %08X\tSR = %08X\n", sh2opcodes->regs.R[4], sh2opcodes->regs.SR.all);
         fprintf(stderr, "R5 = %08X\tGBR = %08X\n", sh2opcodes->regs.R[5], sh2opcodes->regs.GBR);
         fprintf(stderr, "R6 = %08X\tVBR = %08X\n", sh2opcodes->regs.R[6], sh2opcodes->regs.VBR);
         fprintf(stderr, "R7 = %08X\tMACH = %08X\n", sh2opcodes->regs.R[7], sh2opcodes->regs.MACH);
         fprintf(stderr, "R8 = %08X\tMACL = %08X\n", sh2opcodes->regs.R[8], sh2opcodes->regs.MACL);
         fprintf(stderr, "R9 = %08X\tPR = %08X\n", sh2opcodes->regs.R[9], sh2opcodes->regs.PR);
         fprintf(stderr, "R10 = %08X\tPC = %08X\n", sh2opcodes->regs.R[10], sh2opcodes->regs.PC);
         fprintf(stderr, "R11 = %08X\n", sh2opcodes->regs.R[11]);
*/
         break;
      case YAB_ERR_SH2READ:
         YuiErrorMsg("SH2 read error\n"); // fix me
         break;
      case YAB_ERR_SH2WRITE:
         YuiErrorMsg("SH2 write error\n"); // fix me
         break;
      case YAB_ERR_UNKNOWN:
      default:
         YuiErrorMsg("Unknown error occured\n");
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

