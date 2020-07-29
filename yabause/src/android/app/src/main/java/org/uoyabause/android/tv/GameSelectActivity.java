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

import android.app.UiModeManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import androidx.appcompat.app.AppCompatActivity;

import android.view.InputDevice;
import android.view.KeyEvent;

import org.uoyabause.android.phone.GameSelectFragmentPhone;
import org.uoyabause.uranus.R;

import androidx.tvprovider.media.tv.Channel;
import androidx.tvprovider.media.tv.TvContractCompat;
import androidx.tvprovider.media.tv.ChannelLogoUtils;
import androidx.tvprovider.media.tv.PreviewProgram;
import androidx.tvprovider.media.tv.WatchNextProgram;

import static androidx.tvprovider.media.tv.ChannelLogoUtils.storeChannelLogo;

/*
 * MainActivity class that loads MainFragment
 */
public class GameSelectActivity extends FragmentActivity {
    /**
     * Called when the activity is first created.
     */
    final String TAG ="GameSelectActivity";


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_game_select);

        UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
                uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION ) {

            new Thread(new Runnable() {
                public void run(){
                    Subscription subscription = Subscription.createSubscription("RECENT","recent played games", "saturngame://yabasanshiro/", R.mipmap.ic_launcher );
                    long channelId = TvUtil.createChannel(GameSelectActivity.this,subscription);
                    TvUtil.syncPrograms(GameSelectActivity.this,channelId);
                }
            }).start();

        }
        else {
            // Use the recommendations row API
        }


/*
        SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean("donated", false);
        editor.apply();
*/
        //CheckDonated();

    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

    }

    GameSelectFragmentPhone frg_ = null;

    @Override
    public void onResume(){
        super.onResume();
    }

    @Override
    public void onStop(){
        super.onStop();
        UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
                uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION ) {

            Subscription subscription = Subscription.createSubscription("RECENT","recent played games", "saturngame://yabasanshiro/", R.mipmap.ic_launcher );
            long channelId = TvUtil.createChannel(this,subscription);
            TvUtil.syncPrograms(this,channelId);
        }
        else {
            // Use the recommendations row API
        }
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
