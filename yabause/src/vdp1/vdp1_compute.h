#ifndef VDP1_COMPUTE_H
#define VDP1_COMPUTE_H

#include "vdp1_prog_compute.h"
#include "vdp1.h"

enum
{
  VDP1_MESH_STANDARD = 0,
  VDP1_MESH_IMPROVED,
  CLEAR,
  NB_PRG
};

extern void vdp1_compute_init(int width, int height, float ratiow, float ratioh);
extern int* vdp1_compute(Vdp2 *varVdp2Regs, int id);
extern int vdp1_add(vdp1cmd_struct* cmd, int clipcmd);
extern void vdp1_clear(int id);
extern void vdp1_setup(void);

#endif //VDP1_COMPUTE_H
