#ifndef UIYABAUSE_H
#define UIYABAUSE_H

#include "ui_UIYabause.h"

class YabauseGL;
class YabauseThread;

class UIYabause : public QMainWindow, public Ui::UIYabause
{
	Q_OBJECT
	
public:
	UIYabause( QWidget* parent = 0 );
	~UIYabause();

	void appendLog( const char* msg );
	void swapBuffers();

protected:
	YabauseGL* mYabauseGL;

protected:
	YabauseThread* mYabauseThread;
	QString mLog;

protected slots:
	void on_aYabauseSettings_triggered();
	void on_aYabauseRun_triggered();
	void on_aYabausePause_triggered();

};

#endif // UIYABAUSE_H
