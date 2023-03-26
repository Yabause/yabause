package org.uoyabause.yabasanshiro;

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

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;
import org.uoyabause.android.GameInfo;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

@RunWith(RobolectricTestRunner.class)
@Config(
    //constants = BuildConfig.class,
    application = GameInfoUnitText.TestApplication.class,    // Run with TestApplication instead of actual
    sdk = 21                                // Test against Lollipop
)
public class GameInfoUnitText {

  @Before
  public void setUp() throws Exception {
    ShadowLog.stream = System.out;
    //mInstance = DatabaseUtils.getInstance();
  }
  @Test
  public void retrieveFromDatabase() throws Exception {
    assertEquals(1, 1);
  }

  public static class TestApplication extends Application {
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
}
