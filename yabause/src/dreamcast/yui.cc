#include "yui.hh"

int stop;

static pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

        /* Vertex buffer size 512K */
        512*1024,
        
        /* DMA Enabled */
        1
};

uint8 dmabuffers[2][1024*1024] __attribute__((aligned(32)));

void yui_fps(int i) {
	fprintf(stderr, "%d\n", i);
}

char * yui_bios(void) {
	return "/cd/saturn.bin";
}

char * yui_cdrom(void) {
	return "blah";
}

void yui_hide_show(void) {
}

void yui_quit(void) {
	stop = 1;
}

void yui_init(int (*yab_main)(void*)) {
	SaturnMemory *mem;

	stop = 0;
	pvr_init(&params);
	glKosInit();
	glKosEnableDma();
  
	pvr_set_vertbuf(PVR_LIST_OP_POLY, dmabuffers[0], 1024*1024);
	pvr_set_vertbuf(PVR_LIST_TR_POLY, dmabuffers[1], 1024*1024);
  
	mem = new SaturnMemory();
	mem->start();

	while (!stop) yab_main(mem);
	delete(mem);
}
