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
package org.devmiyax.yabasanshiro

import android.app.ActivityManager
import android.app.UiModeManager
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.graphics.Point
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.method.LinkMovementMethod
import android.util.DisplayMetrics
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.PreferenceManager
import androidx.window.layout.WindowMetricsCalculator
import com.google.android.gms.common.ConnectionResult
import com.google.android.gms.common.GoogleApiAvailability
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.remoteconfig.FirebaseRemoteConfig
import org.uoyabause.android.SettingsActivity
import org.uoyabause.android.phone.GameSelectActivityPhone
import org.uoyabause.android.tv.GameSelectActivity

class StartupActivity : AppCompatActivity() {
    val TAG = "StartupActivity"

    private var mFirebaseRemoteConfig: FirebaseRemoteConfig? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_startup)

        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)

        val video = sharedPref.getString("pref_video", "-1")
        if( video == "-1" ){

            val editor = sharedPref.edit()
            if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL)) {
                // Defulat is Vulkan!
                editor.putString("pref_video", "4")
                editor.putBoolean("pref_use_compute_shader", true)
                editor.putString("pref_polygon_generation", "2")
            }else {
                val activityManager = getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
                val configurationInfo = activityManager.deviceConfigurationInfo
                val supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000
                if (supportsEs3) {
                    editor.putString("pref_video", "1")
                }else{
                    editor.putString("pref_video", "2")
                }
                editor.putBoolean("pref_use_compute_shader", false)
                editor.putString("pref_polygon_generation", "0")
            }
            editor.apply()

        }


        val aspectRateSetting = sharedPref.getString("pref_aspect_rate", "BAD")
        if (aspectRateSetting == "BAD") {
            val editor = sharedPref.edit()
            val windowMetrics = WindowMetricsCalculator.getOrCreate().computeCurrentWindowMetrics(this)
            val currentBounds = windowMetrics.bounds
            val width = currentBounds.width()
            val height = currentBounds.height()
            val arate = width.toDouble() / height.toDouble()
            if (arate >= 1.21 && arate <= 1.34) {
                editor.putString("pref_aspect_rate", "3")
            } else {
                editor.putString("pref_aspect_rate", "0")
            }
            editor.apply()
        }

        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        val googleAPI = GoogleApiAvailability.getInstance()
        val resultCode = googleAPI.isGooglePlayServicesAvailable(this)
        if (resultCode != ConnectionResult.SUCCESS) {
            Log.e(TAG, "This device is not supported.")
        }
        // Log.d(TAG, "InstanceID token: " + FirebaseInstanceId.getInstance().token)
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser != null) {
            // already signed in
        } else {
            // not signed in
        }

        val r = Runnable {
            mFirebaseRemoteConfig = FirebaseRemoteConfig.getInstance()
            mFirebaseRemoteConfig!!.setDefaultsAsync(R.xml.config)
            val cacheExpiration: Long = 3600 // 1 hour in seconds.
            mFirebaseRemoteConfig!!.fetch(cacheExpiration)
                .addOnCompleteListener(this) { task ->
                    if (task.isSuccessful) {
                        mFirebaseRemoteConfig!!.fetchAndActivate()
                    }
                }

            val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
            val sharedPrefLocal = PreferenceManager.getDefaultSharedPreferences(this@StartupActivity)
            val tvmode = sharedPrefLocal.getBoolean("pref_force_androidtv_mode", false)
            val i: Intent
            if (uiModeManager.currentModeType == Configuration.UI_MODE_TYPE_TELEVISION || tvmode) {
                i = Intent(this@StartupActivity, GameSelectActivity::class.java)
                Log.d(TAG, "executing: GameSelectActivity")
            } else {
                i = Intent(this@StartupActivity, GameSelectActivityPhone::class.java)
                Log.d(TAG, "executing: GameSelectActivityPhone")
            }

            val sargs = intent.dataString
            if (sargs != null) {
                Log.d(TAG, "getDataString = $sargs")
                if (sargs.contains("//login")) {
                    i.putExtra("showPin", true)
                }
            }
            val args = intent.data
            if (args != null && !args.pathSegments.isEmpty()) {
                Log.d(TAG, "getData = $args")
                i.data = args
            }
            startActivity(i)
            return@Runnable
        }

        val prefs = getSharedPreferences("private", Context.MODE_PRIVATE)
        val agreed = prefs.getBoolean("agreed", false)

        val v = findViewById<View>(R.id.agree)
        if (agreed) {
            v.visibility = View.GONE
            Handler(Looper.getMainLooper()).postDelayed(r, 2000)
        } else {

            val t2 = findViewById<TextView>(R.id.agree_text)
            t2.movementMethod = LinkMovementMethod.getInstance()

            val b = findViewById<Button>(R.id.agree_and_start)
            b.requestFocus()
            b.setOnClickListener {
                prefs.edit().apply {
                    putBoolean("agreed", true)
                    commit()
                }
                v.visibility = View.GONE
                Handler(Looper.getMainLooper()).postDelayed(r, 1)
            }
            v.visibility = View.VISIBLE
        }
    }
}
