#include "YabauseGL.h"

extern "C"
{
	#include "../vdp1.h"
}

YabauseGL::YabauseGL( QWidget* parent )
	: QGLWidget( parent )
{}

void YabauseGL::resizeGL( int w, int h )
{
	if ( VIDCore )
		VIDCore->Resize( w, h, 0 );
}
