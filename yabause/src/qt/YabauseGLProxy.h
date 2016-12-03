/*	Copyright 2016 Guillaume Duhamel

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
#ifndef YABAUSEGLPROXY_H
#define YABAUSEGLPROXY_H

#include <QWidget>

class YabauseGL;
class YabauseSoftGL;

class YabauseGLProxy : public QObject
{
	Q_OBJECT

protected:
	int mImpl;
	YabauseGL * mYabauseGL;
	YabauseSoftGL * mYabauseSoft;

public:
	static const int DEFAULT;
	static const int OPENGL;
	static const int SOFTWARE;

	YabauseGLProxy( QWidget* parent = 0, int impl = DEFAULT );

	void updateView( const QSize& size = QSize() );
	virtual void swapBuffers();
	QImage grabFrameBuffer(bool withAlpha = false);
	void makeCurrent();
	void setMouseTracking(bool track);
	void resize(int width, int height);

	QWidget * getWidget();
	void select(QWidget * parent, int impl);
};

#endif // YABAUSEGLPROXY_H
