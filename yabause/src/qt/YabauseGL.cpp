/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

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
#include "YabauseGL.h"
#include "QtYabause.h"
#include <cassert>

YabauseGL::YabauseGL( QWidget* p )
	: QGLWidget( p ),
	  lastRenderedScreen(nullptr),
	  overlayPauseImage(nullptr)
{
	setFocusPolicy( Qt::StrongFocus );
	
	if ( p ) {
		p->setFocusPolicy( Qt::StrongFocus );
		setFocusProxy( p );
	}
	
	constexpr size_t arraySizes = 704 * 512 * sizeof(uint32_t);
	lastRenderedScreen = new uint32_t[ arraySizes ];
	overlayPauseImage = new uint32_t[ arraySizes ];
	memset(lastRenderedScreen, 0, arraySizes);
	memset(overlayPauseImage, 0, arraySizes);
	
	paused = false;
}
	
YabauseGL::~YabauseGL()
{
	delete [] lastRenderedScreen;
	delete [] overlayPauseImage;
}

void YabauseGL::showEvent( QShowEvent* e )
{
	// hack for clearing the the gl context
	QGLWidget::showEvent( e );
	QSize s = size();
	resize( 0, 0 );
	resize( s );
}

void YabauseGL::resizeGL( int w, int h )
{ updateView( QSize( w, h ) ); }

void YabauseGL::snapshotView()
{
  int width, height, interlace;
  VIDCore->GetNativeResolution( &width, &height, &interlace );

	const size_t totalSize = static_cast<size_t>(width) * static_cast<size_t>(height) *
		sizeof(uint32_t);

  memcpy( lastRenderedScreen, dispbuffer, totalSize );
	memset( overlayPauseImage, 0, totalSize );
}

void YabauseGL::updatePausedView( const QSize& s )
{
	// Paint static image
  int width, height, interlace;
  VIDCore->GetNativeResolution( &width, &height, &interlace );

	glViewport( 0, 0, s.width(), s.height() );
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		lastRenderedScreen);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	QMutexLocker lock(&overlayMutex);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		overlayPauseImage);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);

  swapBuffers();
}

void YabauseGL::paintGL()
{
	if (paused)
		updatePausedView( size() );
}

void YabauseGL::setPaused( bool isPaused )
{ paused = isPaused; }

QPair<uint32_t*, QMutex*> YabauseGL::getOverlayPausedImage()
{
	return QPair<uint32_t*, QMutex*>(overlayPauseImage, &overlayMutex);
}

void YabauseGL::updateView( const QSize& s )
{
	const QSize size = s.isValid() ? s : this->size();
	glViewport( 0, 0, size.width(), size.height() );
	if ( VIDCore )
		VIDCore->Resize( size.width(), size.height(), 0 );

	if ( paused )
		updatePausedView( s );
}
