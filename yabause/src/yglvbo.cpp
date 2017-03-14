/*
Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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

#ifdef USEVBO

extern "C" {
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
}

extern "C" {

class CVboPool
{
private:
    GLuint _vertexBuffer;
    void * _pMapBuffer;
    int _initsize;

    unsigned char * m_pMemBlock;                //The address of memory pool.
    unsigned long m_ulBlockSize;

    void * prepos;

    unsigned long currentpos;
    unsigned long tc_startpos;
    unsigned long va_startpos;

public:
    CVboPool(unsigned long size);
    ~CVboPool();

    int alloc(unsigned long size, void ** vpos, void ** tcpos, void ** vapos  ); //Allocate memory unit
    int expand( unsigned long addsize,void ** vpos, void **tcpos, void **vapos  );                                   //Free memory unit

    void unMap();
    void reMap();
	GLuint getVboId(){ return _vertexBuffer; }
    intptr_t getOffset( void* address ); //{ return address-(intptr_t)m_pMemBlock; }
};

CVboPool::CVboPool(unsigned long size )
{
    m_ulBlockSize = size * ( sizeof(int)*2 + sizeof(float)*4 + sizeof(float)*4 ) ;
    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, m_ulBlockSize,NULL,GL_DYNAMIC_DRAW);
    m_pMemBlock = (unsigned char *)glMapBufferRange(GL_ARRAY_BUFFER, 0, m_ulBlockSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    currentpos = 0;

    tc_startpos = size * ( sizeof(int)*2);
    va_startpos = tc_startpos + (size*sizeof(float)*4) ;

}

void CVboPool::unMap()
{
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);

}

void CVboPool::reMap()
{
    void *p;
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    p = glMapBufferRange(GL_ARRAY_BUFFER, 0, m_ulBlockSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    if( p != m_pMemBlock )
    {
        printf("???\n");
    }
    currentpos = 0;
    prepos = NULL;
}


CVboPool::~CVboPool()
{
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glDeleteBuffers(1,&_vertexBuffer);
}

int CVboPool::alloc(unsigned long size, void ** vpos, void ** tcpos, void ** vapos  )
{
    if( (currentpos* ( sizeof(int)))  >= tc_startpos ||
        tc_startpos + (currentpos*sizeof(float)*2)  >= va_startpos ||
        va_startpos + (currentpos*sizeof(float)*2) >= m_ulBlockSize )
    {
        printf("bad alloc %X,%X,%d\n",prepos,*vpos,size);
        return -1;
    }

    *vpos = m_pMemBlock + (currentpos* ( sizeof(int)*2)) ;
    *tcpos = m_pMemBlock + tc_startpos + (currentpos*sizeof(float)*4) ;
    *vapos = m_pMemBlock + va_startpos + (currentpos*sizeof(float)*4) ;
    prepos = *vpos;
    currentpos += size;
//    printf("alloc %X,%X,%X,%X,%d,%d\n",*vpos,*tcpos,*vapos,prepos,size,currentpos);
    return 0;
}

int CVboPool::expand( unsigned long addsize,void ** vpos, void **tcpos, void **vapos  )
{
    if( (currentpos += addsize) >= tc_startpos || tc_startpos + currentpos + addsize >= va_startpos || va_startpos + currentpos + addsize >= m_ulBlockSize )
    {

        printf("bad expand %X,%X,%d\n",prepos,*vpos,addsize);
        return -1;
    }

    // OverWitten!
    if( *vpos != prepos )
    {
        int a=0;
        printf("bad expand %X,%X,%d\n",prepos,*vpos,addsize);
    }else{
        currentpos += addsize;
//        printf("expand %X,%X,%d,%d\n",prepos,*vpos,addsize,currentpos);
    }
}

intptr_t CVboPool::getOffset( void* address )
{
//    printf("getOffset %X-%X=%X\n",address,m_pMemBlock,(intptr_t)address-(intptr_t)m_pMemBlock);
    return (intptr_t)address-(intptr_t)m_pMemBlock;
}

CVboPool * g_pool;

int YglInitVertexBuffer( int initsize ) {
    g_pool = new CVboPool(initsize);
	return 0;
}

void YglDeleteVertexBuffer()
{
    delete g_pool;
}

int YglUnMapVertexBuffer() {
    g_pool->unMap();
    //printf("================= unMap ====================\n");
	return 0;
}

int YglMapVertexBuffer() {
    g_pool->reMap();
    //printf("================= reMap ====================\n");
	return 0;
}

int YglGetVertexBuffer( int size, void ** vpos, void **tcpos, void **vapos )
{
    return g_pool->alloc(size,vpos,tcpos,vapos);
}

int YglExpandVertexBuffer( int addsize, void ** vpos, void **tcpos, void **vapos )
{
    g_pool->expand(addsize,vpos,tcpos,vapos);
	return 0;
}

int YglUserDirectVertexBuffer()
{
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 return 0;
}

int YglUserVertexBuffer()
{
	 glBindBuffer(GL_ARRAY_BUFFER, g_pool->getVboId() );
	 return 0;
}

intptr_t YglGetOffset( void* address )
{
    return g_pool->getOffset(address);
}


}

#endif
