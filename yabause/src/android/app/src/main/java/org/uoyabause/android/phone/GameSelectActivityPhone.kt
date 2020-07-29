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
package org.uoyabause.android.phone

import android.content.Context
import android.content.res.Configuration
import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.view.MenuItem
import android.view.Window
import android.view.WindowManager
import android.widget.FrameLayout
import android.widget.LinearLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import net.nend.android.NendAdListener
import net.nend.android.NendAdView
import org.uoyabause.uranus.BuildConfig
import org.uoyabause.uranus.R

class GameSelectActivityPhone : AppCompatActivity(), NendAdListener {
    lateinit var frg_: GameSelectFragmentPhone
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val window: Window = getWindow()
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS)
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
        window.setStatusBarColor(getResources().getColor(R.color.colorPrimaryDark))

        val frame = FrameLayout(this)
        frame.id = CONTENT_VIEW_ID
        setContentView(
            frame, FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT
            )
        )
        if (savedInstanceState == null) {
            frg_ = GameSelectFragmentPhone()

            val ft = supportFragmentManager.beginTransaction()
            ft.add(CONTENT_VIEW_ID, frg_).commit()
        } else {
            frg_ =
                supportFragmentManager.findFragmentById(CONTENT_VIEW_ID) as GameSelectFragmentPhone
        }
        if (BuildConfig.BUILD_TYPE != "pro") {
            val prefs =
                getSharedPreferences("private", Context.MODE_PRIVATE)
            val hasDonated = prefs.getBoolean("donated", false)
            if (hasDonated == false) {
                try {
                    val nendAdView = NendAdView(
                        this,
                        getString(R.string.nend_spoid).toInt(),
                        getString(R.string.nend_apikey)
                    )
                    nendAdView.setListener(this)
                    val params = FrameLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT
                    )
                    params.gravity = Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL
                    frame.addView(nendAdView, params)
                    nendAdView.bringToFront()
                    nendAdView.invalidate()
                    ViewCompat.setTranslationZ(nendAdView, 90f)
                    nendAdView.loadAd()
                } catch (e: Exception) {
                }
            }
        }
        /*
        GameSelectFragmentPhone frg = new GameSelectFragmentPhone();
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        //transaction.replace(R.id.fragment_container, newFragment);
        transaction.add(frg,"mainfrag");
        transaction.addToBackStack(null);
        transaction.commit();
*/
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (frg_ != null) {
            frg_!!.onConfigurationChanged(newConfig)
        }
        // mDrawerToggle.onConfigurationChanged(newConfig);
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean { // Pass the event to ActionBarDrawerToggle, if it returns
// true, then it has handled the app icon touch event
// if (mDrawerToggle.onOptionsItemSelected(item)) {
//    return true;
// }

        if (frg_ != null) {
            val rtn = frg_!!.onOptionsItemSelected(item)
            if (rtn == true) {
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    override fun onFailedToReceiveAd(nendAdView: NendAdView) { // Toast.makeText(getApplicationContext(), "onFailedToReceiveAd", Toast.LENGTH_LONG).show();
        val nendError = nendAdView.nendError
        when (nendError) {
            NendAdView.NendError.INVALID_RESPONSE_TYPE -> {
            }
            NendAdView.NendError.FAILED_AD_DOWNLOAD -> {
            }
            NendAdView.NendError.FAILED_AD_REQUEST -> {
            }
            NendAdView.NendError.AD_SIZE_TOO_LARGE -> {
            }
            NendAdView.NendError.AD_SIZE_DIFFERENCES -> {
            }
        }
        Log.e("nend", nendError.message)
    }

    override fun onReceiveAd(nendAdView: NendAdView) { // Toast.makeText(getApplicationContext(), "onReceiveAd", Toast.LENGTH_LONG).show();
        nendAdView.bringToFront()
        nendAdView.invalidate()
    }

    override fun onClick(nendAdView: NendAdView) { // Toast.makeText(getApplicationContext(), "onClick", Toast.LENGTH_LONG).show();
    }

    override fun onDismissScreen(nendAdView: NendAdView) { // Toast.makeText(getApplicationContext(), "onDismissScreen", Toast.LENGTH_LONG).show();
    }

    companion object {
        const val CONTENT_VIEW_ID = 10101010
    }
}