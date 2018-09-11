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
#ifndef YABAUSEGL_H
#define YABAUSEGL_H

#ifdef HAVE_LIBGL
#include <QOpenGLWindow>
#include <QOpenGLFunctions>

class YabauseGL : public QOpenGLWindow, protected QOpenGLFunctions
#else
#include <QWidget>
#include <QImage>


class YabauseGL : public QWidget
#endif
{
	Q_OBJECT
	
public:
	YabauseGL( );
	
	void updateView( const QSize& size = QSize() );
	void swapBuffers();
#ifndef HAVE_LIBGL
        QImage grabFrameBuffer();
	virtual void paintEvent( QPaintEvent * event );
	void makeCurrent();
#endif

protected:
        void initializeGL() override;
        void resizeGL(int width, int height) override;
        void keyPressEvent( QKeyEvent* e ) override;
        void keyReleaseEvent( QKeyEvent* e ) override;
};

#endif // YABAUSEGL_H
