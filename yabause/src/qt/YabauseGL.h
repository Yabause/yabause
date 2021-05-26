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

#include <QGLWidget>
#include <QtCore/qmutex.h>
#include <QPair>

class YabauseGL : public QGLWidget
{
	Q_OBJECT
	
public:
	YabauseGL( QWidget* parent = 0 );
	~YabauseGL();
	
	void updatePausedView( const QSize& size = QSize() );
	void updateView( const QSize& size = QSize() );
	void snapshotView();

	void paintGL() override;
	void setPaused(bool isPaused);
	QPair<uint32_t*, QMutex*> getOverlayPausedImage();

protected:
	bool paused;
	uint32_t* lastRenderedScreen;
	uint32_t* overlayPauseImage;
	QMutex overlayMutex;

	virtual void showEvent( QShowEvent* event );
	virtual void resizeGL( int w, int h );
};

#endif // YABAUSEGL_H
