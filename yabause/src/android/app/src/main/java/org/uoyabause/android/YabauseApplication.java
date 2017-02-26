package org.uoyabause.android;

import android.content.Context;

import com.activeandroid.ActiveAndroid;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Logger;
import com.google.android.gms.analytics.Tracker;
import com.google.firebase.FirebaseApp;

/**
 * Created by shinya on 2015/12/30.
 */
public class YabauseApplication extends com.activeandroid.app.Application {

    private static Context context;
    private Tracker mTracker;

    @Override
    public void onCreate() {
        super.onCreate();
        ActiveAndroid.initialize(this);
        YabauseApplication.context = getApplicationContext();

        FirebaseApp.initializeApp(YabauseApplication.context);
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
            //GoogleAnalytics analytics = GoogleAnalytics.getInstance(this);
            // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG
            //mTracker = analytics.newTracker(R.xml.global_tracker);
            //mTracker.enableAdvertisingIdCollection(true);
        }
        return mTracker;
    }

}
