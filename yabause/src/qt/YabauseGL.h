#ifndef YABAUSEGL_H
#define YABAUSEGL_H

#include <QGLWidget>

class YabauseGL : public QGLWidget
{
	Q_OBJECT
	
public:
	YabauseGL( QWidget* parent = 0 );
	~YabauseGL();

protected:
	virtual void initializeGL();
	virtual void resizeGL( int w, int h );
	virtual void paintGL();

};

#endif // YABAUSEGL_H
