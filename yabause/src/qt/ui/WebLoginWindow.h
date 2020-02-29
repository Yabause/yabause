#ifndef WEBLOGINWINDOW_H
#define WEBLOGINWINDOW_H

#include <QDialog>
#include <thread>
#include "ui_WebLoginWindow.h"

using std::thread;

class YabaAuthStateListenerx;

class WebLoginWindow : public QDialog , public Ui::WebLoginWindow
{
    Q_OBJECT

public:
    explicit WebLoginWindow(QWidget *parent = nullptr);
    ~WebLoginWindow();
    virtual void done(int r);

signals: // [1]
    void setResult(QString id,QString error);
    void failToLogin();
    void reqFetchStatus();

public slots: // [1]

    void Lodaed(bool t);
    void getResult(QString id,QString error);
    void close_window(){ this->close(); }
    void onFailedToLogin();
    void fetchStatus();

private:
    std::thread * tx;
    bool login_cancel;
    QString id;
    QString key;
    QString error;

    YabaAuthStateListenerx * authlistner;

};

#endif // WEBLOGINWINDOW_H
