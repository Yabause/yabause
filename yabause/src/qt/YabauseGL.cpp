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

YabauseGL::YabauseGL( QWidget* p )
  : QGLWidget(p)
{
	setFocusPolicy( Qt::StrongFocus );

  QGLFormat fmt;
  fmt.setProfile(QGLFormat::CompatibilityProfile);
  fmt.setSwapInterval(0);
  setFormat(fmt);

	if ( p ) {
		p->setFocusPolicy( Qt::StrongFocus );
		setFocusProxy( p );
	}
  viewport_width_ = 0;
  viewport_height_ = 0;
  viewport_origin_x_ = 0;
  viewport_origin_y_ = 0;
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

void YabauseGL::updateView( const QSize& s )
{
	const QSize size = s.isValid() ? s : this->size();
	//glViewport( 0, 0, viewport_width_, viewport_height_ );
	if ( VIDCore ) {
          VIDCore->Resize(0, 0, size.width(), size.height(), 0);
        }
}


extern "C"{
	int YuiRevokeOGLOnThisThread(){
		// Todo: needs to imp for async rendering
		return 0;
	}

	int YuiUseOGLOnThisThread(){
		// Todo: needs to imp for async rendering
		return 0;
	}
}
