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
#include "YabauseSoftGL.h"
#include "QtYabause.h"
#include "../vidsoft.h"

#include <QPainter>

YabauseSoftGL::YabauseSoftGL( QWidget* p )
	: QWidget( p )
{
	setFocusPolicy( Qt::StrongFocus );

	if ( p ) {
		p->setFocusPolicy( Qt::StrongFocus );
		setFocusProxy( p );
	}
}

void YabauseSoftGL::showEvent( QShowEvent* e )
{
}

void YabauseSoftGL::resizeGL( int w, int h )
{ updateView( QSize( w, h ) ); }

void YabauseSoftGL::updateView( const QSize& s )
{
}

void YabauseSoftGL::swapBuffers()
{
	this->update(this->rect());
}

QImage YabauseSoftGL::grabFrameBuffer(bool withAlpha)
{
	int buf_width, buf_height;

	if (dispbuffer == NULL) return QImage();

	VIDCore->GetGlSize( &buf_width, &buf_height );

#ifdef USE_RGB_555
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB555);
#elif USE_RGB_565
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB16);
#else
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB32);
	image = image.rgbSwapped();
#endif
	return image;
}

void YabauseSoftGL::paintEvent( QPaintEvent * event )
{
	int buf_width, buf_height;

	if (dispbuffer == NULL) return;

	VIDCore->GetGlSize( &buf_width, &buf_height );

#ifdef USE_RGB_555
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB555);
#elif USE_RGB_565
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB16);
#else
	QImage image = QImage((uchar *) dispbuffer, buf_width, buf_height, QImage::Format_RGB32);
	image = image.rgbSwapped();
#endif
	QPainter p(this);
	p.drawImage(this->rect(), image);
}

void YabauseSoftGL::makeCurrent()
{
}
