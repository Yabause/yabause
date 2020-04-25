#include "WebLoginWindow.h"
#include "ui_WebLoginWindow.h"
#include <qwebengineprofile.h>
#include <qwebenginecookiestore.h>
#include <QMessageBox>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <firebase/app.h>
#include <firebase/auth.h>
#include <firebase/database.h>
#include <firebase/variant.h>

#include "UIYabause.h"
 
#include <iostream>
#include <string>

using firebase::database::DataSnapshot;
using firebase::Variant;

#define API_KEY "pass"

class YabaAuthStateListenerx : public firebase::auth::AuthStateListener {
 public:
 WebLoginWindow* program_context;
  void OnAuthStateChanged(firebase::auth::Auth* auth) override {
    firebase::auth::User* user = auth->current_user();
    if (user != nullptr) {
      // User is signed in
      printf("OnAuthStateChanged: signed_in as %s\n", user->display_name().c_str());
      firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
      firebase::database::DatabaseReference dbref = database->GetReference();
      //QMetaObject::invokeMethod(program_context,"close",Qt::QueuedConnection);
    } else {
      // User is signed out
      printf("OnAuthStateChanged: signed_out\n");
    }
    // ...
  }
};


WebLoginWindow::WebLoginWindow(QWidget *parent) :
    QDialog(parent)
{
    authlistner = new YabaAuthStateListenerx();
    authlistner->program_context = this;
    setupUi(this);
    tx = nullptr;

    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    firebase::auth::User* user = auth->current_user();
    if (user != nullptr) {
        QMessageBox msgBox(parent);
        msgBox.setText(tr("Are you sure to sigin out?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int res = msgBox.exec();
        if (res == QMessageBox::Yes){
            auth->SignOut();
        }
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return;
    }else{
    }

//    connect(this, SIGNAL(setResult(QString,QString)), this, SLOT(getResult(QString,QString)));
    connect(this, SIGNAL(failToLogin()), this, SLOT(onFailedToLogin()));
//    connect(this, SIGNAL(reqFetchStatus()), this, SLOT(fetchStatus()));

    tx = nullptr;
    login_cancel = false;

}

class YMessageBox : public QMessageBox
{
public:
  explicit YMessageBox(QWidget *parent = Q_NULLPTR) {
  };

  int timeout = 0;
  bool autoClose = false;
  int currentTime = 0;
  
  void setAutoClose(bool b) {
    autoClose = true;
  }

  void setTimeout(int t) {
    timeout = t;
  }
  
  virtual void showEvent(QShowEvent * event) {
    currentTime = 0;
    if (autoClose) {
      this->startTimer(1000);
    }
  }
  virtual void timerEvent(QTimerEvent *event)
  {
    currentTime++;
    if (currentTime >= timeout) {
      this->done(0);
    }
  }
};


void WebLoginWindow::done(int r) {

  if (isSignInOK){
    
    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    firebase::auth::User* user = auth->current_user();
    if (user != nullptr) {
      YMessageBox msgBox(this);
      QString msg;
      msg = "Signin is successful. \n  Hello! " + QString(user->display_name().c_str());
      msgBox.setText(msg);
      msgBox.setStandardButtons(0); // QMessageBox::Ok);
      msgBox.setAutoClose(true);
      msgBox.setTimeout(3); //Closes after three seconds
      int res = msgBox.exec();
    }
    
  }
  QDialog::done(r);
}

WebLoginWindow::~WebLoginWindow()
{
  if( authlistner != nullptr ){
    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    auth->RemoveAuthStateListener(authlistner);
    delete authlistner;
  }
}


void WebLoginWindow::getResult(QString id,QString error){
    this->id = id;
    WebLoginWindow *self = this;
    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    auth->AddAuthStateListener(authlistner);

    //printf("id:%s\n",this->id.toStdString().c_str());

    firebase::auth::Credential credential =
    firebase::auth::GoogleAuthProvider::GetCredential(this->id.toStdString().c_str(),nullptr);
    firebase::Future<firebase::auth::User*> result = auth->SignInWithCredential(credential);

    result.OnCompletion(
      [](const firebase::Future<firebase::auth::User*>& result,
         void* user_data) {
        WebLoginWindow* program_context = (WebLoginWindow*)user_data;
        if (result.status() == firebase::kFutureStatusComplete) {
            if (result.error() == firebase::auth::kAuthErrorNone) {
                firebase::auth::User* user = *result.result();
                std::string name = user->display_name();
                std::string email = user->email();
                std::string photo_url = user->photo_url();
                std::string uid = user->uid();
                program_context->isSignInOK = true;
                QMetaObject::invokeMethod(program_context, "close", Qt::QueuedConnection);
            }else{
                printf("Failed: %s\n",result.error_message());
                program_context->failToLogin();
            }
        }
        program_context->ok->setDisabled(false);
        program_context->cancel->setDisabled(false);
      },
    (void*)self);
}

void WebLoginWindow::onFailedToLogin(){
  QMessageBox msgBox(this);
  msgBox.setText(tr("Failed to Sigin in. Check if you are sigin in on Android version and try again."));
  msgBox.setStandardButtons(QMessageBox::Ok);
  int res = msgBox.exec();
  QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}

void WebLoginWindow::on_cancel_clicked(){
  this->close();
}

void WebLoginWindow::on_ok_clicked()
{
  this->ok->setDisabled(true);
  this->cancel->setDisabled(true);

    QString pinnum = PinNumberEdit->text();
    QString pinnumjson = "{ \"key\":\"" + pinnum + "\"}";

    QNetworkAccessManager * mgr = new QNetworkAccessManager(this);
    connect(mgr,SIGNAL(finished(QNetworkReply*)),this,SLOT(onfinish(QNetworkReply*)));
    connect(mgr,SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

    QNetworkRequest request;
    request.setUrl(QUrl("https://5n71v2lg48.execute-api.us-west-2.amazonaws.com/default/getTokenAndDelete"));
    request.setRawHeader("x-api-key", API_KEY);
    request.setRawHeader("Content-Type", "application/json");

    QByteArray data;
    data = pinnumjson.toUtf8();

    mgr->post(request,data);
}

void WebLoginWindow::onfinish(QNetworkReply *rep)
{
    QVariant statusCode = rep->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    int status = statusCode.toInt();

    if ( status != 200 )
    {
        QString reason = rep->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
        QMessageBox msgBox(this);
        msgBox.setText(tr("Wrong PIN"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        int res = msgBox.exec();
        this->ok->setDisabled(false);
        this->cancel->setDisabled(false);
        return;
    }

    QByteArray bts = rep->readAll();
    QString id(bts);
    getResult(id,"");
}

