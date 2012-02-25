#ifndef TITAN_H
#define TITAN_H

#include "../core.h"

#define TITAN_BLEND_TOP     0
#define TITAN_BLEND_BOTTOM  1

int TitanInit();
int TitanDeInit();

void TitanSetResolution(int width, int height);

u32 * TitanGetDispBuffer();

void TitanPutBackPixel(s32 x, s32 y, u32 color);
void TitanPutBackHLine(s32 x, s32 y, s32 width, u32 color);

void TitanPutLineHLine(int linescreen, s32 y, u32 color);

void TitanPutPixel(int priority, s32 x, s32 y, u32 color, int linescreen);
void TitanPutHLine(int priority, s32 x, s32 y, s32 width, u32 color);

void TitanRender(int blend_mode);

#endif
