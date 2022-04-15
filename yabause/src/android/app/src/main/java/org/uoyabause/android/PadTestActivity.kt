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
package org.uoyabause.android

import androidx.appcompat.app.AppCompatActivity
import org.uoyabause.android.PadTestFragment.PadTestListener
import org.uoyabause.android.YabausePad
import org.uoyabause.android.PadManager
import android.widget.TextView
import android.os.Bundle
import android.content.SharedPreferences
import android.content.pm.ActivityInfo
import android.view.View
import android.view.Window
import android.widget.FrameLayout
import android.widget.SeekBar
import androidx.fragment.app.Fragment
import androidx.preference.PreferenceManager
import org.uoyabause.android.PadTestActivity
import org.uoyabause.android.PadTestFragment

class PadTestActivity : AppCompatActivity(), PadTestListener {
    var mPadView: YabausePad? = null
    var mSlide: SeekBar? = null
    var mTransSlide: SeekBar? = null
    private val padm: PadManager? = null
    var tv: TextView? = null
    public override fun onCreate(savedInstanceState: Bundle?) {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        val lock_landscape = sharedPref.getBoolean("pref_landscape", false)
        requestedOrientation = if (lock_landscape == true) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        }
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)

        // Immersive mode
        val decor = this.window.decorView
        decor.systemUiVisibility =
            View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_IMMERSIVE
        val frame = FrameLayout(this)
        frame.id = CONTENT_VIEW_ID
        setContentView(frame, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT))
        if (savedInstanceState == null) {
            val newFragment: Fragment = PadTestFragment()
            (newFragment as PadTestFragment).setListener(this)
            val ft = supportFragmentManager.beginTransaction()
            ft.add(CONTENT_VIEW_ID, newFragment, PadTestFragment.TAG).commit()
        }
    }

    override fun onBackPressed() {
        val fg2 = supportFragmentManager.findFragmentByTag(PadTestFragment.TAG) as PadTestFragment?
        if (fg2 != null) {
            fg2.onBackPressed()
            return
        }
        super.onBackPressed()
    }

    override fun onFinish() {
        finish()
    }

    override fun onCancel() {
        finish()
    }

    companion object {
        private const val CONTENT_VIEW_ID = 10101010
    }
}