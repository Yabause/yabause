/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "MenuScreen.h"
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/button.h>
#include <nanogui/popupbutton.h>
#include <nanogui/messagedialog.h>
#include "GameListPanel.h"
#include <nanogui/combobox.h>
#include <nanogui/vscrollpanel.h>
#include "../config.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include <experimental/filesystem>
#include <ctime>
#include <iomanip>

#include <nanogui/imageview.h>

namespace fs = std::experimental::filesystem ;

using namespace std;

#include "InputManager.h"

#include "about.h"

#include "Preference.h"

#include "GameInfo.h"

#include "yabause.h"

//#define MENU_LOG
#define MENU_LOG printf


int MenuScreen::onShow(){
  //setupPlayerPsuhButton( 0, player1, "Player1 Input Settings", &p1cb );
  //setupPlayerPsuhButton( 1, player2, "Player2 Input Settings", &p2cb );
}

#include <thread>

void MenuScreen::refreshGameListAsync(const vector<string> & base_path_array) {
  bFileSearchCancled = false;

  MessageDialog * dlg = new MessageDialog(this, MessageDialog::Type::Warning, "Searching games", "", "Cancel");
  evm->setEvent("updateFile", [this, dlg](int code, void * data1, void * data2) {
    if (!bFileSearchCancled) {
      string msg((char*)data1);
      Label * pl = dlg->messageLabel();
      pl->setCaption(msg);
      delete data1;
    }
  });


  std::thread *t = new std::thread([this, base_path_array]() {
    int indent = 0;
    for (int i = 0; i < base_path_array.size(); i++) {
      listdir(base_path_array[i], indent, games);
    }
    if (!bFileSearchCancled) {
      evm->postEvent("showFileSelectDialog");
    }
  });

  dlg->setCallback([this, t](int result) {
    bFileSearchCancled = true;
    t->join();
    delete t;
    games.clear();
    MENU_LOG("Cancel\n");
    //void * datax = malloc(256 * sizeof(char));
    //strcpy((char*)datax, "");
    //evm->postEvent("close tray", datax);
  });

  evm->setEvent("showFileSelectDialog", [this, dlg, t, base_path_array](int code, void * data1, void * data2) {
    t->join();
    delete t;
    dlg->dispose();
    showFileSelectDialog(tools, bCdTray, base_path_array);
  });


}

MenuScreen::MenuScreen( SDL_Window* pwindow, int rwidth, int rheight, const std::string & fname, const std::string & game )
: nanogui::Screen( pwindow, Eigen::Vector2i(rwidth, rheight), "Menu Screen"){
  

  gameInfoManager = new GameInfoManager();
  gameInfoManager->getAll(games);

  mFocus = nullptr;
  config_file_ = fname;
  current_game_path_ = game;
  swindow = nullptr;
  imageWindow = nullptr;

  int image_pix_size_w = this->width() / 2;
  int image_pix_size_h = this->height() / 2;
  imageWindow = new Window(this, "About");                                                                                                         
  imageWindow->setPosition(Vector2i(0, 0));                                                                                                                   
  imageWindow->setLayout(new GroupLayout(0,0,0));                                                                                                                     
  GLTexture t;    
  t.load(about_png,about_png_size);  
  float scale = (float)image_pix_size_w / t.width();
  auto imageView = new ImageView(imageWindow,t);  
  imageView->setScale(scale);
  imageView->setFixedScale(true);
  imageView->setFixedOffset(true);
  imageView->setFixedWidth(image_pix_size_w);
  imageView->setFixedHeight(image_pix_size_h);
  imageWindow->center();
  //imageWindow->setModal(true);

  std::string title = "Yaba Sanshiro "+ std::string(YAB_VERSION) +" Menu";
        window = new Window(this, title);
        window->setPosition(Vector2i(0, 0));
        window->setLayout(new GroupLayout());

        tools = new Widget(window);
        pushActiveMenu(tools, nullptr );
        tools->setLayout(new BoxLayout(Orientation::Vertical,Alignment::Middle, 0, 5));
        tools->setFixedWidth(256);

        PlayerConfig tmp;
        tmp.player = new PopupButton(tools, "Player1", ENTYPO_ICON_EXPORT);      
        setupPlayerPsuhButton( 0, tmp.player, "Player1 Input Settings", &tmp.cb );
        player_configs_.push_back(tmp);
        tmp.player = new PopupButton(tools, "Player2", ENTYPO_ICON_EXPORT);      
        setupPlayerPsuhButton( 1, tmp.player, "Player2 Input Settings", &tmp.cb );
        player_configs_.push_back(tmp);

       
        PopupButton * ps_config = new PopupButton(tools, "Config");
        ps_config->setFixedWidth(248);
        showConfigDialog(ps_config);
        ps_config->setCallback([this,ps_config]() {      
          pushActiveMenu(ps_config->popup(),ps_config); 
        });

        Button *b0 = new Button(tools, "Exit");
        b0->setFixedWidth(248);
        b0->setCallback([this]() { 
          MENU_LOG("Exit\n"); 
      		SDL_Event* quit = new SDL_Event();
			    quit->type = SDL_QUIT;
			    SDL_PushEvent(quit);  
        });

        Button *b1 = new Button(tools, "Reset");
        b1->setFixedWidth(248);
        b1->setCallback([this]() { 
          MENU_LOG("Reset\n");  
          SDL_Event event = {};
          event.type = reset_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });        

        PopupButton * ps = new PopupButton(tools, "Save State");
        ps->setFixedWidth(248);
        ps->setCallback([this,ps]() {      
          showSaveStateDialog( ps->popup());
          pushActiveMenu(ps->popup(),ps); 
        });

        ps = new PopupButton(tools, "Load State");
        ps->setFixedWidth(248);
        ps->setCallback([this,ps]() {      
          showLoadStateDialog( ps->popup());
          pushActiveMenu(ps->popup(),ps); 
        });



        bCdTray = new Button(tools, "Open CD Tray");
        bCdTray->setFixedWidth(248);
        bCdTray->setCallback([this]() { 
          if( this->is_cdtray_open_ ){
            MENU_LOG("Close CD Tray\n"); 
            bCdTray->setCaption("Open CD Tray");
            this->is_cdtray_open_ = false;

            size_t pos = current_game_path_.find_last_of("/");
            string base_path = current_game_path_.substr(0,pos);
            showFileSelectDialog( tools, bCdTray, base_path);

            /*
            SDL_Event event = {};
            event.type = close_tray_;
            event.user.code = 0;
            //event.user.data1 = malloc( 256* sizeof(char) );
            //strcpy( (char*)event.user.data1, "filename" );
            event.user.data2 = 0;
            SDL_PushEvent(&event);               
            */

            Preference pref("default");
            vector<string> base_path_array = pref.getStringArray("game directories");
            if (base_path_array.size() >= 0) {
              base_path = base_path_array[0];
            }

            if (games.size() == 0) {
              refreshGameListAsync(base_path_array);
            }
            else {
              showFileSelectDialog(tools, bCdTray, base_path_array);
            }
           
          }else{
            MENU_LOG("Open CD Tray\n"); 
            bCdTray->setCaption("Close CD Tray");
            this->is_cdtray_open_ = true;
            SDL_Event event = {};
            event.type = open_tray_;
            event.user.code = 0;
            event.user.data1 = 0;
            event.user.data2 = 0;
            SDL_PushEvent(&event);
          }
        });


        Button *b2 = new Button(tools, "Show/Hide FPS");
        b2->setFixedWidth(248);
        b2->setCallback([this]() { 
          MENU_LOG("Show/Hide FPS\n");  
          SDL_Event event = {};
          event.type = toggile_fps_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });

        Button *b3 = new Button(tools, "Enable/Disable Frame Skip");
        b3->setFixedWidth(248);
        b3->setCallback([this]() { 
          MENU_LOG("Reset\n");  
          SDL_Event event = {};
          event.type = toggile_frame_skip_;
          event.user.code = 0;
          event.user.data1 = 0;
          event.user.data2 = 0;
          SDL_PushEvent(&event);          
        });        
#if 0
        Button *b4 = new Button(tools, "About");
        b4->setFixedWidth(248);
        b4->setCallback([this,b4]() { 
          int image_pix_size_w = this->width() / 2;
          int image_pix_size_h = this->height() / 2;
          imageWindow = new Window(this, "About");                                                                                                         
          imageWindow->setPosition(Vector2i(0, 0));                                                                                                                   
          imageWindow->setLayout(new GroupLayout(0,0,0));                                                                                                                     
          GLTexture t;    
          t.load(about_png,about_png_size);  
          float scale = (float)image_pix_size_w / t.width();
          //float offset = (t.width() - image_pix_size) / 2;
          auto imageView = new ImageView(imageWindow,t);  
          imageView->setScale(scale);
          imageView->setFixedScale(true);
          imageView->setFixedOffset(true);
          imageView->setFixedWidth(image_pix_size_w);
          imageView->setFixedHeight(image_pix_size_h);

          Button *btn = new Button(imageWindow, "Close");
          btn->setCallback([this]() { 
            this->popActiveMenu();
            if( this->imageWindow != nullptr ){
              this->imageWindow->dispose();
              this->imageWindow = nullptr;
            }
          });

          imageWindow->center();
          imageWindow->setModal(true);
          imageWindow->requestFocus();

          pushActiveMenu(imageWindow,b4);

        });
#endif
        player_configs_[0].player->focusEvent(true);
        player_configs_[0].player->mouseEnterEvent(player_configs_[0].player->absolutePosition(),true);
        mFocus = player_configs_[0].player;

        performLayout();
        
}


void MenuScreen::showInputCheckDialog( const std::string & key ){
    swindow = new Window(this, key);
    swindow->setPosition(Vector2i(0, 0));
    swindow->setLayout(new GroupLayout(32));
    new Label(swindow,"Push key for " + key, "sans", 64);
    swindow->center();
    swindow->setModal(true);
    swindow->requestFocus();
    current_key_ = key;
}

#include <dirent.h>

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


void MenuScreen::showConfigDialog( PopupButton *parent ){

  // Todo setCurrentGamePath
  std::shared_ptr<Preference> preference(new Preference( current_game_path_ ));

  Popup *popup = parent->popup();    
  popup->setLayout(new GroupLayout(4,2,2,2)); 
  new Label(popup, "Resolution");
  ComboBox * cb = new ComboBox(popup);  
  vector<string> items;
  items.push_back("Native");
  items.push_back("4x");
  items.push_back("2x");
  items.push_back("Original");
  items.push_back("720p");
  items.push_back("1080p");

  cb->setItems(items);
  Popup *cbpopup = cb->popup(); 
  cb->setCallback([this,cbpopup,cb]() {       
    pushActiveMenu(cbpopup, cb );
  });

  cb->setSelectedIndex( preference->getInt("Resolution",0) );
  cb->setCallbackSelect([this,preference]( int idx ) {
    popActiveMenu();
    preference->setInt("Resolution",idx);
    VideoSetSetting(VDP_SETTING_RESOLUTION_MODE, idx);
  });

  new Label(popup, "Aspect rate");
  cb = new ComboBox(popup);  
  items.clear();
  items.push_back("Original");
  items.push_back("4:3");
  items.push_back("16:9");
  items.push_back("Full Screen");
  cb->setItems(items);
  cbpopup = cb->popup(); 
  cb->setCallback([this,cbpopup,cb]() {       
    pushActiveMenu(cbpopup, cb );
  });

  cb->setSelectedIndex( preference->getInt("Aspect rate",0) );
  cb->setCallbackSelect([this,preference]( int idx ) {
    popActiveMenu();
    preference->setInt("Aspect rate",idx);
  });

  new Label(popup, "Rotate screen resolution");
  cb = new ComboBox(popup);  
  items.clear();
  items.push_back("Original");
  items.push_back("2x");
  items.push_back("720p");
  items.push_back("1080p");
  items.push_back("Native");
  cb->setItems(items);
  cbpopup = cb->popup(); 
  cb->setCallback([this,cbpopup,cb]() {       
    pushActiveMenu(cbpopup, cb );
  });

  cb->setSelectedIndex( preference->getInt("Rotate screen resolution",0) );
  cb->setCallbackSelect([this,preference]( int idx ) {
    popActiveMenu();
    preference->setInt("Rotate screen resolution",idx);
    VideoSetSetting(VDP_SETTING_RBG_RESOLUTION_MODE, idx);
  });


  Button * ba = new Button(popup,"Use compute shader");  
  ba->setFlags(Button::ToggleButton); 
  ba->setPushed( preference->getBool("Use compute shader",false) );
  ba->setChangeCallback([this,preference](bool state) { 
    preference->setBool("Use compute shader",state);
    VideoSetSetting(VDP_SETTING_RBG_USE_COMPUTESHADER, state);
  });


  ba = new Button(popup,"Rotate screen");  
  ba->setFlags(Button::ToggleButton); 
  ba->setPushed( preference->getBool("Rotate screen",false) );
  ba->setChangeCallback([this,preference](bool state) { 
    preference->setBool("Rotate screen",state);
    VideoSetSetting(VDP_SETTING_ROTATE_SCREEN, state);
  });

}

void MenuScreen::showSaveStateDialog( Popup *popup ){

  while (popup->childCount() != 0)
    popup->removeChild(popup->childCount()-1);

  popup->setLayout(new GroupLayout(4,2,2,2)); 

  for( int i = 0; i< 5; i++ ){

    string title;

    std::stringstream stream;

    stream << "Save Slot ";
    stream << i;

    std::stringstream img_stream;
    img_stream << this->cuurent_game_id_;
    img_stream << i;
    img_stream << ".yss";

    printf("Checking %s \n", img_stream.str().c_str() );
    if( fs::exists( img_stream.str() ) ){
      auto ftime = fs::last_write_time(img_stream.str());
      namespace chrono = std::chrono;
      auto sec = chrono::duration_cast<chrono::seconds>(ftime.time_since_epoch());
      std::time_t t = sec.count();
      const tm* lt = std::localtime(&t);
      std::ostringstream ss;
      stream << "  " << std::put_time(lt, "%c");
      printf("%s",stream.str().c_str());
      printf("\n");
    }

    Button *tmp = new Button(popup, stream.str() );
    tmp->setCallback([this,i,popup]() { 
      SDL_Event event = {};
      event.type = save_state_;
      event.user.code = i;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);
      popActiveMenu();
    });  


    tmp->setOnSetFocusCallback([this,i,popup]() {
      std::stringstream img_stream;
      img_stream << this->cuurent_game_id_;
      img_stream << i;
      img_stream << ".png";
      cout << img_stream.str();
      this->setTmpBackGroundImage(img_stream.str());
    });

    tmp->setOnLeaveCallback([this,i,popup]() {
      this->setDefalutBackGroundImage();
    });

  }
  this->performLayout();
}

void MenuScreen::showLoadStateDialog( Popup *popup ){
  while (popup->childCount() != 0)
    popup->removeChild(popup->childCount()-1);

  popup->setLayout(new GroupLayout(4,2,2,2)); 

  for( int i = 0; i< 5; i++ ){

    string title;

    std::stringstream stream;

    stream << "Load Slot ";
    stream << i;

    std::stringstream img_stream;
    img_stream << this->cuurent_game_id_;
    img_stream << i;
    img_stream << ".yss";

    printf("Checking %s \n", img_stream.str().c_str() );
    if( fs::exists( img_stream.str() ) ){
      auto ftime = fs::last_write_time(img_stream.str());
      namespace chrono = std::chrono;
      auto sec = chrono::duration_cast<chrono::seconds>(ftime.time_since_epoch());
      std::time_t t = sec.count();
      const tm* lt = std::localtime(&t);
      std::ostringstream ss;
      stream << "  " << std::put_time(lt, "%c");
      printf("%s",stream.str().c_str());
      printf("\n");

      Button *tmp = new Button(popup, stream.str() );
      tmp->setCallback([this,i,popup]() { 
        SDL_Event event = {};
        event.type = load_state_;
        event.user.code = i;
        event.user.data1 = 0;
        event.user.data2 = 0;
        SDL_PushEvent(&event);
        popActiveMenu();
      });  

      tmp->setOnSetFocusCallback([this,i,popup]() {
        std::stringstream img_stream;
        img_stream << this->cuurent_game_id_;
        img_stream << i;
        img_stream << ".png";
        cout << img_stream.str();
        this->setTmpBackGroundImage(img_stream.str());
      });
    
      tmp->setOnLeaveCallback([this,i,popup]() {
        this->setDefalutBackGroundImage();
      });
    }
  }
  this->performLayout();

}

void MenuScreen::listdir(const string & dirname, int indent, vector<shared_ptr<GameInfo>> & files )
{
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(dirname.c_str())))
    return;

  while ((entry = readdir(dir)) != NULL) {

    if ( bFileSearchCancled ) {
      return;
    }

    if (entry->d_type == DT_DIR) {
      char path[1024];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", dirname.c_str(), entry->d_name);
      printf("%*s[%s]\n", indent, "", entry->d_name);
      listdir(string(path), indent + 2, files);
    }
    else {
      printf("%*s- %s\n", indent, "", entry->d_name);
      string dname = dirname + "/" + entry->d_name;
      std::transform(dname.begin(), dname.end(), dname.begin(), ::tolower);
      //if (ends_with(dname, ".cue") || ends_with(dname, ".mdf") || ends_with(dname, ".ccd") || ends_with(dname, ".chd")) {
      //  files.push_back(dname);
      //}

      if (ends_with(dname, ".cue") ) {
        shared_ptr<GameInfo> p = GameInfo::genGameInfoFromCUE(dname);
        if (p != nullptr) {
          files.push_back(p);
          gameInfoManager->insert(p);
        }
        else {
          cout << "Fail to generate " << dname << endl;
        }
      }

      if (ends_with(dname, ".chd")) {
        shared_ptr<GameInfo> p = GameInfo::genGameInfoFromCHD(dname);
        if (p != nullptr) {
          files.push_back(p);
          gameInfoManager->insert(p);
        }
        else {
          cout << "Fail to generate " << dname << endl;
        }
      }

      void * data = new char[dname.length()+1];
      strcpy((char*)data, dname.c_str());
      evm->postEvent("updateFile", data);

    }
  }
  closedir(dir);
}


void MenuScreen::checkdir(const string & dirname, int indent, vector<string> & files)
{
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(dirname.c_str())))
    return;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char path[1024];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", dirname.c_str(), entry->d_name);
      printf("%*s[%s]\n", indent, "", entry->d_name);
      checkdir(string(path), indent + 2, files);
      if (files.size() != 0) {
        closedir(dir);
        return;
      }
    }
    else {
      printf("%*s- %s\n", indent, "", entry->d_name);
      string dname = dirname + "/" + entry->d_name;
      std::transform(dname.begin(), dname.end(), dname.begin(), ::tolower);
      if (ends_with(dname, ".cue") || ends_with(dname, ".mdf") || ends_with(dname, ".ccd") || ends_with(dname, ".chd")) {
        files.push_back(dname);
        closedir(dir);
        return;
      }
    }
  }
  closedir(dir);
}

void MenuScreen::checkGameFiles(Widget * parent, const vector<std::string> & base_paths) {
  DIR *dir;
  struct dirent *ent;
  int filecount = 0;

  vector<string> files;
  int indent = 0;
  for (int i = 0; i < base_paths.size(); i++) {
    checkdir(base_paths[i], indent, files);
    filecount = files.size();
    if (filecount != 0) {
      break;
    }
  }

#if 0
  if ((dir = opendir(base_path.c_str())) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      string dname = ent->d_name;
      std::transform(dname.begin(), dname.end(), dname.begin(), ::tolower);
      if (ends_with(dname, ".cue") || ends_with(dname, ".mdf") || ends_with(dname, ".ccd") || ends_with(dname, ".chd")) {
        filecount++;
        break;
      }
    }
  }
#endif

  if (filecount == 0) {
    string message;
    message = "No games are found in \"";
    message += base_paths[0] + "\" folder. ";
    message += "Place cue or ccd or chd files there.";
    auto dlg = new MessageDialog(this, MessageDialog::Type::Warning, "Game not found", message.c_str() );
    nanogui::Button* btn = (nanogui::Button*)mFocus;
    mFocus->mouseEnterEvent(mFocus->position(), false);

    auto preFocus = mFocus;
    dlg->setCallback([this, preFocus](int result) { 
      mFocus = preFocus;
      preFocus->mouseEnterEvent(preFocus->position(), true); 
    });
    dlg->requestFocus();

    mFocus = dlg->getOkBottn();
    MENU_LOG("%s is selected\n", ((nanogui::Button*)mFocus)->caption().c_str());
    mFocus->mouseEnterEvent(mFocus->position(), true);
  }
}

void MenuScreen::showFileSelectDialog( Widget * parent, Widget * toback, const std::string & base_path ){
  const int dialog_width = 512;
  const int dialog_height = this->size()[1] - 20 ;
    swindow = new Window(this, "Select Game");
    swindow->setPosition(Vector2i(  this->size()[0]/2 - (dialog_width/2) ,   this->size()[1]/2 - (dialog_height/2) ));
    swindow->setFixedSize({dialog_width, dialog_height});
    //swindow->setLayout(new GroupLayout());

    auto vscroll = new VScrollPanel(swindow);
    vscroll->setPosition(Vector2i(0, 20));
    vscroll->setFixedSize({dialog_width, dialog_height - 28});
    
    auto wrapper = new Widget(vscroll);
    wrapper->setFixedSize({dialog_width, dialog_height });
    wrapper->setLayout(new GroupLayout());
    

    bool first_object = true;

    Button *b1 = new Button(wrapper, "refresh");
    if (first_object) {
      first_object = false;
      pushActiveMenu(wrapper, toback);
    }

    b1->setCallback([this]() {
      MENU_LOG("refresh\n");

      this->popActiveMenu();
      swindow->dispose();
      swindow = nullptr;

      gameInfoManager->clearAll();
      games.clear();

      Preference pref("default");
      vector<string> base_path_array = pref.getStringArray("game directories");
      refreshGameListAsync(base_path_array);

    });



    Button *b0 = new Button(wrapper, "Cancel");
    if (first_object) {
      first_object = false;
      pushActiveMenu(wrapper, toback);
    }

    b0->setCallback([this]() {
      MENU_LOG("Cancel\n");
      void * data1 = malloc(256 * sizeof(char));
      strcpy((char*)data1, "");
      evm->postEvent("close tray", data1);
      this->popActiveMenu();
      swindow->dispose();
      swindow = nullptr;
    });

    int file_count = 0;
    int indent = 0;
    if (games.size() == 0) {
      gameInfoManager->clearAll();
      for (int i = 0; i < base_paths.size(); i++) {
        listdir(base_paths[i], indent, games);
      }
    }

    file_count = games.size();

    
    vector<pair<int, string>> icons;
    for (int i = 0; i < games.size(); i++) {

      printf("%d, %s\n",i, games[i]->imageUrl.c_str());

      ImageButton *tmp = new ImageButton(wrapper, games[i]->gameTitle, i);
      string path = games[i]->filePath;

      tmp->setOnImageRequested([this,vscroll](int id, int x, int y, int w, int h) {

        float pos = vscroll->getScrollPos();

        const int bx = vscroll->position().x();
        const int by = vscroll->position().y() + int(pos);
        const int bw = vscroll->size().x();
        const int bh = vscroll->size().y();

        // 交差してる?
        if ( x < bx + bw && bx < x + w &&
             y < by + bh && by < y + h) {

          int imageid = imageCache->get(id);

          if (imageid == -1) {
            int img = nvgCreateImage(mNVGContext, games[id]->imageUrl.c_str(), 0);
            int removed = imageCache->set(id,img);
            if (removed != -1) {
              nvgDeleteImage(mNVGContext, removed);
            }
            return img;
          }
          return imageid;
        }
        return -1;

      });

      tmp->setCallback([this, path]() {
        MENU_LOG("CD Close: %s\n", path.c_str());
        void * data1 = malloc((path.size() + 1) * sizeof(char));
        strcpy((char*)data1, path.c_str());
        evm->postEvent("close tray", data1);

        this->popActiveMenu();
        this->swindow->dispose();
        this->swindow = nullptr;
      });

      if (first_object) {
        first_object = false;
        pushActiveMenu(wrapper, toback);
      }

    }



    if (file_count == 0) {
      swindow->setModal(true);
      checkGameFiles(this, base_paths);
    }
    else {
      swindow->setModal(true);
      swindow->requestFocus();
    }

    this->performLayout();

}

void MenuScreen::getSelectedGUID( int user_index, std::string & selguid ){

  selguid = "-1";
  json j;
  std::stringstream ss;
  std::string userid;

  try{
    std::ifstream fin( config_file_ );
    fin >> j;
    fin.close();
    ss << "player" << (user_index+1);
    userid = ss.str();
    if( j.find(userid) != j.end() ) {
      InputManager::genJoyString( selguid, j[userid]["DeviceID"], j[userid]["deviceName"], j[userid]["deviceGUID"] );
    }
  }catch ( json::exception& e ){

  }

}


void MenuScreen::setupPlayerPsuhButton( int user_index, PopupButton *player, const std::string & label, ComboBox **cbo ){
  player->setFixedWidth(248);
  Popup *popup = player->popup();     
  popup->setLayout(new GroupLayout(4,2,2,2)); 
  new Label(popup, label);

  std::string username;


  json j;
  string selguid="BADGUID";
  string selname="BADNAME";
  int padmode=0;
  std::stringstream ss;
  std::string userid;

  try{
    std::ifstream fin( config_file_ );
    fin >> j;
    fin.close();
    ss << "player" << (user_index+1);
    userid = ss.str();
    if( j.find(userid) != j.end() ) {
      selguid = j[userid]["deviceGUID"];
      InputManager::genJoyString( selguid, j[userid]["DeviceID"], j[userid]["deviceName"], j[userid]["deviceGUID"] );
      padmode = j[userid]["padmode"];
    }
  }catch ( json::exception& e ){

  }

  ComboBox * cb = new ComboBox(popup);  
  vector<string> items;
  vector<string> itemsShort;
  int index = 0;
  int selindex = -1;
  for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
    SDL_Joystick* joy = it->second;
    SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
    char guid[65];
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
    string key_string;
    InputManager::genJoyString( key_string, joyId, SDL_JoystickName(joy), guid );
    if( selguid  == key_string ){
      selindex = index;
    }
    printf("listguid = %d:%s\n", index, key_string.c_str() );
    index++;
    items.push_back( key_string );
    itemsShort.push_back(SDL_JoystickName(joy));
  }  
  cb->setItems(itemsShort);

  if( selindex != -1 ){
    cb->setSelectedIndex(selindex);
  }
  printf("selguid = %d:%s\n", selindex, selguid.c_str() );

  Popup *cbpopup = cb->popup(); 
  cb->setCallback([this,cbpopup,cb]() {       
    pushActiveMenu(cbpopup, cb );
  });
  
  cb->setCallbackSelect([this, userid]( int idx ) {
      popActiveMenu();

      SDL_JoystickID joyId = -1;
      int itenindex = 0;
      std::string device_name = "Keyboard";
      string guid_only = "-1";
      cuurent_deviceguid_ = "Keyboard_-1";
      if( idx >= joysticks_.size() ){
        cuurent_deviceguid_ = "Keyboard_-1"; // keyboard may be
      }else{
        for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
          if( itenindex == idx ){
            SDL_Joystick* joy = it->second;
            joyId = SDL_JoystickInstanceID(joy);
            char guid[65];
            string joy_name_and_guid;
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
            InputManager::genJoyString( joy_name_and_guid, joyId, SDL_JoystickName(joy),  guid );
            cuurent_deviceguid_ = joy_name_and_guid;
            MENU_LOG("cuurent_deviceguid_ = %s\n", cuurent_deviceguid_.c_str() );
            device_name = SDL_JoystickName(joy);
            guid_only = guid;
            break;
          }
          itenindex++;
        }
      }

      json j;
      std::ifstream fin( this->config_file_ );
      fin >> j;
      fin.close();
      //if( j.find(userid) == j.end() ) {
      //  return;
      //}            
      j[userid]["deviceGUID"] = guid_only;
      j[userid]["DeviceID"]   = joyId;
      j[userid]["deviceName"] = device_name;

      if( cuurent_deviceguid_ != "Disable_-2"){
        for( int i=0; i<player_configs_.size(); i++ ){
          std::stringstream ss;
          ss << "player" << (i+1);
          std::string taeget_userid = ss.str();
          if( userid != taeget_userid){
            if( j.find(taeget_userid) != j.end()){
              std::string other_guid;
              InputManager::genJoyString( other_guid, j[taeget_userid]["DeviceID"],  j[taeget_userid]["deviceName"], j[taeget_userid]["deviceGUID"] );
              cout << "O = " << other_guid << endl << "C = " << cuurent_deviceguid_ << endl;
              if( other_guid == cuurent_deviceguid_ ){
                cout << "Disable:" << other_guid << endl;
                // Select Disable
                j[taeget_userid]["deviceGUID"] = "-2";
                j[taeget_userid]["DeviceID"] = -2;
                j[taeget_userid]["deviceName"] = "Disable";    
                const std::vector<std::string> & items = player_configs_[i].cb->items(); 
                for( int ii = 0; ii < items.size(); ii++ ) {
                  if( items[ii] == j[taeget_userid]["deviceName"] ){
                    player_configs_[i].cb->setSelectedIndex(ii);
                  }
                }
              }
            }
          }
        }      
      }
      this->performLayout();
      std::ofstream out(this->config_file_);
      out << j.dump(2);
      out.close();        

      SDL_Event event = {};
      event.type = this->update_config_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);         

  });


  *cbo = cb;

  Button * ba = new Button(popup,"Analog mode");  
  ba->setFlags(Button::ToggleButton); 
  if( padmode != 0 ){
    ba->setPushed(true);
  }else{
    ba->setPushed(false);
  }
  ba->setChangeCallback([this, userid](bool state) { 
    cout << "Toggle button state: " << state << endl;

    json j;
    std::ifstream fin( this->config_file_ );
    fin >> j;
    fin.close();
    //if( j.find(userid) == j.end() ) {
    //  return;
    //}

    try{
      if( state )
        j[userid]["padmode"] = 1;
      else
        j[userid]["padmode"] = 0;

      std::ofstream out(this->config_file_);
      out << j.dump(2);
      out.close();

      SDL_Event event = {};
      event.type = this->update_config_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);   
    }catch ( json::exception& e ){

    }

  }); 

  
  player->setCallback([this,popup,player,user_index]() {      
    pushActiveMenu(popup,player); 
    std::stringstream s ;
    s << "player" << user_index;
    current_user_ = s.str();

  });


  Button * b;
  b = new Button(popup, "UP");
  b->setCallback([this, user_index ]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("up");
  });
  b = new Button(popup, "DOWN");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("down");
  });
  b = new Button(popup, "LEFT");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("left");
  });
  b = new Button(popup, "RIGHT");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("right");
  });

  b = new Button(popup, "START");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("start");
  });

  b = new Button(popup, "A");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("a");
  }); 

  b = new Button(popup, "B");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("b");
  }); 

  b = new Button(popup, "C");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("c");
  });   
  b = new Button(popup, "X");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("x");
  });   
  b = new Button(popup, "Y");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("y");
  });     
  b = new Button(popup, "Z");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("z");
  });     
  b = new Button(popup, "L");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("l");
  });   
  b = new Button(popup, "R");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    showInputCheckDialog("r");
  });   
  b = new Button(popup, "Analog X");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    if( this->cuurent_deviceguid_ != "Keyboard_-1" ){
      showInputCheckDialog("analogx");
    }
  });   
  b = new Button(popup, "Analog Y");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    if( this->cuurent_deviceguid_ != "Keyboard_-1" ){
      showInputCheckDialog("analogy");
    }
  });   
  b = new Button(popup, "Analog L");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    if( this->cuurent_deviceguid_ != "Keyboard_-1"){
      showInputCheckDialog("analogl");
    }
  });   
  b = new Button(popup, "Analog R");
  b->setCallback([this, user_index]{
    getSelectedGUID( user_index, this->cuurent_deviceguid_ );
    if( this->cuurent_deviceguid_ != "Keyboard_-1"){
      showInputCheckDialog("analogr");
    }
  });   
}

/*
int MenuScreen::OnInputSelected( string & type, int & id, int & value ){
  current_player[ buttons[ current_setting_index ] ] = { {"id",id},{"value",value},{"type",type}};
  current_setting_index++;

  b
}
*/

int MenuScreen::onRawInputEvent( InputManager & imp, const std::string & deviceguid, const std::string & type, int id, int value ){
  if( swindow == nullptr ){ return -1; }
  if( swindow->title() == "Select File"){ return -1; }

  cout << "onRawInputEvent deviceguid:" << deviceguid << " type:" << type << " id:" << id << " val:" << value << endl;

  if( deviceguid != cuurent_deviceguid_ ){ 
    cout << "deviceguid = " << deviceguid << " vs cuurent_deviceguid = " << cuurent_deviceguid_ << endl;
    return -1; 
  }

  // wait for key input?
  if( current_key_ != "l" && current_key_ != "r" ) {
    if( current_key_.find("analog") == std::string::npos  ){
      //if( type == "axis" ){
      //  return -1;
      //}
    }else{
      if( type != "axis" ){
        return -1;
      }
    }
  }

  imp.saveInputConfig( deviceguid , current_key_, type, id, value);
  swindow->dispose();
  swindow = nullptr;

  SDL_Event event = {};
  event.type = this->update_config_;
  event.user.code = 0;
  event.user.data1 = 0;
  event.user.data2 = 0;
  SDL_PushEvent(&event);   
  return 0;
}

bool MenuScreen::keyboardEvent( std::string & keycode , int scancode, int action, int modifiers){

  if( swindow != nullptr && swindow->title() != "Select Game"){ return false; }

  MENU_LOG("%s %d %d\n",keycode.c_str(),scancode,action);
  if (action != 0) {
          
    if ( keycode.find("down") != string::npos ) {
      nanogui::Button* btn = (nanogui::Button*)mFocus;
      //btn->setPushed(false);
      mFocus->mouseEnterEvent(mFocus->position(), false);
      mFocus = getActiveMenu()->getNearestWidget(mFocus, 1);
      MENU_LOG("%s is selected\n",((nanogui::Button*)mFocus)->caption().c_str());
      mFocus->mouseEnterEvent(mFocus->position(), true);
     
      auto wrapper = mFocus->parent();
      if( wrapper != nullptr ){
          auto vscroll = wrapper->parent();
          if( vscroll != nullptr && dynamic_cast<VScrollPanel*>(vscroll) != nullptr  ){
              MENU_LOG("pos=%d vpod = %f\n",mFocus->position().y(),((VScrollPanel*)vscroll)->getScrollPos()  );
              if( (mFocus->position().y() + mFocus->size().y())  - ((VScrollPanel*)vscroll)->getScrollPos() > vscroll->height() ){
                Vector2i p(0,0);
                Vector2f rel(0.0, -(1.0 / (games.size()) * 20) );
                (VScrollPanel*)vscroll->scrollEvent(p,rel);
            }
        }
      }

    }else if ( keycode.find("up") != string::npos ) {
      nanogui::Button* btn = (nanogui::Button*)mFocus;
      //btn->setPushed(false);
      mFocus->mouseEnterEvent(mFocus->position(), false);
      mFocus = getActiveMenu()->getNearestWidget(mFocus, 0);
      mFocus->mouseEnterEvent(mFocus->position(), true);

      auto wrapper = mFocus->parent();
      if( wrapper != nullptr ){
          auto vscroll = wrapper->parent();
          if( vscroll != nullptr ){
              MENU_LOG("pos=%d vpod = %f\n",mFocus->position().y(),((VScrollPanel*)vscroll)->getScrollPos()  );
              if((mFocus->position().y()) - ((VScrollPanel*)vscroll)->getScrollPos() < 0 ){
                Vector2i p(0,0);
                Vector2f rel(0.0, (1.0 / (games.size()) * 20) );
                (VScrollPanel*)vscroll->scrollEvent(p,rel);
            }
        }
      }

    }
    if (keycode == "a") {
        mFocus->mouseButtonEvent(mFocus->position(), SDL_BUTTON_LEFT, true, 0);
    }
  }
 
  if (action == 0 && keycode == "a" ) {
    mFocus->mouseButtonEvent(mFocus->position(), SDL_BUTTON_LEFT, false, 0);
  }    

  return false;
}

void MenuScreen::setBackGroundImage( const std::string & fname ){
  if(imageid_!=0){
    nvgDeleteImage(mNVGContext,imageid_);
  }
  imageid_ = nvgCreateImage(mNVGContext, fname.c_str(), 0 );
  nvgImageSize(mNVGContext, imageid_, &imgw_, &imgh_);
  MENU_LOG("imageid_:%d w:%d h:%d\n",imageid_,imgw_,imgh_);
	imgPaint_ = nvgImagePattern(mNVGContext, 0, 0, imgw_,imgh_, 0, imageid_, 0.5f);
}

void MenuScreen::setTmpBackGroundImage( const std::string & fname ){
  if( !fs::exists(fname) ) {
      setDefalutBackGroundImage();
      return;
  }

  if(imageid_tmp_!=0){
    nvgDeleteImage(mNVGContext,imageid_tmp_);
  }
  imageid_tmp_ = nvgCreateImage(mNVGContext, fname.c_str(), 0 );
  nvgImageSize(mNVGContext, imageid_, &imgw_, &imgh_);
  MENU_LOG("imageid_:%d w:%d h:%d\n",imageid_,imgw_,imgh_);
	imgPaint_ = nvgImagePattern(mNVGContext, 0, 0, imgw_,imgh_, 0, imageid_tmp_, 0.5f);
}

void MenuScreen::setDefalutBackGroundImage(){
  if(imageid_==0){
    return;
  }
  nvgImageSize(mNVGContext, imageid_, &imgw_, &imgh_);
	imgPaint_ = nvgImagePattern(mNVGContext, 0, 0, imgw_,imgh_, 0, imageid_, 0.5f);
}


void MenuScreen::draw(NVGcontext *ctx){

  if( imageid_ != 0 ){
    nvgSave(ctx);
		nvgBeginPath(ctx);
		nvgRect(ctx, 0, 0, imgw_,imgh_);
		nvgFillPaint(ctx, imgPaint_);
    //nvgFillColor(ctx, nvgRGBA(0,160,192,255));
		nvgFill(ctx);
    nvgRestore(ctx);
  }

  //window->center();
  Screen::draw(ctx);
}

void MenuScreen::setCurrentInputDevices( std::map<SDL_JoystickID, SDL_Joystick*> & joysticks ){

  joysticks_ = joysticks;
  
  vector<string> items;
  vector<string> itemsShort;

  json j;
  string selguid="BADGUID";
  std::stringstream ss;
  std::string userid;

  try{
    std::ifstream fin( config_file_ );
    fin >> j;
    fin.close();
  }catch ( json::exception& e ){

  }


  for( auto it = joysticks_.begin(); it != joysticks_.end() ; ++it ) {
      SDL_Joystick* joy = it->second;
      SDL_JoystickID joyId = SDL_JoystickInstanceID(joy);
      char guid[65];
      SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, 65);
      string key_string;
      InputManager::genJoyString( key_string, joyId, SDL_JoystickName(joy), guid );

      items.push_back(key_string);
      itemsShort.push_back(SDL_JoystickName(joy));
  }  
  itemsShort.push_back("KeyBoard");
  items.push_back("Keyboard_-1");
  itemsShort.push_back("Disable");
  items.push_back("Disable_-2");

  for( int i=0; i< player_configs_.size(); i++ ){
    ComboBox * cb = player_configs_[i].cb ;
    if( cb != nullptr ){
      cb->setItems(itemsShort,itemsShort); 
      std::stringstream ss;
      ss << "player" << (i+1);
      std::string userid = ss.str();      
      if( j.find(userid) != j.end() ) {
        InputManager::genJoyString( selguid, j[userid]["DeviceID"], j[userid]["deviceName"], j[userid]["deviceGUID"] );
      }else{
        selguid = "Disable_-2";
      }
      printf("setCurrentInputDevices %s:%s\n", userid.c_str() ,selguid.c_str() );
      for( int i=0; i<items.size(); i++  ){
        if( items[i] == selguid ){
          cb->setSelectedIndex(i);
          break;
        }
      }
    }
  }
  performLayout();
}

void MenuScreen::pushActiveMenu( Widget *active, Widget * button  ){
    PreMenuInfo tmp;
    tmp.window = active;
    tmp.button = button;
    menu_stack.push(tmp);
    if( mFocus != nullptr ){
      mFocus->mouseEnterEvent(mFocus->position(), false);
      const std::vector<Widget *> &children = active->children();
      for( int i=0; i<children.size(); i++ ){
        if(  dynamic_cast<Button*>(children[i]) != nullptr ){
          mFocus = children[i];
          mFocus->requestFocus();
          MENU_LOG("%s is selected\n",((nanogui::Button*)mFocus)->caption().c_str());
          mFocus->mouseEnterEvent(mFocus->position(), true);
          break;
        }
      }
  }
}

void MenuScreen::popActiveMenu(){
  MENU_LOG("popActiveMenu ");
  if( menu_stack.size() <= 1 ) return;
  PreMenuInfo tmp;
  tmp = menu_stack.top();
  PopupButton * p = dynamic_cast<PopupButton*>(tmp.button);
  if( p != nullptr ) {
    MENU_LOG("Hide menu\n");
    mFocus->mouseEnterEvent(p->absolutePosition(),false);
    p->setPushed(false);
    p->popup()->setVisible(false);
    p->mouseEnterEvent(p->absolutePosition(),true);
    mFocus = p;
    mFocus->requestFocus();
    menu_stack.pop();
  }else{
    tmp = menu_stack.top();
    /*
    const std::vector<Widget *> &children = tmp.window->children();
    for( int i=0; i<children.size(); i++ ){
      if(  dynamic_cast<Button*>(children[i]) != nullptr ){
        Widget * px = children[i];
        if( tmp.button != nullptr ) {
          mFocus->mouseEnterEvent(tmp.button->absolutePosition(),false);
          px->mouseEnterEvent(tmp.button->absolutePosition(),true);
          MENU_LOG("%s is selected\n",((nanogui::Button*)mFocus)->caption().c_str());
        }
        mFocus = px;
        mFocus->requestFocus();        
        break;
      }
    }
    */
    if( tmp.button != nullptr ) {
      mFocus->mouseEnterEvent(tmp.button->absolutePosition(),false);
      tmp.button->mouseEnterEvent(tmp.button->absolutePosition(),true);
      MENU_LOG("%s is selected\n",((nanogui::Button*)tmp.button)->caption().c_str());
    }
    mFocus = tmp.button;
    mFocus->requestFocus();        
    menu_stack.pop();
  }
  return;
}

Widget * MenuScreen::getActiveMenu(){
  PreMenuInfo tmp;
  tmp = menu_stack.top();
  return tmp.window;
}

int MenuScreen::onBackButtonPressed(){
  MENU_LOG("onBackButtonPressed\n");
  
  if( swindow != nullptr ){

    if( swindow->title() == "Select File"){ 
      this->popActiveMenu();
    }
    swindow->dispose();
    swindow = nullptr;
    return 1;    
  }

  if( imageWindow != nullptr ){
    this->popActiveMenu();
    imageWindow->dispose();
    imageWindow = nullptr;    
    return 1;
  }
  
 //if( swindow != nullptr ){ 
 //  printf("swindow != null\n");
 //  return 1; 
 //}

  Widget * item = getActiveMenu();
  if( item == tools ){
    printf("Finish\n");
    return 0;
  }
  popActiveMenu();
/*
  if( activeWindow == tools ){
    return 0;
  }
  player1->setPushed(false);
  player2->setPushed(false);
  activeWindow = tools;
  Widget * p = tools->children()[0];
  if( p != nullptr ) {
    p->mouseEnterEvent(p->absolutePosition(),true);
    mFocus = p;
    mFocus->focusEvent(true);
  }
*/  
  return 1;
}

