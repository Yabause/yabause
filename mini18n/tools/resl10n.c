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
               case 0: /* initial state */
                  switch(*c)
                  {
                     case '"':
                        pos = 0;
                        state = 1;
                        break;
                     case '#':
                        pos = 0;
                        state = 2;
                        break;
                  }
                  outbuf[opos++] = *c;
                  break;
               case 1: /* reading string */
                  if (*c == '"')
                  {
                     key[pos] = 0;
                     opos += sprintf(outbuf + opos, "%s", _(key));
                     state = 0;
                     outbuf[opos++] = *c;
                  }
                  else
                     key[pos++] = *c;
                  break;
               case 2: /* preprocessor directive */
                  if (*c == ' ')
                  {
                     key[pos] = 0;
                     opos += sprintf(outbuf + opos, "%s", key);
                     if (!strcmp(key, "pragma")) 
                     {
                        pos = 0;
                        state = 3;
                     }
                     else
                        state = 0;
                     outbuf[opos++] = *c;
                  }
                  else
                     key[pos++] = *c;
                  break;
               case 3: /* pragma directive */
                  if (*c == '(')
                  {
                     key[pos] = 0;
                     opos += sprintf(outbuf + opos, "%s", key);
                     if (!strcmp(key, "code_page"))
                        state = 4;
                     else
                        state = 0;
                     outbuf[opos++] = *c;
                  }
                  else
                     key[pos++] = *c;
                  break;
               case 4: /* code_page */
                  if (*c == ')')
                  {
                     opos += sprintf(outbuf + opos, "%d", 65001);
                     state = 0;
                     outbuf[opos++] = *c;
                  }
                  break;
            }
            c++;
         }
         outbuf[opos] = 0;
         printf("%s", outbuf);
      }

      fclose(src);
   }
}
