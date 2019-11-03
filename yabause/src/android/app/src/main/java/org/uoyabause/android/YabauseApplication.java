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


package org.uoyabause.android;

import android.content.Context;

import androidx.multidex.MultiDex;
import androidx.multidex.MultiDexApplication;

import com.activeandroid.ActiveAndroid;
import com.activeandroid.Configuration;
import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.ndk.CrashlyticsNdk;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Tracker;
import com.google.firebase.FirebaseApp;

import org.uoyabause.android.cheat.Cheat;
import org.uoyabause.uranus.R;

import io.fabric.sdk.android.Fabric;

public class YabauseApplication extends MultiDexApplication {

    private static Context context;
    private Tracker mTracker;
    final String TAG ="YabauseApplication";

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        MultiDex.install(this);
    }

    Crashlytics _crash  = new Crashlytics();
    CrashlyticsNdk _crashndk = new CrashlyticsNdk();

    @Override
    public void onCreate() {
        super.onCreate();
        Configuration config = new Configuration.Builder(this)
                .setDatabaseName("localgameinfo.db")
                .setDatabaseVersion(2)
                .setModelClasses(org.uoyabause.android.GameInfo.class,
                        org.uoyabause.android.GameStatus.class,
                        Cheat.class )
                .create();
        ActiveAndroid.initialize(config);

        YabauseApplication.context = getApplicationContext();
        Fabric fabric = new Fabric.Builder(this)
                .kits(_crash, _crashndk)
                .build();
        Fabric.with(fabric);

        FirebaseApp.initializeApp(YabauseApplication.context);

       // Log.d(TAG,"Firebase token: " + FirebaseInstanceId.getInstance().getToken() );
    }

    Crashlytics getCrashlytics() { return _crash; }


    public static Context getAppContext() {
        return YabauseApplication.context;
    }

    /**
     * Gets the default {@link Tracker} for this {@link Application}.
     * @return tracker
     */
    synchronized public Tracker getDefaultTracker() {
        if (mTracker == null) {
            GoogleAnalytics analytics = GoogleAnalytics.getInstance(this);
            // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG
            mTracker = analytics.newTracker(R.xml.global_tracker);
            mTracker.enableAdvertisingIdCollection(true);
        }
        return mTracker;
    }

}
