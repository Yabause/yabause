#include "YabauseGL.h"

extern "C"
{
	#include "../vdp1.h"
}

YabauseGL::YabauseGL( QWidget* parent )
	: QGLWidget( parent )
{}

YabauseGL::~YabauseGL()
{}

void YabauseGL::initializeGL()
{}

void YabauseGL::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );
	VIDCore->Resize( w, h, 0 );
}

void YabauseGL::paintGL()
{}
