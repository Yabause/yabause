#include "../src/mini18n.h"
#include <stdio.h>

int main(int argc, char ** argv)
{
   if (argc < 3)
   {
      fprintf(stderr, "usage: resl10n resource.rc language\n");
      return 1;
   }

   mini18n_set_locale(argv[2]);

   {
      FILE * src;
      char buffer[1024];
      char outbuf[1024];
      char key[1024];
      unsigned int pos, opos;

      src = fopen(argv[1], "r");
      if (src == NULL)
      {
         fprintf(stderr, "cannot open %s\n", argv[1]);
         return 2;
      }

      while(fgets(buffer, 1024, src) != NULL)
      {
         char * c;
         int state;

         c = buffer;
         state = 0;
         opos = 0;
         while(*c)
         {
            switch(state)
            {
               case 0:
                  if (*c == '"')
                  {
                     pos = 0;
                     state = 1;
                  }
                  outbuf[opos++] = *c;
                  break;
               case 1:
                  if (*c == '"')
                  {
                     key[pos] = 0;
                     opos += sprintf(outbuf + opos, "%s", _(key));
                     state = 0;
                     outbuf[opos++] = *c;
                  }
                  else
                  {
                     key[pos++] = *c;
                  }
            }
            c++;
         }
         outbuf[opos] = 0;
         printf("%s", outbuf);
      }

      fclose(src);
   }
}
