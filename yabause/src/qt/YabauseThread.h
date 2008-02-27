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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef YABAUSETHREAD_H
#define YABAUSETHREAD_H

#include <QThread>
#include <QSize>
#include <QMutexLocker>

#include "QtYabause.h"

class YabauseThread : public QObject
{
	Q_OBJECT
	
public:
	YabauseThread( QObject* owner = 0 );
	
	yabauseinit_struct* yabauseConf();
	bool emulationRunning();
	bool emulationPaused();

protected:
	yabauseinit_struct mYabauseConf;
	QMutex mMutex;
	bool mPause;
	bool mRunning;
	int mTimerId;
	int mInit;
	
	void initEmulation();
	void deInitEmulation();
	void resetYabauseConf();
	void timerEvent( QTimerEvent* );

public slots:
	void startEmulation();
	void stopEmulation();
	
	void runEmulation();
	void pauseEmulation();
	void resetEmulation( bool fr = false );
	void reloadSettings();

signals:
	void requestSize( const QSize& size );
	void requestFullscreen( bool fullscreen );
};

#endif // YABAUSETHREAD_H
