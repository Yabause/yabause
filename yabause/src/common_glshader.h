#ifndef COMMON_GLSHADER_H
#define COMMON_GLSHADER_H

#include "ygl.h"

extern const GLchar *vdp2blit_palette_mode_f[2];
extern const GLchar *vdp2blit_srite_type_f[16];
extern const GLchar *Yglprg_color_condition_f[5];
extern const GLchar *Yglprg_color_mode_f[4];

extern const GLchar *pYglprg_vdp2_blit_f[128*5][16];

extern GLuint _prgid[PG_MAX];

extern int YglInitShader(int id, const GLchar * vertex[], int vcount, const GLchar * frag[], int fcount, const GLchar * tc[], const GLchar * te[], const GLchar * g[] );
extern void initVDP2DrawCode(const GLchar* start, const GLchar* draw, const GLchar* end, const GLchar* final);
extern void compileVDP2Prog(int id, const GLchar *v, int CS);
extern int setupVDP2Prog(Vdp2* varVdp2Regs, int CS);
#endif //COMMON_GLSHADER_H
