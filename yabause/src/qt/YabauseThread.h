#ifndef YABAUSETHREAD_H
#define YABAUSETHREAD_H

#include <QThread>
#include <QMutexLocker>

#include "QtYabause.h"

class YabauseThread : public QThread
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
	
	void initEmulation();
	void deInitEmulation();
	void resetYabauseConf();
	void run();
};

#endif // YABAUSETHREAD_H
