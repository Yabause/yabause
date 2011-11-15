/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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

#ifdef HAVE_LIBGL
#include <stdlib.h>
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

YglTextureManager * YglTM;
Ygl * _Ygl;

typedef struct
{
   u32 id;
   YglCache c;
} cache_struct;

static cache_struct *cachelist;
static int cachelistsize=0;

typedef struct
{
   float s, t, r, q;
} texturecoordinate_struct;

#define STD_Q2 (1.0f)
#define EPS (1e-10)
#define EQ(a,b) (abs((a)-(b)) < EPS)
#define IS_ZERO(a) ( (a) < EPS && (a) > -EPS)

// AXB = |A||B|sin
INLINE float cross2d( float veca[2], float vecb[2] )
{
	return (veca[0]*vecb[1])-(vecb[0]*veca[1]);
}

/*-----------------------------------------
    b1+--+ a1
     /  / \
    /  /   \
  a2+-+-----+b2
      ans
      
  get intersection point for opssite edge.
--------------------------------------------*/  
int FASTCALL YglIntersectionOppsiteEdge(float * a1, float * a2, float * b1, float * b2, float * out ) 
{
  float veca[2];
  float vecb[2];
  float vecc[2];
  float d1;
  float d2;

  veca[0]=a2[0]-a1[0];
  veca[1]=a2[1]-a1[1];
  vecb[0]=b1[0]-a1[0];
  vecb[1]=b1[1]-a1[1];
  vecc[0]=b2[0]-a1[0];
  vecc[1]=b2[1]-a1[1];
  d1 = cross2d(vecb,vecc);
  if( IS_ZERO(d1) ) return -1;
  d2 = cross2d(vecb,veca);
  
  out[0] = a1[0]+vecc[0]*d2/d1;
  out[1] = a1[1]+vecc[1]*d2/d1;
 
  return 0;
}


int YglCalcTextureQ(
	int	*pnts,
	float *q	
)
{
	float p1[2],p2[2],p3[2],p4[2],o[2];
	float	q1, q3, q4, qw;
	float	x, y;
	float	dx, w;
	float	b;
	float	ww;
	
	// fast calculation for triangle
	if (( pnts[2*0+0] == pnts[2*1+0] ) && ( pnts[2*0+1] == pnts[2*1+1] )) {
		q[0] = 1.0f;
		q[1] = 1.0f;
		q[2] = 1.0f;
		q[3] = 1.0f;
		return 0;
		
	} else if (( pnts[2*1+0] == pnts[2*2+0] ) && ( pnts[2*1+1] == pnts[2*2+1] ))  {
		q[0] = 1.0f;
		q[1] = 1.0f;
		q[2] = 1.0f;
		q[3] = 1.0f;
		return 0;
	} else if (( pnts[2*2+0] == pnts[2*3+0] ) && ( pnts[2*2+1] == pnts[2*3+1] ))  {
		q[0] = 1.0f;
		q[1] = 1.0f;
		q[2] = 1.0f;
		q[3] = 1.0f;
		return 0;
	} else if (( pnts[2*3+0] == pnts[2*0+0] ) && ( pnts[2*3+1] == pnts[2*0+1] )) {
		q[0] = 1.0f;
		q[1] = 1.0f;
		q[2] = 1.0f;
		q[3] = 1.0f;
		return 0;
	}

	p1[0]=pnts[0];
	p1[1]=pnts[1];
	p2[0]=pnts[2];
	p2[1]=pnts[3];
	p3[0]=pnts[4];
	p3[1]=pnts[5];
	p4[0]=pnts[6];
	p4[1]=pnts[7];

	// calcurate Q1
	if( YglIntersectionOppsiteEdge( p3, p1, p2, p4,  o ) == 0 )
	{
		dx = o[0]-p1[0];
		if( !IS_ZERO(dx) )
		{
			w = p3[0]-p2[0];
			if( !IS_ZERO(w) )
			 q1 = fabs(dx/w);
			else
			 q1 = 0.0f;
		}else{
			w = p3[1] - p2[1];
			if ( !IS_ZERO(w) ) 
			{
				ww = ( o[1] - p1[1] );
				if ( !IS_ZERO(ww) )
					q1 = fabs(ww / w);
				else
					q1 = 0.0f;
			} else {
				q1 = 0.0f;
			}			
		}
	}else{
		q1 = 1.0f;
	}

	/* q2 = 1.0f; */

	// calcurate Q3
	if( YglIntersectionOppsiteEdge( p1, p3, p2,p4,  o ) == 0 )
	{
		dx = o[0]-p3[0];
		if( !IS_ZERO(dx) )
		{
			w = p1[0]-p2[0];
			if( !IS_ZERO(w) )
			 q3 = fabs(dx/w);
			else
			 q3 = 0.0f;
		}else{
			w = p1[1] - p2[1];
			if ( !IS_ZERO(w) ) 
			{
				ww = ( o[1] - p3[1] );
				if ( !IS_ZERO(ww) )
					q3 = fabs(ww / w);
				else
					q3 = 0.0f;
			} else {
				q3 = 0.0f;
			}			
		}
	}else{
		q3 = 1.0f;
	}

	
	// calcurate Q4
	if( YglIntersectionOppsiteEdge( p3, p1, p4, p2,  o ) == 0 )
	{
		dx = o[0]-p1[0];
		if( !IS_ZERO(dx) )
		{
			w = p3[0]-p4[0];
			if( !IS_ZERO(w) )
			 qw = fabs(dx/w);
			else
			 qw = 0.0f;
		}else{
			w = p3[1] - p4[1];
			if ( !IS_ZERO(w) ) 
			{
				ww = ( o[1] - p1[1] );
				if ( !IS_ZERO(ww) )
					qw = fabs(ww / w);
				else
					qw = 0.0f;
			} else {
				qw = 0.0f;
			}			
		}
		if ( !IS_ZERO(qw) )
		{
			w	= qw / q1;
		}
		else
		{
			w	= 0.0f;
		}
		if ( IS_ZERO(w) ) {
			q4 = 1.0f;
		} else {
			q4 = 1.0f / w;
		}		
	}else{
		q4 = 1.0f;
	}

	qw = q1;
	if ( qw < 1.0f )	/* q2 = 1.0f */
		qw = 1.0f;
	if ( qw < q3 )
		qw = q3;
	if ( qw < q4 )
		qw = q4;

	if ( 1.0f != qw )
	{
		qw		= 1.0f / qw;

		q[0]	= q1 * qw;
		q[1]	= 1.0f * qw;
		q[2]	= q3 * qw;
		q[3]	= q4 * qw;
	}
	else
	{
		q[0]	= q1;
		q[1]	= 1.0f;
		q[2]	= q3;
		q[3]	= q4;
	}
	return 0;
}



//////////////////////////////////////////////////////////////////////////////

void YglTMInit(unsigned int w, unsigned int h) {
   YglTM = (YglTextureManager *) malloc(sizeof(YglTextureManager));
   YglTM->texture = (unsigned int *) malloc(sizeof(unsigned int) * w * h);
   YglTM->width = w;
   YglTM->height = h;

   YglTMReset();
}

//////////////////////////////////////////////////////////////////////////////

void YglTMDeInit(void) {
   free(YglTM->texture);
   free(YglTM);
}

//////////////////////////////////////////////////////////////////////////////

void YglTMReset(void) {
   YglTM->currentX = 0;
   YglTM->currentY = 0;
   YglTM->yMax = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglTMAllocate(YglTexture * output, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y) {
   if ((YglTM->height - YglTM->currentY) < h) {
      fprintf(stderr, "can't allocate texture: %dx%d\n", w, h);
      *x = *y = 0;
      output->w = 0;
      output->textdata = YglTM->texture;
      return;
   }

   if ((YglTM->width - YglTM->currentX) >= w) {
      *x = YglTM->currentX;
      *y = YglTM->currentY;
      output->w = YglTM->width - w;
      output->textdata = YglTM->texture + YglTM->currentY * YglTM->width + YglTM->currentX;
      YglTM->currentX += w;
      if ((YglTM->currentY + h) > YglTM->yMax)
         YglTM->yMax = YglTM->currentY + h;
   }
   else {
      YglTM->currentX = 0;
      YglTM->currentY = YglTM->yMax;
      YglTMAllocate(output, w, h, x, y);
   }
}

//////////////////////////////////////////////////////////////////////////////

int YglGLInit(int width, int height) {
   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 320, 224, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-width, width, -height, height, 1, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glGenTextures(1, &_Ygl->texture);
   glBindTexture(GL_TEXTURE_2D, _Ygl->texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, YglTM->texture);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YglScreenInit(int r, int g, int b, int d) {
   YuiSetVideoAttribute(RED_SIZE, r);
   YuiSetVideoAttribute(GREEN_SIZE, g);
   YuiSetVideoAttribute(BLUE_SIZE, b);
   YuiSetVideoAttribute(DEPTH_SIZE, d);
   return (YuiSetVideoMode(320, 224, 32, 0) == 0);
}

//////////////////////////////////////////////////////////////////////////////

int YglInit(int width, int height, unsigned int depth) {
   unsigned int i;

   VideoInitGlut();

   YglTMInit(width, height);

   if ((_Ygl = (Ygl *) malloc(sizeof(Ygl))) == NULL)
      return -1;
   _Ygl->depth = depth;
   if ((_Ygl->levels = (YglLevel *) malloc(sizeof(YglLevel) * depth)) == NULL)
      return -1;
   for(i = 0;i < depth;i++) {
      _Ygl->levels[i].currentQuad = 0;
      _Ygl->levels[i].maxQuad = 8 * 2000;
#ifdef USEMICSHADERS
      _Ygl->levels[i].currentColors = 0;
      _Ygl->levels[i].maxColors = 16 * 2000;
#endif
      if ((_Ygl->levels[i].quads = (int *) malloc(_Ygl->levels[i].maxQuad * sizeof(int))) == NULL)
         return -1;

      if ((_Ygl->levels[i].textcoords = (float *) malloc(_Ygl->levels[i].maxQuad * sizeof(float) * 2)) == NULL)
         return -1;

#ifdef USEMICSHADERS
      if ((_Ygl->levels[i].colors = (unsigned char *) malloc(_Ygl->levels[i].maxColors * sizeof(unsigned char))) == NULL)
         return -1;
#endif
   }

   YuiSetVideoAttribute(DOUBLEBUFFER, 1);

   if (!YglScreenInit(8, 8, 8, 24))
   {
      if (!YglScreenInit(4, 4, 4, 24))
      {
         if (!YglScreenInit(5, 6, 5, 16))
         {
	    YuiErrorMsg("Couldn't set GL mode\n");
            return -1;
         }
      }
   }

   YglGLInit(width, height);

   _Ygl->st = 0;
   _Ygl->msglength = 0;

   // This is probably wrong, but it'll have to do for now
   if ((cachelist = (cache_struct *)malloc(0x100000 / 8 * sizeof(cache_struct))) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglDeInit(void) {
   unsigned int i;

   YglTMDeInit();

   if (_Ygl)
   {
      if (_Ygl->levels)
      {
         for (i = 0; i < _Ygl->depth; i++)
         {
            if (_Ygl->levels[i].quads)
               free(_Ygl->levels[i].quads);
            if (_Ygl->levels[i].textcoords)
               free(_Ygl->levels[i].textcoords);
         }
         free(_Ygl->levels);
      }

      free(_Ygl);
   }

   if (cachelist)
      free(cachelist);
}

//////////////////////////////////////////////////////////////////////////////

float * YglQuad(YglSprite * input, YglTexture * output, YglCache * c) {
   unsigned int x, y;
   YglLevel *level;
   texturecoordinate_struct *tmp;
   float q[4];

   if (input->priority > 7) {
      VDP1LOG("sprite with priority %d\n", input->priority);
      return NULL;
   }

   level = &_Ygl->levels[input->priority];

   if (level->currentQuad == level->maxQuad) {
      level->maxQuad += 8;
      level->quads = (int *) realloc(level->quads, level->maxQuad * sizeof(int));
      level->textcoords = (float *) realloc(level->textcoords, level->maxQuad * sizeof(float) * 2);
      YglCacheReset();
   }

#ifdef USEMICSHADERS
   if (level->currentColors == level->maxColors) {
      level->maxColors += 16;
      level->colors = (unsigned char *) realloc(level->colors, level->maxColors * sizeof(unsigned char));
      YglCacheReset();
   }
#endif

   tmp = (texturecoordinate_struct *)(level->textcoords + (level->currentQuad * 2));

   memcpy(level->quads + level->currentQuad, input->vertices, 8 * sizeof(int));
#ifdef USEMICSHADERS
   memcpy(level->colors + level->currentColors, noMeshGouraud, 16 * sizeof(unsigned char));
#endif

   level->currentQuad += 8;
#ifdef USEMICSHADERS
   level->currentColors += 16;
#endif
   YglTMAllocate(output, input->w, input->h, &x, &y);

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( c != NULL )
   {
	   switch(input->flip) {
		  case 0:
			 c->x = *(level->textcoords + ((level->currentQuad - 8) * 2));   // upper left coordinates(0)
			 c->y = *(level->textcoords + ((level->currentQuad - 8) * 2)+1); // upper left coordinates(0)	
			 break;
		  case 1:
			 c->x = *(level->textcoords + ((level->currentQuad - 6) * 2));   // upper left coordinates(0)
			 c->y = *(level->textcoords + ((level->currentQuad - 6) * 2)+1); // upper left coordinates(0)		 
			 break;
		 case 2:
			 c->x = *(level->textcoords + ((level->currentQuad - 2) * 2));   // upper left coordinates(0)
			 c->y = *(level->textcoords + ((level->currentQuad - 2) * 2)+1); // upper left coordinates(0)		 
			 break;
		 case 3:
			 c->x = *(level->textcoords + ((level->currentQuad - 4) * 2));   // upper left coordinates(0)
			 c->y = *(level->textcoords + ((level->currentQuad - 4) * 2)+1); // upper left coordinates(0)		 
			 break;
	   }
   }

   
   if( input->dst == 1 )
   {
	YglCalcTextureQ(input->vertices,q);
	tmp[0].s *= q[0];
	tmp[0].t *= q[0];
	tmp[1].s *= q[1];
	tmp[1].t *= q[1];
	tmp[2].s *= q[2];
	tmp[2].t *= q[2];
	tmp[3].s *= q[3];
	tmp[3].t *= q[3];
	tmp[0].q = q[0]; 
	tmp[1].q = q[1]; 
	tmp[2].q = q[2]; 
	tmp[3].q = q[3]; 
   }else{
	tmp[0].q = 1.0f; 
	tmp[1].q = 1.0f; 
	tmp[2].q = 1.0f; 
	tmp[3].q = 1.0f; 
   }


   return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifdef USEMICSHADERS
int YglQuad2(YglSprite * input, YglTexture * output, YglColor * colors,YglCache * c) {
   unsigned int x, y;
   YglLevel *level;
   texturecoordinate_struct *tmp;

   level = &_Ygl->levels[input->priority];

   if (level->currentQuad == level->maxQuad) {
      level->maxQuad += 8;
      level->quads = (int *) realloc(level->quads, level->maxQuad * sizeof(int));
      level->textcoords = (int *) realloc(level->textcoords, level->maxQuad * sizeof(float));
      YglCacheReset();
   }

   if (level->currentColors == level->maxColors)
   {
	   level->maxColors += 16;
	   level->colors = (unsigned char *) realloc(level->colors, level->maxColors * sizeof(unsigned char));
	   YglCacheReset();
   }

   tmp = (texturecoordinate_struct *)(level->textcoords + (level->currentQuad * 2));

   memcpy(level->quads + level->currentQuad, input->vertices, 8 * sizeof(int));
   memcpy(level->colors + level->currentColors, colors, 16 * sizeof(unsigned char));

   level->currentQuad += 8;
   level->currentColors += 16;

   YglTMAllocate(output, input->w, input->h, &x, &y);

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

    switch(input->flip) {
      case 0:
         c->x = *(level->textcoords + ((level->currentQuad - 8) * 2));   // upper left coordinates(0)
         c->y = *(level->textcoords + ((level->currentQuad - 8) * 2)+1); // upper left coordinates(0)	
		 break;
      case 1:
         c->x = *(level->textcoords + ((level->currentQuad - 6) * 2));   // upper left coordinates(0)
         c->y = *(level->textcoords + ((level->currentQuad - 6) * 2)+1); // upper left coordinates(0)		 
		 break;
	 case 2:
         c->x = *(level->textcoords + ((level->currentQuad - 2) * 2));   // upper left coordinates(0)
         c->y = *(level->textcoords + ((level->currentQuad - 2) * 2)+1); // upper left coordinates(0)		 
		 break;
	 case 3:
         c->x = *(level->textcoords + ((level->currentQuad - 4) * 2));   // upper left coordinates(0)
         c->y = *(level->textcoords + ((level->currentQuad - 4) * 2)+1); // upper left coordinates(0)		 
		 break;
   }

    if( input->dst == 1 )
   {
	YglCalcTextureQ(input->vertices,q);
	tmp[0].s *= q[0];
	tmp[0].t *= q[0];
	tmp[1].s *= q[1];
	tmp[1].t *= q[1];
	tmp[2].s *= q[2];
	tmp[2].t *= q[2];
	tmp[3].s *= q[3];
	tmp[3].t *= q[3];

	tmp[0].q = q[0]; 
	tmp[1].q = q[1]; 
	tmp[2].q = q[2]; 
	tmp[3].q = q[3]; 
	
   }else{
	tmp[0].q = 1.0f; 
	tmp[1].q = 1.0f; 
	tmp[2].q = 1.0f; 
	tmp[3].q = 1.0f; 
   }

   return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////

void YglCachedQuad(YglSprite * input, YglCache * cache) {
   YglLevel * level = _Ygl->levels + input->priority;
   unsigned int x,y;
   texturecoordinate_struct *tmp;
	float q[4];

   x = cache->x;
   y = cache->y;

   if (level->currentQuad == level->maxQuad) {
      level->maxQuad += 8;
      level->quads = (int *) realloc(level->quads, level->maxQuad * sizeof(int));
      level->textcoords = (int *) realloc(level->textcoords, level->maxQuad * sizeof(float) * 2);
      YglCacheReset();
   }

#ifdef USEMICSHADERS
   if (level->currentColors == level->maxColors)
   {
      level->maxColors += 16;
      level->colors = (unsigned char *) realloc(level->colors, level->maxColors * sizeof(unsigned char));
      YglCacheReset();
   }
#endif

   tmp = (texturecoordinate_struct *)(level->textcoords + (level->currentQuad * 2));

   memcpy(level->quads + level->currentQuad, input->vertices, 8 * sizeof(int));
#ifdef USEMICSHADERS
   memcpy(level->colors + level->currentColors, noMeshGouraud, 16 * sizeof(unsigned char));
#endif

   level->currentQuad += 8;
#ifdef USEMICSHADERS
   level->currentColors += 16;
#endif

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0

  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( input->dst == 1 )
   {
		YglCalcTextureQ(input->vertices,q);
		tmp[0].s *= q[0];
	tmp[0].t *= q[0];
	tmp[1].s *= q[1];
	tmp[1].t *= q[1];
	tmp[2].s *= q[2];
	tmp[2].t *= q[2];
	tmp[3].s *= q[3];
	tmp[3].t *= q[3];
	tmp[0].q = q[0]; 
	tmp[1].q = q[1]; 
	tmp[2].q = q[2]; 
	tmp[3].q = q[3]; 
   }else{
	tmp[0].q = 1.0f; 
	tmp[1].q = 1.0f; 
	tmp[2].q = 1.0f; 
	tmp[3].q = 1.0f; 
   }

   
}

//////////////////////////////////////////////////////////////////////////////

#ifdef USEMICSHADERS
void YglCachedQuad2(YglSprite * input, int * cache, YglColor * colors) {
   YglLevel * level = _Ygl->levels + input->priority;
   unsigned int x,y;
   texturecoordinate_struct *tmp;

   x = cache->x;
   y = cache->y;

   if (level->currentQuad == level->maxQuad) {
      level->maxQuad += 8;
      level->quads = (int *) realloc(level->quads, level->maxQuad * sizeof(int));
      level->textcoords = (float *) realloc(level->textcoords, level->maxQuad * sizeof(float));
      YglCacheReset();
   }

   if (level->currentColors == level->maxColors)
   {
	   level->maxColors += 16;
	   level->colors = (unsigned char *) realloc(level->colors, level->maxColors * sizeof(unsigned char));
	   YglCacheReset();
   }

   tmp = (texturecoordinate_struct *)(level->textcoords + (level->currentQuad * 2));

   memcpy(level->quads + level->currentQuad, input->vertices, 8 * sizeof(int));
   memcpy(level->colors + level->currentColors, colors, 16 * sizeof(unsigned char));

   level->currentQuad += 8;
   level->currentColors += 16;

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = 0; // these can stay at 0
	
  if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = (float)(x + input->w);
      tmp[1].s = tmp[2].s = (float)(x);
   } else {
      tmp[0].s = tmp[3].s = (float)(x);
      tmp[1].s = tmp[2].s = (float)(x + input->w);
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = (float)(y + input->h);
      tmp[2].t = tmp[3].t = (float)(y);
   } else {
      tmp[0].t = tmp[1].t = (float)(y);
      tmp[2].t = tmp[3].t = (float)(y + input->h);
   }

   if( input->dst == 1 )
   {
	YglCalcTextureQ(input->vertices,q);
	tmp[0].s *= q[0];
	tmp[0].t *= q[0];
	tmp[1].s *= q[1];
	tmp[1].t *= q[1];
	tmp[2].s *= q[2];
	tmp[2].t *= q[2];
	tmp[3].s *= q[3];
	tmp[3].t *= q[3];
	tmp[0].q = q[0]; 
	tmp[1].q = q[1]; 
	tmp[2].q = q[2]; 
	tmp[3].q = q[3]; 
   }else{
	tmp[0].q = 1.0f; 
	tmp[1].q = 1.0f; 
	tmp[2].q = 1.0f; 
	tmp[3].q = 1.0f; 
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////

void YglRender(void) {
   YglLevel * level;

   glEnable(GL_TEXTURE_2D);
#ifdef USEMICSHADERS
   glShadeModel(GL_SMOOTH);
#endif

   glBindTexture(GL_TEXTURE_2D, _Ygl->texture);

   glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, YglTM->width, YglTM->yMax, GL_RGBA, GL_UNSIGNED_BYTE, YglTM->texture);

#ifdef USEMICSHADERS
   if (useShaders)
   {
     glEnableClientState(GL_COLOR_ARRAY);
     pfglUseProgram(shaderProgram);
   }
#endif

   if(_Ygl->st) {
      int vertices [] = { 0, 0, 320, 0, 320, 224, 0, 224 };
      int text [] = { 0, 0, YglTM->width, 0, YglTM->width, YglTM->height, 0, YglTM->height };
      glVertexPointer(2, GL_INT, 0, vertices);
#ifdef USEMICSHADERS
      // FIXME: this needs to be defined  --AC
      //glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
#endif
      glTexCoordPointer(4, GL_INT, 0, text);
      glDrawArrays(GL_QUADS, 0, 4);
   } else {
      unsigned int i;
      for(i = 0;i < _Ygl->depth;i++) {
         level = _Ygl->levels + i;
         glVertexPointer(2, GL_INT, 0, level->quads);
#ifdef USEMICSHADERS
         glColorPointer(4, GL_UNSIGNED_BYTE, 0, level->colors);
#endif
         glTexCoordPointer(4, GL_FLOAT, 0, level->textcoords);
         glDrawArrays(GL_QUADS, 0, level->currentQuad / 2);
      }
   }

#ifdef USEMICSHADERS
   if (useShaders)
   {
      glDisableClientState(GL_COLOR_ARRAY);
      pfglUseProgram(0);
   }
#endif

   glDisable(GL_TEXTURE_2D);
#ifndef _arch_dreamcast
#if HAVE_LIBGLUT
   if (_Ygl->msglength > 0) {
      int i;
      glColor3f(1.0f, 0.0f, 0.0f);
      glRasterPos2i(10, 22);
      for (i = 0; i < _Ygl->msglength; i++) {
         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, _Ygl->message[i]);
      }
      glColor3f(1, 1, 1);
   }
#endif
#endif

   YuiSwapBuffers();
}

//////////////////////////////////////////////////////////////////////////////

void YglReset(void) {
   YglLevel * level;
   unsigned int i;

   glClear(GL_COLOR_BUFFER_BIT);

   YglTMReset();

   for(i = 0;i < _Ygl->depth;i++) {
      level = _Ygl->levels + i;
      level->currentQuad = 0;
   }
   _Ygl->msglength = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglShowTexture(void) {
   _Ygl->st = !_Ygl->st;
}

//////////////////////////////////////////////////////////////////////////////

void YglChangeResolution(int w, int h) {
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, h, 0, 1, 0);
}

//////////////////////////////////////////////////////////////////////////////

void YglOnScreenDebugMessage(char *string, ...) {
   va_list arglist;

   va_start(arglist, string);
   vsprintf(_Ygl->message, string, arglist);
   va_end(arglist);
   _Ygl->msglength = (int)strlen(_Ygl->message);
}

//////////////////////////////////////////////////////////////////////////////

int YglIsCached(u32 addr, YglCache * c ) {
   int i = 0;

   for (i = 0; i < cachelistsize; i++)
   {
      if (addr == cachelist[i].id)
	  {
         c->x=cachelist[i].c.x;
		 c->y=cachelist[i].c.y;
		 return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheAdd(u32 addr, YglCache * c) {
   cachelist[cachelistsize].id = addr;
   cachelist[cachelistsize].c.x = c->x;
   cachelist[cachelistsize].c.y = c->y;
   cachelistsize++;
}

//////////////////////////////////////////////////////////////////////////////

void YglCacheReset(void) {
   cachelistsize = 0;
}

//////////////////////////////////////////////////////////////////////////////

#endif

