#ifndef __SCANLINE_INCLUDE_H__
#define __SCANLINE_INCLUDE_H__
static const GLchar Yglprg_blit_scanline_f[] =
"    float alpha = cos(3.1415*lineNumber[1]/decim * (vTexCoord.y+1.0)/2.0);\n" //Analog ray of CRT looks like a square sine
"    alpha = 0.5 + (alpha*alpha)/2.0;\n"
"    fragColor = vec4(fragColor.xyz*alpha, 1.0);\n";
#endif
