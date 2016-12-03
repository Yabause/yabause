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

#include "YabauseGLProxy.h"
#include "YabauseSoftGL.h"

const int YabauseGLProxy::DEFAULT = 0;
const int YabauseGLProxy::OPENGL = 1;
const int YabauseGLProxy::SOFTWARE = 2;

#ifdef HAVE_LIBGL
#include "YabauseGL.h"

YabauseGLProxy::YabauseGLProxy( QWidget* parent, int impl )
{
	mImpl = -1;
	select( parent, impl );
}

void YabauseGLProxy::updateView( const QSize& size )
{
	if (mImpl == YabauseGLProxy::SOFTWARE) mYabauseSoft->updateView(size);
	else mYabauseGL->updateView(size);
}

void YabauseGLProxy::swapBuffers()
{
	if (mImpl == YabauseGLProxy::SOFTWARE) mYabauseSoft->swapBuffers();
	else mYabauseGL->swapBuffers();
}

QImage YabauseGLProxy::grabFrameBuffer(bool withAlpha)
{

#if defined(HAVE_LIBGL) && !defined(QT_OPENGL_ES_1) && !defined(QT_OPENGL_ES_2)
	glReadBuffer(GL_FRONT);
#endif

	if (mImpl == YabauseGLProxy::SOFTWARE) return mYabauseSoft->grabFrameBuffer(withAlpha);
	else return mYabauseGL->grabFrameBuffer(withAlpha);
}

void YabauseGLProxy::makeCurrent()
{
	if (mImpl == YabauseGLProxy::SOFTWARE) mYabauseSoft->makeCurrent();
	else mYabauseGL->makeCurrent();
}

QWidget * YabauseGLProxy::getWidget()
{
	if (mImpl == YabauseGLProxy::SOFTWARE) return mYabauseSoft;
	else return mYabauseGL;
}

void YabauseGLProxy::setMouseTracking(bool track)
{
	if (mImpl == YabauseGLProxy::SOFTWARE) mYabauseSoft->setMouseTracking(track);
	else mYabauseGL->setMouseTracking(track);
}

void YabauseGLProxy::resize(int width, int height)
{
	if (mImpl == YabauseGLProxy::SOFTWARE) mYabauseSoft->resize(width, height);
	else mYabauseGL->resize(width, height);
}

void YabauseGLProxy::select(QWidget * parent, int impl)
{
	if (impl == YabauseGLProxy::DEFAULT)
	{
#ifdef HAVE_LIBGL
		impl = YabauseGLProxy::OPENGL;
#else
		impl = YabauseGLProxy::SOFTWARE;
#endif
	}

	if (impl == mImpl) return;

	mImpl = impl;
	switch(impl)
	{
		case YabauseGLProxy::SOFTWARE:
			mYabauseSoft = new YabauseSoftGL(parent);
			break;
		case YabauseGLProxy::OPENGL:
		default:
			mYabauseGL = new YabauseGL(parent);
			break;
	}
}

#else

YabauseGLProxy::YabauseGLProxy( QWidget* parent, int impl )
{
	mYabauseSoft = new YabauseSoftGL(parent);
}

void YabauseGLProxy::updateView( const QSize& size )
{
	mYabauseSoft->updateView(size);
}

void YabauseGLProxy::swapBuffers()
{
	mYabauseSoft->swapBuffers();
}

QImage YabauseGLProxy::grabFrameBuffer(bool withAlpha)
{
	return mYabauseSoft->grabFrameBuffer(withAlpha);
}

void YabauseGLProxy::makeCurrent()
{
	mYabauseSoft->makeCurrent();
}

QWidget * YabauseGLProxy::getWidget()
{
	return mYabauseSoft;
}

void YabauseGLProxy::setMouseTracking(bool track)
{
	mYabauseSoft->setMouseTracking(track);
}

void YabauseGLProxy::resize(int width, int height)
{
	mYabauseSoft->resize(width, height);
}

void YabauseGLProxy::select(QWidget * parent, int impl)
{
}

#endif
