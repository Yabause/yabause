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
package org.uoyabause.android.tv

import androidx.fragment.app.FragmentActivity
import android.os.Bundle
import org.devmiyax.yabasanshiro.R
import android.app.UiModeManager
import android.content.res.Configuration
import android.os.Build
import android.view.InputDevice
import android.view.KeyEvent
import java.lang.Runnable
import org.uoyabause.android.tv.TvUtil
import org.uoyabause.android.phone.GameSelectFragmentPhone

/*
 * MainActivity class that loads MainFragment
 */
class GameSelectActivity : FragmentActivity() {
    /**
     * Called when the activity is first created.
     */
    val TAG = "GameSelectActivity"
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_game_select)
        val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
            uiModeManager.currentModeType == Configuration.UI_MODE_TYPE_TELEVISION
        ) {
            Thread {
                val subscription = Subscription.createSubscription(
                    "RECENT",
                    "recent played games",
                    "saturngame://yabasanshiro/",
                    R.mipmap.ic_launcher
                )
                val channelId = TvUtil.createChannel(this@GameSelectActivity, subscription)
                TvUtil.syncPrograms(this@GameSelectActivity, channelId)
            }.start()
        } else {
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

    public override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
    }

    var frg_: GameSelectFragmentPhone? = null
    public override fun onResume() {
        super.onResume()
    }

    public override fun onStop() {
        super.onStop()
        val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
            uiModeManager.currentModeType == Configuration.UI_MODE_TYPE_TELEVISION
        ) {
            val subscription = Subscription.createSubscription(
                "RECENT",
                "recent played games",
                "saturngame://yabasanshiro/",
                R.mipmap.ic_launcher
            )
            val channelId = TvUtil.createChannel(this, subscription)
            TvUtil.syncPrograms(this, channelId)
        } else {
            // Use the recommendations row API
        }
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        val dev = InputDevice.getDevice(event.deviceId)
        if (dev != null && dev.name.contains("HuiJia")) {
            if (event.keyCode > 200) {
                return true
            }
        }
        return super.dispatchKeyEvent(event)
    }
}