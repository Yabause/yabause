/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004-2010 Lawrence Sebald

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

#include <stdio.h>

#include "../yui.h"
#include "../peripheral.h"
#include "../cs0.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vidsoft.h"
#include "../vdp2.h"
#ifdef _MSC_VER
#include <Windows.h>
#endif

#define AUTO_TEST_SELECT_ADDRESS 0x7F000
#define AUTO_TEST_STATUS_ADDRESS 0x7F004
#define AUTO_TEST_MESSAGE_ADDRESS 0x7F008
#define AUTO_TEST_MESSAGE_SENT 1
#define AUTO_TEST_MESSAGE_RECEIVED 2

#define VDP2_VRAM 0x25E00000

SH2Interface_struct *SH2CoreList[] = {
    &SH2Interpreter,
    NULL
};

PerInterface_struct *PERCoreList[] = {
    &PERDummy,
    NULL
};

CDInterface *CDCoreList[] = {
    &DummyCD,
    NULL
};

SoundInterface_struct *SNDCoreList[] = {
    &SNDDummy,
    NULL
};

VideoInterface_struct *VIDCoreList[] = {
    &VIDDummy,
    NULL
};

M68K_struct * M68KCoreList[] = {
    &M68KDummy,
#ifdef HAVE_C68K
    &M68KC68K,
#endif
#ifdef HAVE_Q68
    &M68KQ68,
#endif
    NULL
};

struct ConsoleColor
{
   char ansi[12];
   int windows;
};

struct ConsoleColor text_red = { "\033[22;31m",4 };
struct ConsoleColor text_green = { "\033[22;32m", 2 };
struct ConsoleColor text_white = { "\033[01;37m", 7 };

void set_color(struct ConsoleColor color)
{
#ifndef _MSC_VER
   printf("%s", color.ansi);
#else
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hConsole, (WORD)color.windows);
#endif
}

static const char *bios = "";
static int emulate_bios = 0;

void YuiErrorMsg(const char *error_text) {
   printf("\n\nError: %s\n", error_text);
   printf("                                 ");
}

void YuiSwapBuffers(void) {
}

//add tests that currently don't work in yabause here
char* tests_expected_to_fail[] =
{
   //slave sh2
   "SCU Mask Cache Quirk",
   "Slave VBlank",
   //scu interrupt
   "Timer 1 Interrupt",
   "Pad Interrupt",
   "Illegal DMA Interrupt",
   //scu dma
   "Misaligned DMA transfer",
   "Indirect DMA transfer",
   //scu dsp
   "DSP Execution",
   "MVI Imm, [d]",
   "DSP Timing",
   NULL
};

void read_second_part(char*source, char*dest)
{
   int pos = (int)strlen(source) + 1;
   sprintf(dest, "%s", Vdp2Ram + AUTO_TEST_MESSAGE_ADDRESS + pos);
}

void print_basic(char*message)
{
   read_second_part(message,message);
   printf("%s\n", message);
}

int find_test_expected_to_fail(char* test_name)
{
   int j = 0;
   for (j = 0; tests_expected_to_fail[j] != NULL; j++)
   {
      if (!strcmp(test_name, tests_expected_to_fail[j]))
      {
         return 1;
      }
   }

   return 0;
}

int main(int argc, char *argv[])
{
   yabauseinit_struct yinit = { 0 };
   int current_test = 0;
   char stored_test_name[256] = { 0 };
   char * filename = argv[1];

   struct Stats
   {
      int regressions;
      int total_tests;
      int tests_passed;
      int expected_failures;
   }stats = { 0 };

   if (!filename)
   {
      printf("No file specified.\n");
      return 0;
   }

   printf("Running tests...\n\n");

   yinit.percoretype = PERCORE_DUMMY;
   yinit.sh2coretype = SH2CORE_INTERPRETER;
   yinit.vidcoretype = VIDCORE_DUMMY;
   yinit.m68kcoretype = M68KCORE_DUMMY;
   yinit.sndcoretype = SNDCORE_DUMMY;
   yinit.cdcoretype = CDCORE_DUMMY;
   yinit.carttype = CART_NONE;
   yinit.regionid = REGION_AUTODETECT;
   yinit.biospath = emulate_bios ? NULL : bios;
   yinit.cdpath = NULL;
   yinit.buppath = NULL;
   yinit.mpegpath = NULL;
   yinit.cartpath = NULL;
   yinit.frameskip = 0;
   yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
   yinit.clocksync = 0;
   yinit.basetime = 0;
   yinit.skip_load = 1;

   if (YabauseInit(&yinit) != 0)
      return -1;

   MappedMemoryLoadExec(filename, 0);
   MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_SELECT_ADDRESS, current_test);

   for (;;)
   {
      int status = 0;

      PERCore->HandleEvents();

      status = MappedMemoryReadByte(VDP2_VRAM + AUTO_TEST_STATUS_ADDRESS);

      if (status == AUTO_TEST_MESSAGE_SENT)
      {
         char message[256] = { 0 };

         sprintf(message, "%s", Vdp2Ram + AUTO_TEST_MESSAGE_ADDRESS);

         if (!strcmp(message, "DEBUG_MESSAGE"))
         {
            //print a debug message
            print_basic(message);
         }
         else if (!strcmp(message, "SECTION_START"))
         {
            //print the name of the test section
            print_basic(message);
         }
         else if (!strcmp(message, "SECTION_END"))
         {
            //all sub-tests finished, proceed to next main test
            printf("\n");
            current_test++;

            YabauseDeInit();

            if (YabauseInit(&yinit) != 0)
               return -1;

            MappedMemoryLoadExec(filename, 0);
            MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_SELECT_ADDRESS, current_test);
         }
         else if (!strcmp(message, "SUB_TEST_START"))
         {
            //keep the test name for checking if it is a regression or not
            read_second_part(message, stored_test_name);
            printf("%-32s ", stored_test_name);
         }
         else if (!strcmp(message, "RESULT"))
         {
            char result_prefix[64] = { 0 };

            read_second_part(message,message);

            strncpy(result_prefix, message, 4);

            if (!strcmp(result_prefix, "PASS"))
            {
               //test was passed
               set_color(text_green);
               printf("PASS\n");
               set_color(text_white);
               stats.tests_passed++;
            }
            else if (!strcmp(result_prefix, "FAIL"))
            {
               //test failed
               set_color(text_red);

               if (find_test_expected_to_fail(stored_test_name))
               {
                  //test is not a regression
                  printf("FAIL");
                  set_color(text_green);
                  printf(" (Not a regression)\n");
                  stats.expected_failures++;
               }
               else
               {
                  //test is a regression
                  printf("FAIL\n");
                  stats.regressions++;
               }
               
               set_color(text_white);
            }
            else
            {
               printf("Unrecognized result prefix: %s\n",result_prefix);
            }

            stats.total_tests++;
         }
         else if (!strcmp(message, "ALL_FINISHED"))
         {
            //print stats and exit
            if (stats.regressions > 0)
            {
               set_color(text_red);
            }
            else
            {
               set_color(text_green);
            }

            printf("%d of %d tests passed. %d regressions. %d failures that are not regressions. \n", stats.tests_passed, stats.total_tests, stats.regressions, stats.expected_failures);

            set_color(text_white);

            break;
         }
         else
         {
            printf("Unrecognized message type: %s\n", message);
         }

         MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_STATUS_ADDRESS, AUTO_TEST_MESSAGE_RECEIVED);
      }
   }

   return stats.regressions;
}