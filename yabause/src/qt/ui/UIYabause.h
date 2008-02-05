#ifndef UIYABAUSE_H
#define UIYABAUSE_H

#include "ui_UIYabause.h"

class YabauseGL;
class YabauseThread;
class QTextEdit;

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
	QTextEdit* teLog;

	void closeEvent( QCloseEvent* event );

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
	void on_aYabauseFrameSkipLimiter_triggered();
	//load
	//save
	void on_aYabauseQuit_triggered();
	// view menu
	void on_aViewFPS_triggered();
	void on_aViewLayerVdp1_triggered();
	void on_aViewLayerNBG0_triggered();
	void on_aViewLayerNBG1_triggered();
	void on_aViewLayerNBG2_triggered();
	void on_aViewLayerNBG3_triggered();
	void on_aViewLayerRBG1_triggered();
	void on_aViewFullscreen_triggered( bool b );
	void on_aViewLog_triggered();
	// help menu
	void on_aHelpAbout_triggered();
};

#endif // UIYABAUSE_H
