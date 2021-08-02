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

import android.app.UiModeManager
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.Bundle
import android.os.Handler
import android.text.method.LinkMovementMethod
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.PreferenceManager
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.PurchasesUpdatedListener
import com.google.android.gms.analytics.Tracker
import com.google.android.gms.common.ConnectionResult
import com.google.android.gms.common.GoogleApiAvailability
import com.google.firebase.FirebaseApp
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.iid.FirebaseInstanceId
import com.google.firebase.remoteconfig.FirebaseRemoteConfig
import com.google.firebase.remoteconfig.FirebaseRemoteConfigSettings
import org.uoyabause.android.phone.GameSelectActivityPhone
import org.uoyabause.android.tv.GameSelectActivity
import org.uoyabause.util.IabHelper


class StartupActivity : AppCompatActivity() {
    val TAG = "StartupActivity"
    var mHelper: IabHelper? = null
    private var mFirebaseRemoteConfig: FirebaseRemoteConfig? = null

    private val purchasesUpdatedListener =
        PurchasesUpdatedListener { billingResult, purchases ->
            // To be implemented in a later section.
        }

    lateinit private var billingClient : BillingClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_startup)


        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        //boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
        //if( lock_landscape == true ){
        //    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        //}else{
        //    setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        //}
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        val googleAPI = GoogleApiAvailability.getInstance()
        val resultCode = googleAPI.isGooglePlayServicesAvailable(this)
        if (resultCode != ConnectionResult.SUCCESS) {
            Log.e(TAG, "This device is not supported.")
        }
        Log.d(TAG, "InstanceID token: " + FirebaseInstanceId.getInstance().token)
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser != null) {
            // already signed in
        } else {
            // not signed in
        }

        val handler = Handler()
        val r = Runnable {

            mFirebaseRemoteConfig = FirebaseRemoteConfig.getInstance()
            val configSettings = FirebaseRemoteConfigSettings.Builder()
                .setDeveloperModeEnabled(BuildConfig.DEBUG)
                .build()
            mFirebaseRemoteConfig!!.setConfigSettings(configSettings)
            mFirebaseRemoteConfig!!.setDefaults(R.xml.config)
            var cacheExpiration: Long = 3600 // 1 hour in seconds.
            // If your app is using developer mode, cacheExpiration is set to 0, so each fetch will
            // retrieve values from the service.
            if (mFirebaseRemoteConfig!!.info.configSettings.isDeveloperModeEnabled) {
                cacheExpiration = 0
            }
            mFirebaseRemoteConfig!!.fetch(cacheExpiration)
                .addOnCompleteListener(this) { task ->
                    if (task.isSuccessful) {
                        //Toast.makeText(GameSelectActivity.this, "Fetch Succeeded",
                        //        Toast.LENGTH_SHORT).show();

                        // After config data is successfully fetched, it must be activated before newly fetched
                        // values are returned.
                        mFirebaseRemoteConfig!!.activateFetched()
                    } else {
                        //Toast.makeText(GameSelectActivity.this, "Fetch Failed",
                        //        Toast.LENGTH_SHORT).show();
                    }
                    //displayWelcomeMessage();
                }


            billingClient = BillingClient.newBuilder(this)
                .setListener(purchasesUpdatedListener)
                .enablePendingPurchases()
                .build()

            billingClient.startConnection(object : BillingClientStateListener {
                override fun onBillingSetupFinished(billingResult: BillingResult) {
                    if (billingResult.responseCode ==  BillingClient.BillingResponseCode.OK) {
                        // The BillingClient is ready. You can query purchases here.
                    }
                }
                override fun onBillingServiceDisconnected() {
                    // Try to restart the connection on the next request to
                    // Google Play by calling the startConnection() method.
                }
            })

            val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(this@StartupActivity)
            val tvmode = sharedPref.getBoolean("pref_force_androidtv_mode", false)
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
        val any = if (agreed) {
            v.visibility = View.GONE
            handler.postDelayed(r, 2000)
        } else {

            val t2 = findViewById<TextView>(R.id.agree_text)
            t2.movementMethod = LinkMovementMethod.getInstance()

            val b = findViewById<Button>(R.id.agree_and_start)
            b.setOnClickListener {
                prefs.edit().apply {
                    putBoolean("agreed",true)
                    commit()
                }
                handler.postDelayed(r, 1)
            }
            v.visibility = View.VISIBLE
        }
    }

}