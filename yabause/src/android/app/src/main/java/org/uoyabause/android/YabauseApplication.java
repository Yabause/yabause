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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;

import androidx.multidex.MultiDex;
import androidx.multidex.MultiDexApplication;

import com.activeandroid.ActiveAndroid;
import com.activeandroid.Configuration;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Tracker;
import com.google.firebase.FirebaseApp;

import org.uoyabause.android.cheat.Cheat;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;

public class YabauseApplication extends MultiDexApplication {

    private static Context context;
    private Tracker mTracker;
    final String TAG ="YabauseApplication";

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        MultiDex.install(this);
    }

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
        FirebaseApp.initializeApp(YabauseApplication.context);

       // Log.d(TAG,"Firebase token: " + FirebaseInstanceId.getInstance().getToken() );
    }


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

    public static int checkDonated( final Context ctx ){
        if( !BuildConfig.BUILD_TYPE.equals("pro") ) {
            SharedPreferences prefs = ctx.getSharedPreferences("private", Context.MODE_PRIVATE);
            Boolean hasDonated = prefs.getBoolean("donated", false);
            if (hasDonated == false) {
                AlertDialog.Builder builder = new AlertDialog.Builder(ctx);
                builder.setTitle(R.string.not_available);
                builder.setMessage(R.string.only_pro_version);
                builder.setPositiveButton(R.string.got_it,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                String url = "https://play.google.com/store/apps/details?id=org.uoyabause.uranus.pro";
                                Intent intent = new Intent(Intent.ACTION_VIEW);
                                intent.setData(Uri.parse(url));
                                intent.setPackage("com.android.vending");
                                ctx.startActivity(intent);
                            }
                        }
                );
                builder.setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {

                            }
                        }
                );
                builder.create().show();
                return -1;
            }
        }
        return 0;
    }


}
