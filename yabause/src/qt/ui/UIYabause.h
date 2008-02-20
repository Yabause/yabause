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
#ifndef UIYABAUSE_H
#define UIYABAUSE_H

#include "ui_UIYabause.h"
#include "../YabauseThread.h"

class YabauseGL;
class QTextEdit;
class QDockWidget;

class YabauseLocker
{
public:
	YabauseLocker( YabauseThread* yt/*, bool fr = false*/ )
	{
		Q_ASSERT( yt );
		mThread = yt;
		//mForceRun = fr;
		mRunning = mThread->emulationRunning();
		mPaused = mThread->emulationPaused();
		if ( mRunning && !mPaused )
			mThread->pauseEmulation();
	}
	~YabauseLocker()
	{
		if ( ( mRunning && !mPaused ) /*|| mForceRun*/ )
			mThread->runEmulation();
	}

protected:
	YabauseThread* mThread;
	bool mRunning;
	bool mPaused;
	//bool mForceRun;
};

class UIYabause : public QMainWindow, public Ui::UIYabause
{
	Q_OBJECT
	
public:
	UIYabause( QWidget* parent = 0 );
	~UIYabause();

	void swapBuffers();
	void appendLog( const char* msg );

protected:
	YabauseGL* mYabauseGL;
	YabauseThread* mYabauseThread;
	QDockWidget* mLogDock;
	QTextEdit* teLog;
	bool mInit;

	virtual void closeEvent( QCloseEvent* event );
	virtual void showEvent( QShowEvent* event );
	virtual void keyPressEvent( QKeyEvent* event );
	virtual void keyReleaseEvent( QKeyEvent* event );

protected slots:
	void sizeRequested( const QSize& size );
	void fullscreenRequested( bool fullscreen );
	// yabause menu
	void on_aYabauseSettings_triggered();
	void on_aYabauseRun_triggered();
	void on_aYabausePause_triggered();
	void on_aYabauseReset_triggered();
	void on_aYabauseTransfer_triggered();
	void on_aYabauseScreenshot_triggered();
	void on_aYabauseFrameSkipLimiter_triggered( bool b );
	void on_mYabauseSaveState_triggered( QAction* );
	void on_mYabauseLoadState_triggered( QAction* );
	void on_aYabauseSaveStateAs_triggered();
	void on_aYabauseLoadStateAs_triggered();
	void on_aYabauseQuit_triggered();
	// tools
	void on_aToolsBackupManager_triggered();
	void on_aToolsCheatsList_triggered();
	// view menu
	void on_aViewFPS_triggered();
	void on_aViewLayerVdp1_triggered();
	void on_aViewLayerNBG0_triggered();
	void on_aViewLayerNBG1_triggered();
	void on_aViewLayerNBG2_triggered();
	void on_aViewLayerNBG3_triggered();
	void on_aViewLayerRBG0_triggered();
	void on_aViewFullscreen_triggered( bool b );
	void on_aViewLog_triggered();
	// help menu
	void on_aHelpAbout_triggered();
};

#endif // UIYABAUSE_H
