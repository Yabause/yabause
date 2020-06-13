/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006, 2013 Theo Berkau
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
#ifndef UIYABAUSE_H
#define UIYABAUSE_H

#include "ui_UIYabause.h"
#include "../YabauseThread.h"
#include "UICheatSearch.h"
#include <QTimer>

extern "C" {
#include "../screen.h"
}

class YabauseGL;
class QTextEdit;
class QDockWidget;

enum BARDISPLAY
{
	BD_NEVERHIDE=0,
	BD_HIDEFS=1,
	BD_ALWAYSHIDE=2,
	BD_SHOWONFSHOVER=3
};

namespace firebase{
	class App;
}

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
			mThread->pauseEmulation( true, false );
	}
	~YabauseLocker()
	{
		if ( ( mRunning && !mPaused ) /*|| mForceRun*/ )
			mThread->pauseEmulation( false, false );
	}

protected:
	YabauseThread* mThread;
	bool mRunning;
	bool mPaused;
	//bool mForceRun;
};

#if defined Q_OS_WIN
struct SavedWinInfo {
  int style;
  int ex_style;
  QPoint windowspos;
  QSize windowsize;
};
#endif

class UIYabause : public QMainWindow, public Ui::UIYabause
{
	Q_OBJECT
	
public:
	UIYabause( QWidget* parent = 0 );
	~UIYabause();

	void swapBuffers();
	virtual bool eventFilter( QObject* o, QEvent* e );

	static firebase::App* getFirebaseApp(){
		return app;
	}

  void takeScreenshot(const char * fname);

signals: // [1]
  void setStateFileLoaded(std::string filename );

protected:
	static firebase::App* app;
	YabauseGL* mYabauseGL;
	YabauseThread* mYabauseThread;
	QDockWidget* mLogDock;
	QTextEdit* teLog;
	bool mCanLog;
	bool mInit;
	QList <cheatsearch_struct> search;
	int searchType;
	QList <supportedRes_struct> supportedResolutions;
	int oldMouseX, oldMouseY;
	bool mouseCaptured;

	float mouseXRatio, mouseYRatio;
	int mouseSensitivity;
	bool emulateMouse;
	int showMenuBarHeight;
	QTimer* hideMouseTimer;
	QTimer* mouseCursorTimer;
	QList <translation_struct> translations;
	virtual void showEvent( QShowEvent* event );
	virtual void closeEvent( QCloseEvent* event );
	virtual void keyPressEvent( QKeyEvent* event );
	virtual void keyReleaseEvent( QKeyEvent* event );
	virtual void leaveEvent(QEvent * event);
	virtual void mousePressEvent( QMouseEvent* event );
	virtual void mouseReleaseEvent( QMouseEvent* event );
	virtual void mouseMoveEvent( QMouseEvent* event );
	virtual void resizeEvent( QResizeEvent* event );

	bool mIsCdIn;

  void *state_buffer = nullptr;
  size_t state_size = 0;
  QString default_title;
  

#if defined Q_OS_WIN
  HWND hwnd_;
  SavedWinInfo saved_window_info_;
#endif

public slots:
	void appendLog( const char* msg );
	void pause( bool paused );
	void reset();
	void hideMouse();
	void cursorRestore();
	void toggleEmulateMouse( bool enable );

	void breakpointHandlerMSH2(bool displayMessage);
	void breakpointHandlerSSH2(bool displayMessage);
	void breakpointHandlerM68K();
	void breakpointHandlerSCUDSP();
	void breakpointHandlerSCSPDSP();

  void onStateFileLoaded();

protected slots:
	void errorReceived( const QString& error, bool internal = true );
	void sizeRequested( const QSize& size );
	void fixAspectRatio( int width, int height  );
	void getSupportedResolutions();
	int isResolutionValid( int width, int height, int bpp, int freq );
	int findBestVideoFreq( int width, int height, int bpp, int videoFormat );
	void toggleFullscreen( int width, int height, bool f, int videoFormat );
	void fullscreenRequested( bool fullscreen );
	void refreshStatesActions();
   void adjustHeight(int & height);
   void resizeIntegerScaling();
	// file menu
	void on_aFileSettings_triggered();
	void on_aFileOpenISO_triggered();
	void on_aFileOpenCDRom_triggered();
  void on_aFileOpenSSF_triggered();
  void on_actionOpen_Tray_triggered();
	void on_mFileSaveState_triggered( QAction* );
	void on_mFileLoadState_triggered( QAction* );
  void on_actionTo_Cloud_triggered();
  void on_actionFrom_Cloud_triggered();
	void on_aFileSaveStateAs_triggered();
	void on_aFileLoadStateAs_triggered();
	void on_aFileScreenshot_triggered();
	void on_aFileQuit_triggered();
	// emulation menu
	void on_aEmulationRun_triggered();
	void on_aEmulationPause_triggered();
	void on_aEmulationReset_triggered();
	void on_aEmulationFrameSkipLimiter_toggled( bool toggled );
  void on_actionRecord_triggered();
  void on_actionPlay_triggered();
	// tools
  void on_actionOpen_web_interface_triggered();
	void on_aToolsBackupManager_triggered();
	void on_aToolsCheatsList_triggered();
	void on_aToolsCheatSearch_triggered();
	void on_aToolsTransfer_triggered();
	// view menu
	void on_aViewFPS_triggered( bool toggled );
	void on_aViewLayerVdp1_triggered();
	void on_aViewLayerNBG0_triggered();
	void on_aViewLayerNBG1_triggered();
	void on_aViewLayerNBG2_triggered();
	void on_aViewLayerNBG3_triggered();
	void on_aViewLayerRBG0_triggered();
	void on_aViewFullscreen_triggered( bool b );
	// debug menu
	void on_aViewDebugMSH2_triggered();
	void on_aViewDebugSSH2_triggered();
	void on_aViewDebugVDP1_triggered();
	void on_aViewDebugVDP2_triggered();
	void on_aViewDebugM68K_triggered();
   void on_aViewDebugSCSP_triggered();
   void on_aViewDebugSCSPChan_triggered();
	void on_aViewDebugSCSPDSP_triggered();
	void on_aViewDebugSCUDSP_triggered();
	void on_aViewDebugMemoryEditor_triggered();
	void on_aTraceLogging_triggered( bool toggled );
	// help menu
	void on_aHelpReport_triggered();
	void on_aHelpCompatibilityList_triggered();
	void on_aHelpAbout_triggered();
  void on_actionDonate_triggered();
	// toolbar
	void on_aSound_triggered();
	void on_aVideoDriver_triggered();
	void on_cbSound_toggled( bool toggled );
	void on_sVolume_valueChanged( int value );
	void on_cbVideoDriver_currentIndexChanged( int id );
};

#endif // UIYABAUSE_H
