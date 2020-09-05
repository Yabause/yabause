package org.devmiyax.yabasanshiro;

/*  Copyright 2019 devMiyax(smiyaxdev@gmail.com)

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

import android.app.Application;

import com.activeandroid.Configuration;

import  org.uoyabause.android.GameInfo;

public class TestApplication extends Application {
  @Override
  public void onCreate() {
    super.onCreate();
    // Create configurations for a temporary mock database
    Configuration.Builder configuration = new Configuration.Builder(this).setDatabaseName(null);
    configuration.addModelClasses(GameInfo.class);
    // Initialize ActiveAndroid DB
    //ActiveAndroid.initialize(configuration.create());
  }

  @Override
  public void onTerminate() {
    // Dispose temporary database on termination
    //ActiveAndroid.dispose();
    super.onTerminate();
  }
}
