package org.uoyabause.android;

import com.activeandroid.ActiveAndroid;

/**
 * Created by shinya on 2015/12/30.
 */
public class YabauseApplication extends com.activeandroid.app.Application {
    @Override
    public void onCreate() {
        super.onCreate();
        ActiveAndroid.initialize(this);
    }
}
