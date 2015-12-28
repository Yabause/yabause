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
extern "C"
{
#include "../yui.h"
#include "../peripheral.h"
#include "../cs0.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vidsoft.h"
#include "../vdp2.h"
#include "../titan/titan.h"
#ifdef _MSC_VER
#include <Windows.h>
#endif

extern u8 *vdp1backframebuffer;
}

#include "lodepng/lodepng.h"
#include "lodepng/lodepng.cpp"

#include <string>

#define AUTO_TEST_SELECT_ADDRESS 0x7F000
#define AUTO_TEST_STATUS_ADDRESS 0x7F004
#define AUTO_TEST_MESSAGE_ADDRESS 0x7F008
#define AUTO_TEST_MESSAGE_SENT 1
#define AUTO_TEST_MESSAGE_RECEIVED 2

#define VDP2_VRAM 0x25E00000

extern "C"
{
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
      &VIDSoft,
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
}

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

void YuiErrorMsg(const char *error_text)
{
   printf("\n\nError: %s\n", error_text);
   printf("                                 ");
}

void YuiSwapBuffers(void)
{

}

//add tests that currently don't work in yabause here
const char* tests_expected_to_fail[] =
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
   "Clipping test",
   NULL
};

pixel_t runner_dispbuffer[704 * 512];

void read_second_part(char*source, char*dest)
{
   int pos = (int)strlen(source) + 1;
   sprintf(dest, "%s", Vdp2Ram + AUTO_TEST_MESSAGE_ADDRESS + pos);
}

void print_basic(char*message)
{
   read_second_part(message, message);
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

std::string make_screenshot_filename(std::string test_name, std::string path, int preset)
{
   char preset_str[64] = { 0 };
   sprintf(preset_str, "%d", preset);
   return path + test_name + " " + preset_str + ".png";
}

bool do_pixel_test(int x, int y, u32 test_color, u32 correct_color)
{
   if (test_color != correct_color)
   {
      set_color(text_red);
      printf("\nTest color was 0x%08x at x=%d y=%d. 0x%08x was expected.\n", test_color, x, y, correct_color);
      set_color(text_white);
      return false;
   }

   return true;
}

bool handle_screenshot(bool write_images, std::string test_name, std::string path, int preset)
{
   std::string screenshot_filename = make_screenshot_filename(test_name, path, preset);

   if (write_images)
   {
      int width = 0, height = 0;

      TitanGetResolution(&width, &height);
      TitanRender(runner_dispbuffer);

      unsigned error = lodepng::encode(screenshot_filename, (unsigned char*)runner_dispbuffer, width, height);

      if (error)
      {
         printf("error %u: %s\n", error, lodepng_error_text(error));
         return false;
      }
      else
      {
         printf("%s written.\n", screenshot_filename.c_str());
      }
   }
   else
   {
      std::vector<unsigned char> correct_image;
      std::vector<u32> correct_image_u32;

      unsigned correct_width, correct_height;

      unsigned error = lodepng::decode(correct_image, correct_width, correct_height, screenshot_filename);

      if (error)
      {
         printf("error %u: %s\n", error, lodepng_error_text(error));
         return false;
      }

      int test_width = 0, test_height = 0;

      TitanGetResolution(&test_width, &test_height);
      TitanRender(runner_dispbuffer);

      bool check_failed = false;

      if (test_width != correct_width)
      {
         printf("Vdp2 width was %d. %d was expected.\n", test_width, correct_width);
      }

      if (test_height != correct_height)
      {
         printf("Vdp2 height was %d. %d was expected.\n", test_height, correct_height);
      }

      correct_image_u32.resize(correct_width*correct_height);

      int j = 0;

      for (unsigned int i = 0; i < correct_width*correct_height * 4; i += 4)
      {
         correct_image_u32[j] = (correct_image[i + 3] << 24) | (correct_image[i + 2] << 16) | (correct_image[i + 1] << 8) | correct_image[i + 0];
         j++;
      }

      for (unsigned int y = 0; y < correct_height; y++)
      {
         for (unsigned int x = 0; x < correct_width; x++)
         {
            u32 correct_color = correct_image_u32[(y * correct_width) + x];
            u32 test_color = runner_dispbuffer[(y * correct_width) + x];

            bool result = do_pixel_test(x, y, test_color, correct_color);

            if (!result)
               return false;
         }
      }

      if (!check_failed)
      {
         return true;
      }
   }

   return false;
}

bool handle_framebuffer(char * stored_test_name, std::string framebuffer_path)
{
   std::string filename = framebuffer_path + stored_test_name + ".bin";
   u16 correct_framebuffer[0x20000];
   u16 correct_framebuffer_swapped[0x20000];

   unsigned int width = 320;
   unsigned int height = 224;

   FILE * fp = fopen(filename.c_str(), "rb");

   fread(correct_framebuffer, sizeof(u16), 0x20000, fp);

   fclose(fp);

   for (int i = 0; i < 0x20000; i++)
   {
      correct_framebuffer_swapped[i] = ((correct_framebuffer[i] & 0xff) << 8) | ((correct_framebuffer[i] >> 8) & 0xff);
   }

   for (int y = 0; y < height; y++)
   {
      for (int x = 0; x < height; x++)
      {
         int pos = ((y * 320) + x) * 2;
         u16 correct_pixel = correct_framebuffer_swapped[((y * 320) + x)];
         u16 yabause_pixel = (vdp1backframebuffer[pos+1] << 8) | (vdp1backframebuffer[pos] & 0xff);

         bool result = do_pixel_test(x, y, yabause_pixel, correct_pixel);

         if (!result)
            return false;
      }
   }

   return true;
}

int go_to_next_test(int &current_test, char* filename, yabauseinit_struct yinit)
{
   current_test++;

   YabauseDeInit();

   if (YabauseInit(&yinit) != 0)
      return -1;

   MappedMemoryLoadExec(filename, 0);
   MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_SELECT_ADDRESS, current_test);

   return 1;
}

struct Stats
{
   int regressions;
   int total_tests;
   int tests_passed;
   int expected_failures;
   struct {
      int matches;
      int diffs;
      int total;
   }screenshot;
};

void do_test_pass(struct Stats & stats, const char * message)
{
   //test was passed
   set_color(text_green);
   printf("%s\n", message);
   set_color(text_white);
}

void do_test_fail(struct Stats & stats, char* stored_test_name)
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

void do_regression_color(int regressions)
{
   if (regressions > 0)
      set_color(text_red);
   else
      set_color(text_green);
}

int main(int argc, char *argv[])
{
   yabauseinit_struct yinit = { 0 };
   int current_test = 0;
   char stored_test_name[256] = { 0 };
   char * filename = argv[1];
   char * screenshot_path = argv[2];
   char * framebuffer_path = argv[3];

   struct Stats stats = { 0 };

   if (!filename)
   {
      printf("No file specified.\n");
      return 0;
   }

   if (!screenshot_path)
   {
      printf("No screenshot path specified.\n");
      return 0;
   }

   if (!framebuffer_path)
   {
      printf("No framebuffer path specified.\n");
      return 0;
   }

   printf("Running tests...\n\n");

   yinit.percoretype = PERCORE_DUMMY;
   yinit.sh2coretype = SH2CORE_INTERPRETER;
   yinit.vidcoretype = VIDCORE_SOFT;
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

   bool write_images = false;

   std::string screenshot_filename = "";

   int screenshot_preset = 0;
   bool is_screenshot = false;

   for (;;)
   {
      int status = 0;

      //emulate a frame
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
         else if (!strcmp(message, "SCREENSHOT"))
         {
            stats.screenshot.total++;

            if (handle_screenshot(write_images, stored_test_name, screenshot_path, screenshot_preset))
            {
               //screenshot matches
               if (!write_images)
               {
                  printf("Preset %-25d ", screenshot_preset);
                  do_test_pass(stats, "Match");
                  stats.screenshot.matches++;
               }
            }
            else
            {
               //doesn't match
               if (!write_images)
               {
                  stats.screenshot.diffs++;
               }
            }

            screenshot_preset++;
         }
         else if (std::string(message) == "FRAMEBUFFER")
         {
            
            bool result = handle_framebuffer(stored_test_name,framebuffer_path);

            if(!result)
               do_test_fail(stats, stored_test_name);

            stats.total_tests++;
         }
         else if (!strcmp(message, "SECTION_START"))
         {
            //print the name of the test section
            print_basic(message);

            if (std::string(message) == "Vdp2 screenshot tests")
            {
               is_screenshot = true;
            }
         }
         else if (!strcmp(message, "SECTION_END"))
         {
            //all sub-tests finished, proceed to next main test
            printf("\n");

            YabauseDeInit();

            if (YabauseInit(&yinit) != 0)
               return -1;

            MappedMemoryLoadExec(filename, 0);
            MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_SELECT_ADDRESS, current_test);

            go_to_next_test(current_test, filename, yinit);

            is_screenshot = false;
         }
         else if (!strcmp(message, "SUB_TEST_START"))
         {
            //keep the test name for checking if it is a regression or not
            read_second_part(message, stored_test_name);

            screenshot_preset = 0;

            if(is_screenshot)
               printf("\n");

            if (!write_images)
            {
               if (is_screenshot)
               {
                  printf("%-32s \n", stored_test_name);
               }
               else
               {
                  printf("%-32s ", stored_test_name);
               }
            }

            if (is_screenshot)
            {
               screenshot_filename = make_screenshot_filename(stored_test_name, screenshot_path, screenshot_preset);
            }
         }
         else if (!strcmp(message, "RESULT"))
         {
            char result_prefix[64] = { 0 };

            read_second_part(message, message);

            strncpy(result_prefix, message, 4);

            if (!strcmp(result_prefix, "PASS"))
            {
               do_test_pass(stats,"PASS");
               stats.tests_passed++;
            }
            else if (!strcmp(result_prefix, "FAIL"))
            {
               do_test_fail(stats, stored_test_name);
            }
            else
            {
               printf("Unrecognized result prefix: %s\n", result_prefix);
            }

            stats.total_tests++;
         }
         else if (!strcmp(message, "ALL_FINISHED"))
         {
            //print stats and exit

            do_regression_color(stats.regressions);
            printf("%d of %d tests passed. %d regressions. %d failures that are not regressions. \n", stats.tests_passed, stats.total_tests, stats.regressions, stats.expected_failures);
            
            do_regression_color(stats.screenshot.diffs);
            printf("%d of %d screenshots matched. %d did not match. \n", stats.screenshot.matches, stats.screenshot.total, stats.screenshot.diffs);

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

   return stats.regressions || stats.screenshot.diffs;
}