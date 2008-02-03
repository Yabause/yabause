#ifndef YABAUSETHREAD_H
#define YABAUSETHREAD_H

#include <QThread>
#include <QMutexLocker>

#include "QtYabause.h"

class YabauseThread : public QObject
{
	Q_OBJECT
	
public:
	YabauseThread( QObject* owner = 0 );
	~YabauseThread();
	
	YabauseConf* yabauseConf();
	void startEmulation();
	void stopEmulation();
	
	void runEmulation();
	void pauseEmulation();
	void resetEmulation();
	void reloadSettings();

	bool emulationRunning();
	bool emulationPaused();

protected:
	YabauseConf* mYabauseConf;
	QMutex mMutex;
	bool mPause;
	bool mRunning;
	int mTimerId;
	
	void initEmulation();
	void deInitEmulation();
	void resetYabauseConf();
	void timerEvent( QTimerEvent* );

signals:
	void requestSize( const QSize& size );
	void requestFullscreen( bool fullscreen );
};

#endif // YABAUSETHREAD_H
