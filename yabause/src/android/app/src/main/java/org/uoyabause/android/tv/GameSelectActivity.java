/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package org.uoyabause.android.tv;

import android.Manifest;
import android.app.Activity;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.firebase.iid.FirebaseInstanceId;

import org.uoyabause.android.R;

/*
 * MainActivity class that loads MainFragment
 */
public class GameSelectActivity extends Activity {
    /**
     * Called when the activity is first created.
     */
    final String TAG ="GameSelectActivity";


    @Override
    public void onCreate(Bundle savedInstanceState) {

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
        if( lock_landscape == true ){
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }else{
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        }

        super.onCreate(savedInstanceState);

        GoogleApiAvailability googleAPI = GoogleApiAvailability.getInstance();
        int resultCode = googleAPI.isGooglePlayServicesAvailable(this);

        if (resultCode != ConnectionResult.SUCCESS) {
            Log.e(TAG, "This device is not supported.");
        }

        Log.d(TAG, "InstanceID token: " + FirebaseInstanceId.getInstance().getToken());

        setContentView(R.layout.activity_game_select);
    }

    @Override
    public void onResume(){
        super.onResume();
    }

    @Override
    public boolean dispatchKeyEvent (KeyEvent event){

        InputDevice dev = InputDevice.getDevice(event.getDeviceId());
        if( dev != null && dev.getName().contains("HuiJia")){
            if( event.getKeyCode() > 200 ){
                return true;
            }

        }
        return super.dispatchKeyEvent(event);
    }
}
