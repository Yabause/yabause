#include "../yabause/src/snddummy.c"

#ifdef HAVE_THREADS
#include "thr-unix.c"
#else
#include "../yabause/src/thr-dummy.c"
#endif

#include "../yabause/src/titan/titan.c"
#include "../yabause/src/vdp1.c"
#include "../yabause/src/vdp2.c"
#include "../yabause/src/vidshared.c"
#include "../yabause/src/vidsoft.c"
#include "../yabause/src/yabause.c"

#include "libretro.c"
