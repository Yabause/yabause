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

import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.net.Uri
import androidx.multidex.MultiDex
import androidx.multidex.MultiDexApplication
import com.activeandroid.ActiveAndroid
import com.activeandroid.Configuration
import com.google.android.gms.analytics.GoogleAnalytics
import com.google.android.gms.analytics.Tracker
import com.google.firebase.FirebaseApp
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.cheat.Cheat

class YabauseApplication : MultiDexApplication() {
    private var mTracker: Tracker? = null
    val TAG = "YabauseApplication"
    override fun attachBaseContext(base: Context) {
        super.attachBaseContext(base)
        MultiDex.install(this)
    }

    override fun onCreate() {
        super.onCreate()
        val config = Configuration.Builder(this)
            .setDatabaseName("localgameinfo.db")
            .setDatabaseVersion(2)
            .setModelClasses(GameInfo::class.java,
                GameStatus::class.java,
                Cheat::class.java)
            .create()
        ActiveAndroid.initialize(config)
        appContext = applicationContext
        FirebaseApp.initializeApp(applicationContext)

        // Log.d(TAG,"Firebase token: " + FirebaseInstanceId.getInstance().getToken() );
    } // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG

    /**
     * Gets the default [Tracker] for this [Application].
     * @return tracker
     */
    @get:Synchronized
    val defaultTracker: Tracker?
        get() {
            if (mTracker == null) {
                val analytics = GoogleAnalytics.getInstance(this)
                // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG
                mTracker = analytics.newTracker(R.xml.global_tracker)
                mTracker!!.enableAdvertisingIdCollection(true)
            }
            return mTracker
        }

    companion object {
        lateinit var appContext: Context
            private set

        fun isPro(): Boolean {
            val prefs: SharedPreferences? = appContext.getSharedPreferences("private",
                Context.MODE_PRIVATE)
            var hasDonated = false
            if (prefs != null) {
                hasDonated = prefs.getBoolean("donated", false)
            }
            if (BuildConfig.BUILD_TYPE == "pro" || hasDonated) {
                return true
            }
            return false
        }

        fun checkDonated(ctx: Context): Int {
            if (BuildConfig.BUILD_TYPE == "debug") {
                return 0
            }
            if (BuildConfig.BUILD_TYPE != "pro") {
                val prefs = ctx.getSharedPreferences("private", MODE_PRIVATE)
                val hasDonated = prefs.getBoolean("donated", false)
                if (hasDonated == false) {
                    val builder = AlertDialog.Builder(ctx)
                    builder.setTitle(R.string.not_available)
                    builder.setMessage(R.string.only_pro_version)
                    builder.setPositiveButton(R.string.got_it
                    ) { _, _ ->
                        val url =
                            "https://play.google.com/store/apps/details?id=org.devmiyax.yabasanshioro2.pro"
                        val intent = Intent(Intent.ACTION_VIEW)
                        intent.data = Uri.parse(url)
                        intent.setPackage("com.android.vending")
                        ctx.startActivity(intent)
                    }
                    builder.setNegativeButton(R.string.cancel
                    ) { _, _ -> }
                    builder.create().show()
                    return -1
                }
            }
            return 0
        }

        fun getVersionName(context: Context): String? {
            val pm = context.packageManager
            var versionName = ""
            try {
                val packageInfo = pm.getPackageInfo(context.packageName, 0)
                versionName = packageInfo.versionName
            } catch (e: PackageManager.NameNotFoundException) {
                e.printStackTrace()
            }
            return versionName
        }
    }
}
