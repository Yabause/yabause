#include "WebLoginWindow.h"
#include "ui_WebLoginWindow.h"
#include <qwebengineprofile.h>
#include <qwebenginecookiestore.h>
#include <QMessageBox>

#include <firebase/app.h>
#include <firebase/auth.h>
#include <firebase/database.h>
#include <firebase/variant.h>

#include "UIYabause.h"

#include <iostream>
#include <string>

using firebase::database::DataSnapshot;
using firebase::Variant;

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
      QMetaObject::invokeMethod(program_context,"close",Qt::QueuedConnection);
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
      QWebEngineProfile * p = webEngineView->page()->profile();
      p->cookieStore()->deleteAllCookies();
      p->clearHttpCache();
      webEngineView->setUrl(QUrl("https://uoyabause.firebaseapp.com/siginin.html"));
    }

    connect(webEngineView, SIGNAL(loadFinished(bool)), this, SLOT(Lodaed(bool)));
    connect(this, SIGNAL(setResult(QString,QString)), this, SLOT(getResult(QString,QString)));
    connect(this, SIGNAL(failToLogin()), this, SLOT(onFailedToLogin()));
    connect(this, SIGNAL(reqFetchStatus()), this, SLOT(fetchStatus()));

    tx = nullptr;
    login_cancel = false;

}

void WebLoginWindow::done(int r){
  if(tx != nullptr ){ 
    login_cancel = true;
    tx->join();
    delete tx;
    tx = nullptr;
  }
  QDialog::done(r);
}

WebLoginWindow::~WebLoginWindow()
{
  if(tx != nullptr ){ 
    login_cancel = true;
    tx->join();
    delete tx;
  }

  if( authlistner != nullptr ){
    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    auth->RemoveAuthStateListener(authlistner);
    delete authlistner;
  }
}

void WebLoginWindow::Lodaed(bool t){
    if(!t) return;
    if(tx != nullptr ){ 
      login_cancel = true;
      tx->join();
      delete tx;
      tx = nullptr;
    }
    login_cancel = false;
    key = "";
    error = "";
    tx = new std::thread([&]{
      while (!login_cancel) {
        reqFetchStatus();
        std::chrono::milliseconds dura(1000);
        std::this_thread::sleep_for(dura);
      }
#if 0
        QString id = "";
        QString error = "";
        while(id=="" && error==""){
          if( login_cancel == true ){
            return;
          }
          QWebEnginePage* page = webEngineView->page();
          if( page ){
            try {
              page->runJavaScript(
                "document.getElementById('id').innerHTML;",
                [&id](const QVariant &result) {
                //qDebug() << "Value is: " << result.toString() << endl;
                id = result.toString();
              }
              );

              page->runJavaScript(
                "document.getElementById('error').innerHTML;",
                [&error](const QVariant &result) {
                //qDebug() << "Error is: " << result.toString() << endl;
                error = result.toString();
              }
              );
            }
            catch (std::exception& e) {

            }

          }
          std::chrono::milliseconds dura( 1000 );
          std::this_thread::sleep_for( dura );
        }
        setResult(id,error);
#endif
    });

}

void WebLoginWindow::fetchStatus() {

  QWebEnginePage* page = webEngineView->page();
  if (page) {
      page->runJavaScript(
        "document.getElementById('id').innerHTML;",
        [&](const QVariant &result) {
        //qDebug() << "Value is: " << result.toString() << endl;
        if (result.isValid()) {
          key = result.toString();
          if (key != "") {
            if (tx != nullptr) {
              login_cancel = true;
              tx->join();
              delete tx;
              tx = nullptr;
            }
            setResult(key, error);
          }
        }
      }
      );

      page->runJavaScript(
        "document.getElementById('error').innerHTML;",
        [&](const QVariant &result) {
        //qDebug() << "Error is: " << result.toString() << endl;
        if (result.isValid()) {
          error = result.toString();
          if (error != "") {
            if (tx != nullptr) {
              login_cancel = true;
              tx->join();
              delete tx;
              tx = nullptr;
            }
            setResult(key, error);
          }
        }
      }
      );
  }
}

void WebLoginWindow::getResult(QString id,QString error){
    this->id = id;

    WebLoginWindow *self = this;
    
    firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
    firebase::auth::User* user = auth->current_user();
    if (user != nullptr) {
        //std::string name = user->display_name();
        //this->close();
        //auth->SignOut();
    }

    auth->AddAuthStateListener(authlistner);

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
                printf("Sigin in is OK! Name is %s", name.c_str() );
            }else{
                printf("Failed: %s\n",result.error_message());
                program_context->failToLogin();
            }
        }

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
