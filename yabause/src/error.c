/*  Copyright 2005-2006 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
   char tempstr[512];
   SH2_struct *sh;

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
         sh = (SH2_struct *)extra;
         sprintf(tempstr, "%s SH2 invalid opcode\n\n"
                          "R0 =  %08X\tR12 =  %08X\n"
                          "R1 =  %08X\tR13 =  %08X\n"
                          "R2 =  %08X\tR14 =  %08X\n"
                          "R3 =  %08X\tR15 =  %08X\n"
                          "R4 =  %08X\tSR =   %08X\n"
                          "R5 =  %08X\tGBR =  %08X\n"
                          "R6 =  %08X\tVBR =  %08X\n"
                          "R7 =  %08X\tMACH = %08X\n"
                          "R8 =  %08X\tMACL = %08X\n"
                          "R9 =  %08X\tPR =   %08X\n"
                          "R10 = %08X\tPC =   %08X\n"
                          "R11 = %08X\n", sh->isslave ? "Slave" : "Master",
                          sh->regs.R[0], sh->regs.R[12],
                          sh->regs.R[1], sh->regs.R[13],
                          sh->regs.R[2], sh->regs.R[14],
                          sh->regs.R[3], sh->regs.R[15],
                          sh->regs.R[4], sh->regs.SR.all,
                          sh->regs.R[5], sh->regs.GBR,
                          sh->regs.R[6], sh->regs.VBR,
                          sh->regs.R[7], sh->regs.MACH,
                          sh->regs.R[8], sh->regs.MACL,
                          sh->regs.R[9], sh->regs.PR,
                          sh->regs.R[10], sh->regs.PC,
                          sh->regs.R[11]);
         YuiErrorMsg(tempstr);
         break;
      case YAB_ERR_SH2READ:
         YuiErrorMsg("SH2 read error\n"); // fix me
         break;
      case YAB_ERR_SH2WRITE:
         YuiErrorMsg("SH2 write error\n"); // fix me
         break;
      case YAB_ERR_SDL:
         AllocAmendPrintString("SDL Error: ", extra);
         break;
      case YAB_ERR_OTHER:
         YuiErrorMsg((char *)extra);
         break;
      case YAB_ERR_UNKNOWN:
      default:
         YuiErrorMsg("Unknown error occured\n");
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

