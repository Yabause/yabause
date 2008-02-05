#ifndef YABAUSEGL_H
#define YABAUSEGL_H

#include <QGLWidget>

class YabauseGL : public QGLWidget
{
	Q_OBJECT
	
public:
	YabauseGL( QWidget* parent = 0 );

protected:
	virtual void resizeGL( int w, int h );
};

#endif // YABAUSEGL_H
