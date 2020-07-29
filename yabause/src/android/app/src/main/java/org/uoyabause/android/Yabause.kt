/*  Copyright 2011-2013 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
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

import android.app.Activity
import android.app.ActivityManager
import android.app.AlertDialog
import android.app.Dialog
import android.app.ProgressDialog
import android.app.UiModeManager
import android.content.ComponentCallbacks2
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.hardware.input.InputManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.os.Process.killProcess
import android.os.Process.myPid
import android.preference.PreferenceManager
import android.util.Base64
import android.util.Log
import android.view.KeyEvent
import android.view.MenuItem
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.drawerlayout.widget.DrawerLayout.DrawerListener
import androidx.transition.Fade
import com.activeandroid.query.Select
import com.activeandroid.util.IOUtils
import com.firebase.ui.auth.AuthUI
import com.firebase.ui.auth.AuthUI.IdpConfig.GoogleBuilder
import com.firebase.ui.auth.IdpResponse
import com.google.android.gms.ads.AdListener
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.AdSize
import com.google.android.gms.ads.AdView
import com.google.android.gms.analytics.HitBuilders.ScreenViewBuilder
import com.google.android.gms.analytics.Tracker
import com.google.android.gms.tasks.OnFailureListener
import com.google.android.gms.tasks.OnSuccessListener
import com.google.android.material.navigation.NavigationView
import com.google.android.material.snackbar.Snackbar
import com.google.firebase.analytics.FirebaseAnalytics
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.storage.FirebaseStorage
import io.reactivex.Observable
import io.reactivex.ObservableEmitter
import io.reactivex.ObservableOnSubscribe
import io.reactivex.Observer
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.Disposable
import io.reactivex.schedulers.Schedulers
import org.json.JSONObject
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.InputSettingFragment.InputSettingListener
import org.uoyabause.android.PadManager.ShowMenuListener
import org.uoyabause.android.PadTestFragment.PadTestListener
import org.uoyabause.android.StateListFragment.checkMaxFileCount
import org.uoyabause.android.backup.TabBackupFragment
import org.uoyabause.android.cheat.TabCheatFragment
import org.uoyabause.uranus.BuildConfig
import org.uoyabause.uranus.R
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.text.DateFormat
import java.text.SimpleDateFormat
import java.util.Arrays
import java.util.Date
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream
import kotlin.collections.ArrayList

internal class YabauseHandler(private val yabause: Yabause) : Handler() {
    override fun handleMessage(msg: Message) {
        yabause.showDialog(msg.what, msg.data)
    }
} // AppCompatActivity

class Yabause : AppCompatActivity(), FileSelectedListener,
    NavigationView.OnNavigationItemSelectedListener, SelInputDeviceFragment.InputDeviceListener,
    PadTestListener, InputSettingListener, InputManager.InputDeviceListener, ShowMenuListener {
    private var yabauseThread: YabauseRunnable? = null
    private var handler: YabauseHandler? = null
    private var audio: YabauseAudio? = null
    var biosPath: String? = null
        private set
    var gamePath: String? = null
        private set
    var cartridgeType = 0
        private set
    private var padm: PadManager? = null
    var videoInterface = 0
        private set
    private var waiting_reault = false
    private var mTracker: Tracker? = null
    private var tray_state = 0
    var mDrawerLayout: DrawerLayout? = null
    var mNavigationView: NavigationView? = null
    var adView: AdView? = null
    private var mFirebaseAnalytics: FirebaseAnalytics? = null
    val PAD_SETTING = 0
    var mInputManager: InputManager? = null
    val RC_SIGN_IN = 0x8010
    private var mProgressDialog: ProgressDialog? = null
    private var isShowProgress: Boolean? = null
    private var gameCode: String? = null
    private var testCase: String? = ""
    fun print(msg: String?) {
        val calledClass = Thread.currentThread().stackTrace[3]
        Log.d(
            calledClass.fileName + ":" +
                    calledClass.lineNumber, msg
        )
    }

    fun showDialog() {
        mProgressDialog = ProgressDialog(this)
        mProgressDialog!!.setMessage("Sending...")
        mProgressDialog!!.show()
        waiting_reault = true
        isShowProgress = true
    }

    fun dismissDialog() {
        mProgressDialog!!.dismiss()
        mProgressDialog = null
        isShowProgress = false
        waiting_reault = false
        val layout = findViewById<View>(R.id.drawer_layout) as DrawerLayout
        when (_report_status) {
            REPORT_STATE_INIT -> Snackbar.make(
                layout,
                "Fail to send your report. internal error",
                Snackbar.LENGTH_SHORT
            ).show()
            REPORT_STATE_SUCCESS -> Snackbar.make(
                layout,
                "Success to send your report. Thank you for your collaboration.",
                Snackbar.LENGTH_SHORT
            ).show()
            REPORT_STATE_FAIL_DUPE -> Snackbar.make(
                layout,
                "Fail to send your report. You've sent a report for same game, same device and same vesion.",
                Snackbar.LENGTH_SHORT
            ).show()
            REPORT_STATE_FAIL_CONNECTION -> Snackbar.make(
                layout,
                "Fail to send your report. Server is down.",
                Snackbar.LENGTH_SHORT
            ).show()
            REPORT_STATE_FAIL_AUTH -> Snackbar.make(
                layout,
                "Fail to send your report. Authorizing is failed.",
                Snackbar.LENGTH_SHORT
            ).show()
        }
        toggleMenu()
    }

    /**
     * Called when the activity is first created.
     */
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this@Yabause)
        val lock_landscape = sharedPref.getBoolean("pref_landscape", false)
        requestedOrientation = if (lock_landscape == true) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        }
        mInputManager = getSystemService(Context.INPUT_SERVICE) as InputManager
        System.gc()
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(this)
        val application = application as YabauseApplication
        mTracker = application.defaultTracker
        setContentView(R.layout.main)
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                window.setSustainedPerformanceMode(true)
            }
        } catch (e: Exception) {
            // Do Nothing
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER
        }
        if (sharedPref.getBoolean("pref_immersive_mode", false)) {
            window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)
        }
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        mDrawerLayout = findViewById<View>(R.id.drawer_layout) as DrawerLayout
        updateViewLayout(resources.configuration.orientation)
        mNavigationView = findViewById<View>(R.id.nav_view) as NavigationView
        mNavigationView!!.setNavigationItemSelectedListener(this)
        val menu = mNavigationView!!.menu
        if (BuildConfig.BUILD_TYPE != "debug") {
            val rec = menu.findItem(R.id.record)
            if (rec != null) {
                rec.isVisible = false
            }
            val play = menu.findItem(R.id.play)
            if (play != null) {
                play.isVisible = false
            }
        }
        val drawerListener: DrawerListener = object : DrawerListener {
            override fun onDrawerSlide(view: View, v: Float) {
                // Log.d(this.javaClass.name,"onDrawerSlide ${v}")
            }

            override fun onDrawerOpened(view: View) {
                // Log.d(this.javaClass.name,"onDrawerOpened")
            }

            override fun onDrawerClosed(view: View) {
                // Log.d(this.javaClass.name,"onDrawerClosed")
                if (waiting_reault == false && menu_showing == true) {
                    menu_showing = false
                    YabauseRunnable.resume()
                    audio!!.unmute(audio!!.SYSTEM)
                }
            }

            override fun onDrawerStateChanged(i: Int) {
                // Log.d(this.javaClass.name,"onDrawerStateChanged")
            }
        }
        mDrawerLayout!!.addDrawerListener(drawerListener)
        audio = YabauseAudio(this)
        val intent = intent
        val bundle = intent.extras
        if (bundle != null) {
            for (key in bundle.keySet()) {
                Log.e(TAG, key + " : " + if (bundle[key] != null) bundle[key] else "NULL")
            }
        }
        val game = intent.getStringExtra("org.uoyabause.android.FileName")
        if (game != null && game.length > 0) {
            val storage = YabauseStorage.getStorage()
            gamePath = storage.getGamePath(game)
        } else gamePath = ""
        val exgame = intent.getStringExtra("org.uoyabause.android.FileNameEx")
        if (exgame != null) {
            gamePath = exgame
        }
        Log.d(TAG, "File is " + gamePath)
        val gameCode = intent.getStringExtra("org.uoyabause.android.gamecode")
        if (gameCode != null) {
            this.gameCode = gameCode
        } else {
            var gameinfo = GameInfo.getFromFileName(gamePath)
            if (gameinfo != null) {
                this.gameCode = gameinfo.product_number
            } else {
                gameinfo = if (gamePath!!.toUpperCase().endsWith("CUE")) {
                    GameInfo.genGameInfoFromCUE(gamePath)
                } else if (gamePath!!.toUpperCase().endsWith("MDS")) {
                    GameInfo.genGameInfoFromMDS(gamePath)
                } else if (gamePath!!.toUpperCase().endsWith("CCD")) {
                    GameInfo.genGameInfoFromMDS(gamePath)
                } else if (gamePath!!.toUpperCase().endsWith("CHD")) {
                    GameInfo.genGameInfoFromCHD(gamePath)
                } else {
                    GameInfo.genGameInfoFromIso(gamePath)
                }
                if (gameinfo != null) {
                    this.gameCode = gameinfo.product_number
                }
            }
        }
        testCase = intent.getStringExtra("TestCase")
        PreferenceManager.setDefaultValues(this, R.xml.preferences, false)
        if (this.gameCode != null) {
            readPreferences(this.gameCode)
        }
        padm = PadManager.getPadManager()
        padm?.loadSettings()
        padm?.setShowMenulistener(this)
        waiting_reault = false
        handler = YabauseHandler(this)
        yabauseThread = YabauseRunnable(this)
        val uiModeManager = getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION && BuildConfig.BUILD_TYPE != "pro") {
            val prefs = getSharedPreferences("private", Context.MODE_PRIVATE)
            val hasDonated = prefs.getBoolean("donated", false)
            if (hasDonated) {
                adView = null
            } else {
                adView = AdView(this)
                adView!!.adUnitId = getString(R.string.banner_ad_unit_id2)
                adView!!.adSize = AdSize.BANNER
                val adRequest = AdRequest.Builder().build()
                adView!!.loadAd(adRequest)
                adView!!.adListener = object : AdListener() {
                    override fun onAdOpened() {
                        // Save app state before going to the ad overlay.
                    }
                }
            }
        } else {
            adView = null
        }
    }

    fun updateViewLayout(orientation: Int) {
        window.statusBarColor = resources.getColor(R.color.black)
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        var immersiveFlags = 0
        if (sharedPref.getBoolean("pref_immersive_mode", false)) {
            immersiveFlags =
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        }
        val decorView = window.decorView
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    or immersiveFlags
                    or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    or View.SYSTEM_UI_FLAG_FULLSCREEN
                    or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)
        } else if (orientation == Configuration.ORIENTATION_PORTRAIT) {
            decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_LAYOUT_STABLE or immersiveFlags
        }
    }

    override fun onConfigurationChanged(_newConfig: Configuration) {
        updateViewLayout(_newConfig.orientation)
        super.onConfigurationChanged(_newConfig)
    }

    var loginEmitter: ObservableEmitter<FirebaseUser?>? = null
    private fun checkAuth(loginObserver: Observer<FirebaseUser?>): Int {
        Observable.create(ObservableOnSubscribe<FirebaseUser?> { emitter ->
            loginEmitter = null
            val auth = FirebaseAuth.getInstance()
            val user = auth.currentUser
            if (user != null) {
                emitter.onNext(user)
                emitter.onComplete()
                return@ObservableOnSubscribe
            }
            loginEmitter = emitter
            startActivityForResult(
                AuthUI.getInstance()
                    .createSignInIntentBuilder()
                    .setAvailableProviders(
                        Arrays.asList(
                            GoogleBuilder().build()
                        )
                    )
                    .build(),
                RC_SIGN_IN
            )
        }).subscribeOn(AndroidSchedulers.mainThread())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(loginObserver)

        return 0
    }

    fun setSaveStateObserver(saveStateObserver: Observer<String?>) {
        Observable.create(ObservableOnSubscribe<String?> { emitter ->
            val auth = FirebaseAuth.getInstance()
            val user = auth.currentUser
            if (user == null) {
                emitter.onError(Exception("not login"))
                return@ObservableOnSubscribe
            }
            val save_path = YabauseStorage.getStorage().stateSavePath
            val current_gamecode = YabauseRunnable.getCurrentGameCode()
            val save_root = File(YabauseStorage.getStorage().stateSavePath, current_gamecode)
            if (!save_root.exists()) save_root.mkdir()
            val save_filename = YabauseRunnable.savestate_compress(save_path + current_gamecode)
            if (save_filename != "") {
                val storage = FirebaseStorage.getInstance()
                val storage_ref = storage.reference
                val base = storage_ref.child(user.uid)
                val backup = base.child("state")
                val fileref = backup.child(current_gamecode)
                val file = Uri.fromFile(File(save_filename))
                val tsk = fileref.putFile(file)
                tsk.addOnFailureListener { exception ->
                    val stateFile = File(save_filename)
                    if (stateFile.exists()) {
                        stateFile.delete()
                    }
                    emitter.onError(exception)
                }.addOnSuccessListener {
                    val stateFile = File(save_filename)
                    if (stateFile.exists()) {
                        stateFile.delete()
                    }
                    emitter.onNext("OK")
                    emitter.onComplete()
                }
            }
        }).subscribeOn(Schedulers.computation())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(saveStateObserver)
    }

    fun setLoadStateObserver(loadStateObserver: Observer<String?>) {
        Observable.create(ObservableOnSubscribe<String?> { emitter ->
            val auth = FirebaseAuth.getInstance()
            val user = auth.currentUser
            if (user == null) {
                emitter.onError(Exception("not login"))
                return@ObservableOnSubscribe
            }
            val current_gamecode = YabauseRunnable.getCurrentGameCode()
            val storage = FirebaseStorage.getInstance()
            val storage_ref = storage.reference
            val base = storage_ref.child(user.uid)
            val backup = base.child("state")
            val fileref = backup.child(current_gamecode)
            try {
                val localFile = File.createTempFile("currentstate", "bin.z")
                fileref.getFile(localFile).addOnSuccessListener(OnSuccessListener {
                    try {
                        if (localFile.exists()) {
                            YabauseRunnable.loadstate_compress(localFile.absolutePath)
                            localFile.delete()
                        }
                        emitter.onNext("OK")
                        emitter.onComplete()
                    } catch (e: Exception) {
                        emitter.onError(e)
                        return@OnSuccessListener
                    }
                }).addOnFailureListener(OnFailureListener { exception ->
                    localFile.delete()
                    emitter.onError(exception)
                    return@OnFailureListener
                })
            } catch (e: IOException) {
                emitter.onError(e)
            }
        }).subscribeOn(Schedulers.computation())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(loadStateObserver)
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        val id = item.itemId
        val bundle = Bundle()
        val title = item.title.toString()
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "MENU")
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, title)
        mFirebaseAnalytics!!.logEvent(
            FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        )
        when (id) {
            R.id.reset -> YabauseRunnable.reset()
            R.id.report -> startReport()
            R.id.gametitle -> {
                val save_path = YabauseStorage.getStorage().screenshotPath
                val current_gamecode = YabauseRunnable.getCurrentGameCode()
                val screen_shot_save_path = "$save_path$current_gamecode.png"
                if (YabauseRunnable.screenshot(screen_shot_save_path) == 0) {
                    try {
                        val gi = Select().from(GameInfo::class.java)
                            .where("product_number = ?", current_gamecode).executeSingle<GameInfo>()
                        if (gi != null) {
                            gi.image_url = screen_shot_save_path
                            gi.save()
                        }
                    } catch (e: Exception) {
                        Log.e(TAG, e.localizedMessage)
                    }
                }
            }
            R.id.save_state -> {
                val save_path = YabauseStorage.getStorage().stateSavePath
                val current_gamecode = YabauseRunnable.getCurrentGameCode()
                val save_root = File(YabauseStorage.getStorage().stateSavePath, current_gamecode)
                if (!save_root.exists()) save_root.mkdir()
                var save_filename = YabauseRunnable.savestate(save_path + current_gamecode)
                if (save_filename != "") {
                    val point = save_filename.lastIndexOf(".")
                    if (point != -1) {
                        save_filename = save_filename.substring(0, point)
                    }
                    val screen_shot_save_path = "$save_filename.png"
                    if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                        Snackbar.make(
                            mDrawerLayout!!,
                            "Failed to save the current state",
                            Snackbar.LENGTH_SHORT
                        ).show()
                    } else {
                        Snackbar.make(
                            mDrawerLayout!!,
                            "Current state is saved as $save_filename",
                            Snackbar.LENGTH_LONG
                        ).show()
                    }
                } else {
                    Snackbar.make(
                        mDrawerLayout!!,
                        "Failed to save the current state",
                        Snackbar.LENGTH_SHORT
                    ).show()
                }
                checkMaxFileCount(save_path + current_gamecode)
            }
            R.id.save_state_cloud -> {
                if (YabauseApplication.checkDonated(this) == 0) {
                    waiting_reault = true
                    val loginobserver: Observer<FirebaseUser?> = object : Observer<FirebaseUser?> {
                        override fun onSubscribe(d: Disposable) {}
                        override fun onNext(t: FirebaseUser) {
                            TODO("Not yet implemented")
                        }

                        override fun onError(e: Throwable) {
                            waiting_reault = false
                            toggleMenu()
                            Snackbar.make(
                                mDrawerLayout!!,
                                """Failed to login${e.localizedMessage}""".trimIndent(),
                                Snackbar.LENGTH_LONG
                            ).show()
                        }

                        override fun onComplete() {
                            mProgressDialog = ProgressDialog(this@Yabause)
                            mProgressDialog!!.setMessage("Sending...")
                            mProgressDialog!!.show()
                            isShowProgress = true
                            val observer: Observer<String?> = object : Observer<String?> {
                                override fun onSubscribe(d: Disposable) {}
                                override fun onError(e: Throwable) {
                                    mProgressDialog!!.dismiss()
                                    isShowProgress = false
                                    waiting_reault = false
                                    toggleMenu()
                                    Snackbar.make(
                                        mDrawerLayout!!,
                                        "Failed to save the current state to cloud",
                                        Snackbar.LENGTH_LONG
                                    ).show()
                                }

                                override fun onComplete() {
                                    mProgressDialog!!.dismiss()
                                    isShowProgress = false
                                    waiting_reault = false
                                    toggleMenu()
                                    Snackbar.make(
                                        mDrawerLayout!!,
                                        "Success to save the current state to cloud",
                                        Snackbar.LENGTH_SHORT
                                    ).show()
                                }

                                override fun onNext(t: String) {
                                    TODO("Not yet implemented")
                                }
                            }
                            setSaveStateObserver(observer)
                        }
                    }
                    checkAuth(loginobserver)
                }
            }
            R.id.load_state_cloud -> {
                if (YabauseApplication.checkDonated(this) == 0) {
                    waiting_reault = true
                    val loginobserver: Observer<FirebaseUser?> = object : Observer<FirebaseUser?> {
                        override fun onSubscribe(d: Disposable) {}
                        override fun onNext(firebaseUser: FirebaseUser) {}
                        override fun onError(e: Throwable) {
                            waiting_reault = false
                            toggleMenu()
                            Snackbar.make(
                                mDrawerLayout!!,
                                """Failed to login ${e.localizedMessage}""".trimIndent(),
                                Snackbar.LENGTH_LONG
                            ).show()
                        }

                        override fun onComplete() {
                            mProgressDialog = ProgressDialog(this@Yabause)
                            mProgressDialog!!.setMessage("Loading...")
                            mProgressDialog!!.show()
                            isShowProgress = true
                            val observer: Observer<String?> = object : Observer<String?> {
                                override fun onSubscribe(d: Disposable) {}
                                override fun onNext(response: String) {}
                                override fun onError(e: Throwable) {
                                    mProgressDialog!!.dismiss()
                                    isShowProgress = false
                                    waiting_reault = false
                                    toggleMenu()
                                    Snackbar.make(
                                        mDrawerLayout!!,
                                        """Failed to load the state from cloud ${e.localizedMessage}""".trimIndent(),
                                        Snackbar.LENGTH_SHORT
                                    ).show()
                                }

                                override fun onComplete() {
                                    mProgressDialog!!.dismiss()
                                    isShowProgress = false
                                    waiting_reault = false
                                    toggleMenu()
                                    Snackbar.make(
                                        mDrawerLayout!!,
                                        "Success to load the state from cloud",
                                        Snackbar.LENGTH_SHORT
                                    ).show()
                                }
                            }
                            setLoadStateObserver(observer)
                        }
                    }
                    checkAuth(loginobserver)
                }
            }
            R.id.load_state -> {

                // String save_path = YabauseStorage.getStorage().getStateSavePath();
                // YabauseRunnable.loadstate(save_path);
                val basepath: String
                val save_path = YabauseStorage.getStorage().stateSavePath
                val current_gamecode = YabauseRunnable.getCurrentGameCode()
                basepath = save_path + current_gamecode
                waiting_reault = true
                val transaction = supportFragmentManager.beginTransaction()
                val fragment = StateListFragment()
                fragment.setBasePath(basepath)
                transaction.replace(R.id.ext_fragment, fragment, StateListFragment.TAG)
                transaction.show(fragment)
                transaction.commit()
            }
            R.id.record -> {
                if (BuildConfig.BUILD_TYPE == "debug") {
                    YabauseRunnable.record(YabauseStorage.getStorage().recordPath)
                }
            }
            R.id.play -> {
            }
            R.id.menu_item_backup -> {
                waiting_reault = true
                val transaction = supportFragmentManager.beginTransaction()
                val fragment = TabBackupFragment.newInstance("hoge", "hoge")
                // fragment.setBasePath(basepath);
                transaction.replace(R.id.ext_fragment, fragment, TabBackupFragment.TAG)
                transaction.show(fragment)
                transaction.commit()
            }
            R.id.menu_item_pad_device -> {
                waiting_reault = true
                val newFragment = SelInputDeviceFragment()
                newFragment.setTarget(SelInputDeviceFragment.PLAYER1)
                newFragment.setListener(this)
                newFragment.show(fragmentManager, "InputDevice")
            }
            R.id.menu_item_pad_setting -> {
                waiting_reault = true
                if (padm!!.player1InputDevice == -1) { // Using pad?
                    val transaction = supportFragmentManager.beginTransaction()
                    val fragment = PadTestFragment.newInstance("hoge", "hoge")
                    fragment.setListener(this)
                    transaction.replace(R.id.ext_fragment, fragment, PadTestFragment.TAG)
                    transaction.show(fragment)
                    transaction.commit()
                } else {
                    val newFragment = InputSettingFragment()
                    newFragment.setPlayerAndFilename(SelInputDeviceFragment.PLAYER1, "keymap")
                    newFragment.setListener(this)
                    newFragment.show(fragmentManager, "InputSettings")
                }
            }
            R.id.menu_item_pad_device_p2 -> {
                waiting_reault = true
                val newFragment = SelInputDeviceFragment()
                newFragment.setTarget(SelInputDeviceFragment.PLAYER2)
                newFragment.setListener(this)
                newFragment.show(fragmentManager, "InputDevice")
            }
            R.id.menu_item_pad_setting_p2 -> {
                waiting_reault = true
                if (padm!!.player2InputDevice != -1) { // Using pad?
                    val newFragment = InputSettingFragment()
                    newFragment.setPlayerAndFilename(
                        SelInputDeviceFragment.PLAYER2,
                        "keymap_player2"
                    )
                    newFragment.setListener(this)
                    newFragment.show(fragmentManager, "InputSettings")
                }
            }
            R.id.button_open_cd -> {
                if (tray_state == 0) {
                    YabauseRunnable.openTray()
                    item.title = getString(R.string.close_cd_tray)
                    tray_state = 1
                } else {
                    item.title = getString(R.string.open_cd_tray)
                    tray_state = 0
                    val file = File(gamePath)
                    val path = file.parent
                    val fd = FileDialog(this@Yabause, path)
                    fd.addFileListener(this@Yabause)
                    fd.showDialog()
                }
            }
            R.id.pad_mode -> {
                var mode = false
                val padv = findViewById<View>(R.id.yabause_pad) as YabausePad
                val sharedPref = PreferenceManager.getDefaultSharedPreferences(this@Yabause)
                if (sharedPref.getBoolean("pref_analog_pad", false)) {
                    item.isChecked = false
                    padm!!.analogMode = PadManager.MODE_HAT
                    YabauseRunnable.switch_padmode(PadManager.MODE_HAT)
                    padv.setPadMode(PadManager.MODE_HAT)
                    mode = false
                } else {
                    item.isChecked = true
                    padm!!.analogMode = PadManager.MODE_ANALOG
                    YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG)
                    padv.setPadMode(PadManager.MODE_ANALOG)
                    mode = true
                }
                val editor = sharedPref.edit()
                editor.putBoolean("pref_analog_pad", mode)
                editor.apply()
                toggleMenu()
            }
            R.id.pad_mode_p2 -> {
                var mode = false
                val sharedPref = PreferenceManager.getDefaultSharedPreferences(this@Yabause)
                if (sharedPref.getBoolean("pref_analog_pad2", false)) {
                    item.isChecked = false
                    padm!!.analogMode2 = PadManager.MODE_HAT
                    YabauseRunnable.switch_padmode2(PadManager.MODE_HAT)
                    mode = false
                } else {
                    item.isChecked = true
                    padm!!.analogMode2 = PadManager.MODE_ANALOG
                    YabauseRunnable.switch_padmode2(PadManager.MODE_ANALOG)
                    mode = true
                }
                val editor = sharedPref.edit()
                editor.putBoolean("pref_analog_pad2", mode)
                editor.apply()
                toggleMenu()
            }
            R.id.menu_item_cheat -> {
                waiting_reault = true
                val transaction = supportFragmentManager.beginTransaction()
                val fragment =
                    TabCheatFragment.newInstance(YabauseRunnable.getCurrentGameCode(), cheat_codes)
                transaction.replace(R.id.ext_fragment, fragment, TabCheatFragment.TAG)
                transaction.show(fragment)
                transaction.commit()
            }
            R.id.exit -> {
                YabauseRunnable.deinit()
                try {
                    Thread.sleep(1000)
                } catch (e: InterruptedException) {
                }
                finish()
                killProcess(myPid())
            }
            R.id.menu_in_game_setting -> {
                waiting_reault = true
                val transaction = supportFragmentManager.beginTransaction()
                val currentGameCode = YabauseRunnable.getCurrentGameCode()
                val fragment = InGamePreference(currentGameCode)
                val observer: Observer<String?> = object : Observer<String?> {
                    // GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();
                    override fun onSubscribe(d: Disposable) {}
                    override fun onNext(response: String) {}
                    override fun onError(e: Throwable) {
                        waiting_reault = false
                        YabauseRunnable.resume()
                        audio!!.unmute(audio!!.SYSTEM)
                    }

                    override fun onComplete() {
                        // getSupportFragmentManager().popBackStack();
                        YabauseRunnable.lockGL()
                        val gamePreference = getSharedPreferences(gameCode, Context.MODE_PRIVATE)
                        YabauseRunnable.enableRotateScreen(
                            if (gamePreference.getBoolean(
                                    "pref_rotate_screen",
                                    false
                                )
                            ) 1 else 0
                        )
                        val fps = gamePreference.getBoolean("pref_fps", false)
                        YabauseRunnable.enableFPS(if (fps) 1 else 0)
                        Log.d(TAG, "enable FPS $fps")
                        val iPg = gamePreference.getString("pref_polygon_generation", "0")?.toInt()
                        YabauseRunnable.setPolygonGenerationMode(iPg!!)
                        Log.d(TAG, "setPolygonGenerationMode $iPg")
                        val frameskip = gamePreference.getBoolean("pref_frameskip", true)
                        YabauseRunnable.enableFrameskip(if (frameskip) 1 else 0)
                        Log.d(TAG, "enable enableFrameskip $frameskip")
                        val sKa: Int? =
                            gamePreference.getString("pref_polygon_generation", "0")?.toInt()
                        YabauseRunnable.setPolygonGenerationMode(sKa!!)
                        YabauseRunnable.setAspectRateMode(
                            gamePreference.getString(
                                "pref_aspect_rate",
                                "0"
                            )?.toInt()!!
                        )
                        val resolution_setting: Int? =
                            gamePreference.getString("pref_resolution", "0")?.toInt()!!
                        YabauseRunnable.setResolutionMode(resolution_setting!!)
                        YabauseRunnable.enableComputeShader(
                            if (gamePreference.getBoolean(
                                    "pref_use_compute_shader",
                                    false
                                )
                            ) 1 else 0
                        )
                        val rbg_resolution_setting: Int? =
                            gamePreference.getString("pref_rbg_resolution", "0")?.toInt()!!
                        YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting!!)
                        YabauseRunnable.unlockGL()

                        // Recreate Yabause View
                        val v = findViewById<View>(R.id.yabause_view)
                        val layout = findViewById<FrameLayout>(R.id.content_main)
                        layout.removeView(v)
                        val yv = YabauseView(this@Yabause)
                        yv.id = R.id.yabause_view
                        layout.addView(yv, 0)
                        val exitFade = Fade()
                        exitFade.duration = 500
                        fragment.exitTransition = exitFade
                        val transaction = supportFragmentManager.beginTransaction()
                        transaction.remove(fragment)
                        transaction.commitNow()
                        waiting_reault = false
                        menu_showing = false
                        val mainview = findViewById(R.id.yabause_view) as View
                        mainview.requestFocus()
                        YabauseRunnable.resume()
                        audio!!.unmute(audio!!.SYSTEM)
                    }
                }
                fragment.setonEndObserver(observer!!)
                transaction.setCustomAnimations(
                    R.anim.slide_in_up,
                    R.anim.slide_out_up,
                    R.anim.slide_in_up,
                    R.anim.slide_out_up
                )
                transaction.replace(R.id.ext_fragment, fragment, InGamePreference.TAG)
                // transaction.addToBackStack(InGamePreference.TAG);
                transaction.commit()
            }
        }
        val drawer = findViewById<View>(R.id.drawer_layout) as DrawerLayout
        drawer.closeDrawer(GravityCompat.START)
        return true
    }

    // after disc change event
    override fun fileSelected(file: File) {
        if (file != null) {
            gamePath = file.absolutePath
        }
        YabauseRunnable.closeTray()
    }

    public override fun onPause() {
        super.onPause()
        YabauseRunnable.pause()
        audio!!.mute(audio!!.SYSTEM)
        mInputManager!!.unregisterInputDeviceListener(this)
    }

    public override fun onResume() {
        super.onResume()
        if (mTracker != null) {
            mTracker!!.setScreenName(TAG)
            mTracker!!.send(ScreenViewBuilder().build())
        }
        if (waiting_reault == false) {
            audio!!.unmute(audio!!.SYSTEM)
            YabauseRunnable.resume()
        }
        mInputManager!!.registerInputDeviceListener(this, null)
    }

    public override fun onDestroy() {
        Log.v(TAG, "this is the end...")
        yabauseThread!!.destroy()
        super.onDestroy()
    }

    public override fun onCreateDialog(dialogId: Int, args: Bundle): Dialog? {
        val builder = AlertDialog.Builder(this)
        builder.setMessage(args.getString("message"))
            .setCancelable(false)
            .setNegativeButton(R.string.exit) { _, dialogId -> finish() }
            .setPositiveButton(R.string.ignore) { dialog, dialogId -> dialog.cancel() }
        return builder.create()
    }

    fun startReport() {
        waiting_reault = true
        val newFragment = ReportDialog()
        newFragment.show(fragmentManager, "Report")

        // The device is smaller, so show the fragment fullscreen
        // android.app.FragmentTransaction transaction = getFragmentManager().beginTransaction();
        // For a little polish, specify a transition animation
        // transaction.setTransition(android.app.FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
        // To make it fullscreen, use the 'content' root view as the container
        // for the fragment, which is always the root view for the activity
        // transaction.add(R.id.ext_fragment, newFragment)
        //    .addToBackStack(null).commit();

        // android.app.FragmentTransaction transaction = getFragmentManager().beginTransaction();
        // transaction.replace(R.id.ext_fragment, newFragment, StateListFragment.TAG);
        // transaction.show(newFragment);
        // transaction.commit();
    }

    fun onFinishReport() {}
    override fun onInputDeviceAdded(i: Int) {
        updateInputDevice()
    }

    override fun onInputDeviceRemoved(i: Int) {
        updateInputDevice()
    }

    override fun onInputDeviceChanged(i: Int) {}
    override fun show() {
        if (YabauseRunnable.getRecordingStatus() == YabauseRunnable.RECORDING) {
            YabauseRunnable.screenshot("")
        } else {
            toggleMenu()
        }
    }

    inner class ReportContents {
        @JvmField
        var _rating = 0

        @JvmField
        var _message: String? = null

        @JvmField
        var _screenshot = false

        @JvmField
        var _screenshot_base64: String? = null

        @JvmField
        var _screenshot_save_path: String? = null
        var _state_base64: String? = null
        var _state_save_path: String? = null
    }

    @JvmField
    var current_report: ReportContents? = null

    @JvmField
    var current_game_info: JSONObject? = null

    @JvmField
    var _report_status = REPORT_STATE_INIT
    var cheat_codes: Array<String>? = null
    fun updateCheatCode(cheat_codes: Array<String>?) {
        this.cheat_codes = cheat_codes
        if (cheat_codes == null || cheat_codes.size == 0) {
            YabauseRunnable.updateCheat(null)
        } else {
            val index = 0
            val send_codes = ArrayList<String>()
            for (i in cheat_codes.indices) {
                val tmp = cheat_codes[i].split("\n".toRegex()).dropLastWhile { it.isEmpty() }
                    .toTypedArray()
                for (j in tmp.indices) {
                    send_codes.add(tmp[j])
                }
            }
            YabauseRunnable.updateCheat(send_codes.toTypedArray())
        }
        if (waiting_reault) {
            waiting_reault = false
            menu_showing = false
            val mainview = findViewById(R.id.yabause_view) as View
            mainview.requestFocus()
            YabauseRunnable.resume()
            audio!!.unmute(audio!!.SYSTEM)
        }
    }

    fun cancelStateLoad() {
        if (waiting_reault) {
            waiting_reault = false
            menu_showing = false
            val mainview = findViewById(R.id.yabause_view) as View
            mainview.requestFocus()
            YabauseRunnable.resume()
            audio!!.unmute(audio!!.SYSTEM)
        }
    }

    fun loadState(filename: String?) {
        YabauseRunnable.loadstate(filename)
        val fg = supportFragmentManager.findFragmentByTag(StateListFragment.TAG)
        if (fg != null) {
            val transaction = supportFragmentManager.beginTransaction()
            transaction.remove(fg)
            transaction.commit()
        }
        if (waiting_reault) {
            waiting_reault = false
            menu_showing = false
            val mainview = findViewById(R.id.yabause_view) as View
            mainview.requestFocus()
            YabauseRunnable.resume()
            audio!!.unmute(audio!!.SYSTEM)
        }
    }

    @Throws(IOException::class)
    private fun createZip(zos: ZipOutputStream, files: Array<File?>) {
        val buf = ByteArray(1024)
        var `is`: InputStream? = null
        try {
            for (file in files) {
                val entry = ZipEntry(file!!.name)
                zos.putNextEntry(entry)
                `is` = BufferedInputStream(FileInputStream(file))
                var len = 0
                while (`is`.read(buf).also { len = it } != -1) {
                    zos.write(buf, 0, len)
                }
            }
        } finally {
            IOUtils.closeQuietly(`is`)
        }
    }

    var currentReportfilename: String? = null
        private set

    var currentScreenshotfilename: String? = null
        private set

    fun doReportCurrentGame(rating: Int, message: String?, screenshot: Boolean) {
        current_report = ReportContents()
        current_report!!._rating = rating
        current_report!!._message = message
        current_report!!._screenshot = screenshot
        _report_status = REPORT_STATE_INIT
        val gameinfo = YabauseRunnable.getGameinfo() ?: return
        showDialog()
        val dateFormat: DateFormat = SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss")
        val date = Date()
        val zippath = (YabauseStorage.getStorage().screenshotPath +
                YabauseRunnable.getCurrentGameCode() +
                dateFormat.format(date) + ".zip")
        val screen_shot_save_path = (YabauseStorage.getStorage().screenshotPath +
                "screenshot.png")
        if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
            dismissDialog()
            return
        }
        val save_path = YabauseStorage.getStorage().stateSavePath
        val current_gamecode = YabauseRunnable.getCurrentGameCode()
        val save_root = File(YabauseStorage.getStorage().stateSavePath, current_gamecode)
        if (!save_root.exists()) save_root.mkdir()
        val save_filename = YabauseRunnable.savestate(save_path + current_gamecode)
        val files = arrayOfNulls<File>(1)
        files[0] = File(save_filename)
        var zos: ZipOutputStream? = null
        try {
            zos = ZipOutputStream(BufferedOutputStream(FileOutputStream(File(zippath))))
            createZip(zos, files)
        } catch (e: IOException) {
            Log.d(TAG, e.localizedMessage)
            dismissDialog()
            return
        } finally {
            IOUtils.closeQuietly(zos)
        }
        files[0]!!.delete()
        currentScreenshotfilename = screen_shot_save_path
        currentReportfilename = zippath
        current_game_info = try {
            JSONObject(gameinfo)
        } catch (e: Exception) {
            Log.e(TAG, e.localizedMessage)
            dismissDialog()
            return
        }
        val asyncTask = AsyncReportv2(this)
        val url = "https://www.uoyabause.org/api/"
        // url = "http://www.uoyabause.org:3000/api/";
        asyncTask.execute(url, YabauseRunnable.getCurrentGameCode())
    }

    fun doReportCurrentGame_old(rating: Int, message: String?, screenshot: Boolean) {
        current_report = ReportContents()
        current_report!!._rating = rating
        current_report!!._message = message
        current_report!!._screenshot = screenshot
        _report_status = REPORT_STATE_INIT
        val gameinfo = YabauseRunnable.getGameinfo()
        if (gameinfo != null) {
            try {
                showDialog()
                val asyncTask = AsyncReport(this)
                current_game_info = JSONObject(gameinfo)
                val dateFormat: DateFormat = SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss")
                val date = Date()
                val screen_shot_save_path = (YabauseStorage.getStorage().screenshotPath +
                        YabauseRunnable.getCurrentGameCode() +
                        dateFormat.format(date) + ".png")
                if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                    dismissDialog()
                    return
                }
                val inputStream: InputStream =
                    FileInputStream(screen_shot_save_path) // You can get an inputStream using any IO API
                val bytes: ByteArray
                val buffer = ByteArray(8192)
                var bytesRead: Int
                val output = ByteArrayOutputStream()
                try {
                    while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                        output.write(buffer, 0, bytesRead)
                    }
                } catch (e: IOException) {
                    e.printStackTrace()
                    dismissDialog()
                    return
                }
                bytes = output.toByteArray()
                val encodedString = Base64.encodeToString(bytes, Base64.DEFAULT)
                val jsonObjimg = JSONObject()
                jsonObjimg.put("data", encodedString)
                jsonObjimg.put("filename", screen_shot_save_path)
                jsonObjimg.put("content_type", "image/png")
                val jsonObjgame = current_game_info!!.getJSONObject("game")
                jsonObjgame.put("title_image", jsonObjimg)
                if (screenshot) {
                    current_report!!._screenshot_base64 = encodedString
                    current_report!!._screenshot_save_path = screen_shot_save_path
                }

                // asyncTask.execute("http://192.168.0.7:3000/api/", YabauseRunnable.getCurrentGameCode());
                asyncTask.execute(
                    "https://www.uoyabause.org/api/",
                    YabauseRunnable.getCurrentGameCode()
                )
                return
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    fun cancelReportCurrentGame() {
        waiting_reault = false
        YabauseRunnable.resume()
        audio!!.unmute(audio!!.SYSTEM)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            0x01 -> {
                waiting_reault = false
                toggleMenu()
            }
            RC_SIGN_IN -> if (requestCode == RC_SIGN_IN) {
                val response = IdpResponse.fromResultIntent(data)
                if (resultCode == Activity.RESULT_OK) {
                    val user = FirebaseAuth.getInstance().currentUser
                    if (loginEmitter != null) {
                        loginEmitter!!.onNext(user!!)
                        loginEmitter!!.onComplete()
                    }
                } else {
                    if (loginEmitter != null) {
                        loginEmitter!!.onError(Exception(response!!.error!!.localizedMessage))
                    }
                }
            }
        }
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (menu_showing) {
            return super.onGenericMotionEvent(event)
        }
        val rtn = padm!!.onGenericMotionEvent(event)
        return if (rtn != 0) {
            true
        } else super.onGenericMotionEvent(event)
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        val action = event.action
        val keyCode = event.keyCode
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) {
                val fg_ingame =
                    supportFragmentManager.findFragmentByTag(InGamePreference.TAG) as InGamePreference?
                if (fg_ingame != null) {
                    fg_ingame.onBackPressed()
                    return true
                }
                var fg = supportFragmentManager.findFragmentByTag(StateListFragment.TAG)
                if (fg != null) {
                    // cancelStateLoad()
                    val transaction = supportFragmentManager.beginTransaction()
                    transaction.remove(fg)
                    transaction.commit()
                    val mainv = findViewById<View>(R.id.yabause_view)
                    mainv.isActivated = true
                    mainv.requestFocus()
                    waiting_reault = false
                    menu_showing = false
                    YabauseRunnable.resume()
                    audio!!.unmute(audio!!.SYSTEM)
                    return true
                }
                fg = supportFragmentManager.findFragmentByTag(TabBackupFragment.TAG)
                if (fg != null) {
                    val transaction = supportFragmentManager.beginTransaction()
                    transaction.remove(fg)
                    transaction.commit()
                    val mainv = findViewById<View>(R.id.yabause_view)
                    mainv.isActivated = true
                    mainv.requestFocus()
                    waiting_reault = false
                    menu_showing = false
                    YabauseRunnable.resume()
                    audio!!.unmute(audio!!.SYSTEM)
                    return true
                }
                val fg2 =
                    supportFragmentManager.findFragmentByTag(PadTestFragment.TAG) as PadTestFragment?
                if (fg2 != null) {
                    fg2.onBackPressed()
                    return true
                }
                toggleMenu()
            }
            return true
        }
        if (menu_showing) {
            return super.dispatchKeyEvent(event)
        }
        if (waiting_reault) {
            return super.dispatchKeyEvent(event)
        }

        // Log.d("dispatchKeyEvent","device:" + event.getDeviceId() + ",action:" + action +",keyCoe:" + keyCode );
        if (action == KeyEvent.ACTION_UP) {
            val rtn = padm!!.onKeyUp(keyCode, event)
            if (rtn != 0) {
                return true
            }
        } else if (action == KeyEvent.ACTION_MULTIPLE) {
        } else if (action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) {
            val rtn = padm!!.onKeyDown(keyCode, event)
            if (rtn != 0) {
                return true
            }
        }
        return super.dispatchKeyEvent(event)
    }

    override fun onTrimMemory(level: Int) {
        super.onTrimMemory(level)
        when (level) {
            ComponentCallbacks2.TRIM_MEMORY_RUNNING_MODERATE -> {
            }
            ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW -> {
            }
            ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL -> {
            }
            ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN -> {
            }
            ComponentCallbacks2.TRIM_MEMORY_BACKGROUND -> {
            }
            ComponentCallbacks2.TRIM_MEMORY_COMPLETE -> {
            }
        }
    }

    private var menu_showing = false
    private fun toggleMenu() {
        if (menu_showing == true) {
            menu_showing = false
            val mainview = findViewById(R.id.yabause_view) as View
            mainview.requestFocus()
            YabauseRunnable.resume()
            audio!!.unmute(audio!!.SYSTEM)
            mDrawerLayout!!.closeDrawer(GravityCompat.START)
        } else {
            menu_showing = true
            YabauseRunnable.pause()
            audio!!.mute(audio!!.SYSTEM)

            val tx = findViewById<TextView>(R.id.menu_title)
            if (tx != null) {
                val name = YabauseRunnable.getGameTitle()
                tx.text = name
            }

            if (BuildConfig.BUILD_TYPE != "pro") {
                val prefs = getSharedPreferences("private", Context.MODE_PRIVATE)
                val hasDonated = prefs.getBoolean("donated", false)
                if (hasDonated == false) {
                    if (adView != null) {
                        val lp = findViewById<LinearLayout>(R.id.navilayer)
                        if (lp != null) {
                            val mCount = lp.childCount
                            var find = false
                            for (i in 0 until mCount) {
                                val mChild = lp.getChildAt(i)
                                if (mChild === adView) {
                                    find = true
                                }
                            }
                            if (find == false) {
                                lp.addView(adView)
                            }
                            val adRequest = AdRequest.Builder()
                                .addTestDevice("303A789B146C169D4BDB5652D928FF8E").build()
                            adView!!.loadAd(adRequest)
                        }
                    }
                }
            }
            mDrawerLayout!!.openDrawer(GravityCompat.START)
        }
    }

    private var errmsg: String? = null
    fun errorMsg(msg: String) {
        errmsg = msg
        Log.d(TAG, "errorMsg $msg")
        runOnUiThread {
            val layout = findViewById<View>(R.id.drawer_layout) as DrawerLayout
            Snackbar.make(layout, errmsg!!, Snackbar.LENGTH_SHORT).show()
        }
    }

    private fun readPreferences(gamecode: String?) {
        setupInGamePreferences(this, gamecode)

        // ------------------------------------------------------------------------------------------------
        // Load per game setting
        val gamePreference = getSharedPreferences(gameCode, Context.MODE_PRIVATE)
        YabauseRunnable.enableComputeShader(
            if (gamePreference.getBoolean(
                    "pref_use_compute_shader",
                    false
                )
            ) 1 else 0
        )
        YabauseRunnable.enableRotateScreen(
            if (gamePreference.getBoolean(
                    "pref_rotate_screen",
                    false
                )
            ) 1 else 0
        )
        val fps = gamePreference.getBoolean("pref_fps", false)
        YabauseRunnable.enableFPS(if (fps) 1 else 0)
        Log.d(TAG, "enable FPS $fps")
        val iPg: Int? = gamePreference.getString("pref_polygon_generation", "0")?.toInt()
        YabauseRunnable.setPolygonGenerationMode(iPg!!)
        Log.d(TAG, "setPolygonGenerationMode $iPg")
        val frameskip = gamePreference.getBoolean("pref_frameskip", true)
        YabauseRunnable.enableFrameskip(if (frameskip) 1 else 0)
        Log.d(TAG, "enable enableFrameskip $frameskip")
        val sKa: Int? = gamePreference.getString("pref_polygon_generation", "0")?.toInt()
        YabauseRunnable.setPolygonGenerationMode(sKa!!)

        // ToDo: list
        // boolean keep_aspectrate = gamePreference.getBoolean("pref_keepaspectrate", true);
        // if(keep_aspectrate) {
        //  YabauseRunnable.setKeepAspect(1);
        // }else {
        //  YabauseRunnable.setKeepAspect(0);
        // }
        val resolution_setting: Int? = gamePreference.getString("pref_resolution", "0")?.toInt()
        YabauseRunnable.setResolutionMode(resolution_setting!!)
        val rbg_resolution_setting: Int? =
            gamePreference.getString("pref_rbg_resolution", "0")?.toInt()
        YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting!!)

        // -------------------------------------------------------------------------------------
        // Load common setting
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        val extmemory = sharedPref.getBoolean("pref_extend_internal_memory", true)
        YabauseRunnable.enableExtendedMemory(if (extmemory) 1 else 0)
        Log.d(TAG, "enable Extended Memory $extmemory")
        var icpu: Int? = sharedPref.getString("pref_cpu", "3")?.toInt()
        val abi = System.getProperty("os.arch")
        if (abi!!.contains("64")) {
            if (icpu == 2) {
                icpu = 3
            }
        }
        YabauseRunnable.setCpu(icpu!!.toInt())
        Log.d(TAG, "cpu $icpu")
        val ifilter: Int? = sharedPref.getString("pref_filter", "0")?.toInt()
        YabauseRunnable.setFilter(ifilter!!)
        Log.d(TAG, "setFilter $ifilter")
        YabauseRunnable.setAspectRateMode(0)
        val audioout = sharedPref.getBoolean("pref_audio", true)
        if (audioout) {
            audio!!.unmute(audio!!.USER)
        } else {
            audio!!.mute(audio!!.USER)
        }
        Log.d(TAG, "Audio $audioout")
        val bios = sharedPref.getString("pref_bios", "")
        if (bios!!.length > 0) {
            val storage = YabauseStorage.getStorage()
            biosPath = storage.getBiosPath(bios)
        } else biosPath = ""
        Log.d(TAG, "bios $bios")
        val cart = sharedPref.getString("pref_cart", "")
        if (cart!!.length > 0) {
            cartridgeType = cart.toInt()
        } else cartridgeType = -1
        Log.d(TAG, "cart $cart")
        val activityManager = getSystemService(ACTIVITY_SERVICE) as ActivityManager
        val configurationInfo = activityManager.deviceConfigurationInfo
        val supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000
        val video: String?
        video = if (supportsEs3) {
            sharedPref.getString("pref_video", "1")
        } else {
            sharedPref.getString("pref_video", "2")
        }
        if (video!!.length > 0) {
            videoInterface = video!!.toInt()
        } else {
            videoInterface = -1
        }
        Log.d(TAG, "video $video")
        Log.d(TAG, "getGamePath $gamePath")
        Log.d(TAG, "getMemoryPath $memoryPath")
        Log.d(TAG, "getCartridgePath $cartridgePath")
        val isound: Int? = sharedPref.getString("pref_sound_engine", "1")?.toInt()!!
        YabauseRunnable.setSoundEngine(isound!!)
        Log.d(TAG, "setSoundEngine $isound")
        val scsp_sync: Int? = sharedPref.getString("pref_scsp_sync_per_frame", "1")?.toInt()!!
        YabauseRunnable.setScspSyncPerFrame(scsp_sync!!)
        val cpu_sync: Int? = sharedPref.getString("pref_cpu_sync_per_line", "1")?.toInt()!!
        YabauseRunnable.setCpuSyncPerLine(cpu_sync!!)
        val scsp_time_sync: Int? = sharedPref.getString("scsp_time_sync_mode", "1")?.toInt()!!
        YabauseRunnable.setScspSyncTimeMode(scsp_time_sync!!)
        updateInputDevice()
    }

    fun updateInputDevice() {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        val navigationView = findViewById<View>(R.id.nav_view) as NavigationView
        val menu = navigationView.menu
        val nav_pad_device = menu.findItem(R.id.menu_item_pad_device)
        val pad = findViewById<View>(R.id.yabause_pad) as YabausePad

        // InputDevice
        var selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535")
        padm = PadManager.updatePadManager()
        padm!!.setShowMenulistener(this)
        Log.d(TAG, "input $selInputdevice")
        // First time
        if (padm != null && selInputdevice == "65535") {
            // if game pad is connected use it.
            selInputdevice = if (padm?.deviceCount!! > 0) {
                padm?.setPlayer1InputDevice(null)
                val editor = sharedPref.edit()
                editor.putString("pref_player1_inputdevice", padm?.getId(0))
                editor.commit()
                padm?.getId(0)
                // if no game pad is detected use on-screen game pad.
            } else {
                val editor = sharedPref.edit()
                editor.putString("pref_player1_inputdevice", "-1")
                editor.commit()
                "-1"
            }
        }
        if (padm!!.getDeviceCount() > 0 && selInputdevice != "-1") {
            pad.show(false)
            Log.d(TAG, "ScreenPad Disable")
            padm!!.setPlayer1InputDevice(selInputdevice)
            for (inputType in 0 until padm!!.getDeviceCount()) {
                if (padm!!.getId(inputType) == selInputdevice) {
                    nav_pad_device.title = padm!!.getName(inputType)
                }
            }
            // Enable Swipe
            mDrawerLayout!!.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED)
        } else {
            pad.show(true)
            Log.d(TAG, "ScreenPad Enable")
            padm!!.setPlayer1InputDevice(null)

            // Set Menu item
            nav_pad_device.title = getString(R.string.onscreen_pad)

            // Disable Swipe
            mDrawerLayout!!.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED)
        }
        val selInputdevice2 = sharedPref.getString("pref_player2_inputdevice", "65535")
        val nav_pad_device_p2 = menu.findItem(R.id.menu_item_pad_device_p2)
        padm!!.setPlayer2InputDevice(null)
        nav_pad_device_p2.title = "Disconnected"
        menu.findItem(R.id.pad_mode_p2).isVisible = false
        menu.findItem(R.id.menu_item_pad_setting_p2).isVisible = false
        if (selInputdevice != "65535" && selInputdevice != "-1") {
            for (inputType in 0 until padm!!.getDeviceCount()) {
                if (padm!!.getId(inputType) == selInputdevice2) {
                    padm!!.setPlayer2InputDevice(selInputdevice2)
                    nav_pad_device_p2.title = padm!!.getName(inputType)
                    menu.findItem(R.id.pad_mode_p2).isVisible = true
                    menu.findItem(R.id.menu_item_pad_setting_p2).isVisible = true
                }
            }
        }
        var analog = sharedPref.getBoolean("pref_analog_pad", false)
        val padv = findViewById<View>(R.id.yabause_pad) as YabausePad
        if (analog) {
            padm!!.setAnalogMode(PadManager.MODE_ANALOG)
            YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG)
            padv.setPadMode(PadManager.MODE_ANALOG)
        } else {
            padm!!.setAnalogMode(PadManager.MODE_HAT)
            YabauseRunnable.switch_padmode(PadManager.MODE_HAT)
            padv.setPadMode(PadManager.MODE_HAT)
        }
        menu.findItem(R.id.pad_mode).isChecked = analog
        analog = sharedPref.getBoolean("pref_analog_pad2", false)
        if (analog) {
            padm!!.setAnalogMode2(PadManager.MODE_ANALOG)
            YabauseRunnable.switch_padmode2(PadManager.MODE_ANALOG)
        } else {
            padm!!.setAnalogMode2(PadManager.MODE_HAT)
            YabauseRunnable.switch_padmode2(PadManager.MODE_HAT)
        }
        menu.findItem(R.id.pad_mode_p2).isChecked = analog
        val scsp_time_sync: Int? = sharedPref.getString("scsp_time_sync_mode", "1")?.toInt()
        YabauseRunnable.setScspSyncTimeMode(scsp_time_sync!!)
    }

    val testPath: String?
        get() = if (testCase == null) {
            null
        } else YabauseStorage.getStorage().recordPath + testCase

    val memoryPath: String
        get() = YabauseStorage.getStorage().getMemoryPath("memory.ram")

    val player2InputDevice: Int
        get() = padm!!.player2InputDevice

    val cartridgePath: String
        get() = YabauseStorage.getStorage()
            .getCartridgePath(Cartridge.getDefaultFilename(cartridgeType))

    companion object {
        private const val TAG = "Yabause"
        const val REPORT_STATE_INIT = 0
        const val REPORT_STATE_SUCCESS = 1
        const val REPORT_STATE_FAIL_DUPE = -1
        const val REPORT_STATE_FAIL_CONNECTION = -2
        const val REPORT_STATE_FAIL_AUTH = -3

        init {
            System.loadLibrary("yabause_native")
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus && supportFragmentManager.findFragmentById(R.id.ext_fragment) == null) {
            updateViewLayout(resources.configuration.orientation)
        }
    }

    override fun onDeviceUpdated(target: Int) {}
    override fun onSelected(target: Int, name: String, id: String) {
        val pad = findViewById<View>(R.id.yabause_pad) as YabausePad
        val navigationView = findViewById<View>(R.id.nav_view) as NavigationView
        val menu = navigationView.menu
        val nav_pad_device = menu.findItem(R.id.menu_item_pad_device)
        val nav_pad_device_p2 = menu.findItem(R.id.menu_item_pad_device_p2)
        padm = PadManager.updatePadManager()
        padm!!.setShowMenulistener(this)
        if (padm!!.getDeviceCount() > 0 && id != "-1") {
            when (target) {
                SelInputDeviceFragment.PLAYER1 -> {
                    Log.d(TAG, "ScreenPad Disable")
                    pad.show(false)
                    padm!!.setPlayer1InputDevice(id)
                    nav_pad_device.title = name
                }
                SelInputDeviceFragment.PLAYER2 -> {
                    padm!!.setPlayer2InputDevice(id)
                    nav_pad_device_p2.title = name
                }
            }
            mDrawerLayout!!.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED)
        } else {
            if (target == SelInputDeviceFragment.PLAYER1) {
                pad.updateScale()
                pad.show(true)
                nav_pad_device.title = getString(R.string.onscreen_pad)
                Log.d(TAG, "ScreenPad Enable")
                padm!!.setPlayer1InputDevice(null)
                mDrawerLayout!!.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED)
            } else if (target == SelInputDeviceFragment.PLAYER2) {
                padm!!.setPlayer2InputDevice(null)
                nav_pad_device_p2.title = "Disconnected"
            }
        }
        updateInputDevice()
        waiting_reault = false
        toggleMenu()
    }

    override fun onCancel(target: Int) {
        waiting_reault = false
        toggleMenu()
    }

    override fun onBackPressed() {
        val fg = supportFragmentManager.findFragmentByTag(PadTestFragment.TAG) as PadTestFragment?
        fg?.onBackPressed()
        val fg2 =
            supportFragmentManager.findFragmentByTag(InGamePreference.TAG) as InGamePreference?
        fg2?.onBackPressed()
    }

    override fun onFinish() {
        val fg = supportFragmentManager.findFragmentByTag(PadTestFragment.TAG) as PadTestFragment?
        if (fg != null) {
            val transaction = supportFragmentManager.beginTransaction()
            transaction.remove(fg)
            transaction.commit()
            val padv = findViewById<View>(R.id.yabause_pad) as YabausePad
            padv.updateScale()
        }
        waiting_reault = false
        toggleMenu()
    }

    override fun onCancel() {
        val fg = supportFragmentManager.findFragmentByTag(PadTestFragment.TAG) as PadTestFragment?
        if (fg != null) {
            val transaction = supportFragmentManager.beginTransaction()
            transaction.remove(fg)
            transaction.commit()
        }
        waiting_reault = false
        toggleMenu()
    }

    override fun onFinishInputSetting() {
        updateInputDevice()
        waiting_reault = false
        toggleMenu()
    }
}
