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

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.util.Log
import android.view.View
import android.widget.CheckBox
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.view.ContextThemeWrapper
import androidx.core.app.ActivityCompat
import androidx.fragment.app.Fragment
import androidx.preference.PreferenceManager
import com.firebase.ui.auth.AuthUI
import com.firebase.ui.auth.AuthUI.IdpConfig.GoogleBuilder
import com.firebase.ui.auth.ErrorCodes
import com.firebase.ui.auth.IdpResponse
import com.google.firebase.analytics.FirebaseAnalytics
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.crashlytics.FirebaseCrashlytics
import com.google.firebase.database.FirebaseDatabase
import io.reactivex.Observable
import io.reactivex.Observer
import io.reactivex.Single
import io.reactivex.SingleEmitter
import io.reactivex.SingleOnSubscribe
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.observers.DisposableSingleObserver
import io.reactivex.schedulers.Schedulers
import java.io.File
import java.util.Arrays
import java.util.Calendar
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.YabauseStorage.Companion.storage

class GameSelectPresenter(target: Fragment, listener: GameSelectPresenterListener) {
    private val mFirebaseAnalytics: FirebaseAnalytics
    private val TAG = "GameSelectPresenter"

    interface GameSelectPresenterListener {
        // void onUpdateGameList( );
        fun onShowMessage(string_id: Int)
    }

    fun updateGameDatabaseRx(observer: Observer<String>?) {
        if (observer != null) {
            Observable.create<String?> { emitter ->
                val ybs = storage
                ybs.setProcessEmmiter(emitter)
                ybs.generateGameDB(refresh_level_)
                ybs.setProcessEmmiter(null)
                refresh_level_ = 0
                emitter.onComplete()
            }.observeOn(AndroidSchedulers.mainThread())
                .subscribeOn(Schedulers.io())
                .subscribe(observer)
        }
    }

    var refresh_level_ = 0
    val target_: Fragment
    val listener_: GameSelectPresenterListener
    private var username_: String? = null
    private var photo_url_: Uri? = null
    fun updateGameList(refresh_level: Int, observer: Observer<String>?) {
        refresh_level_ = refresh_level
        val activity = target_.activity ?: return
        if (Build.VERSION.SDK_INT >= 23) {
            // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(activity,
                    Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(activity,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED
            ) {
                return
            }
        }
        val external_AND_removable_storage_m1 = activity.getExternalFilesDirs(null)
        if (external_AND_removable_storage_m1.size > 1 && external_AND_removable_storage_m1[1] != null) {
            val path = external_AND_removable_storage_m1[1].toString()
            val ys = storage
            ys.setExternalStoragePath(path /*external_path*/)
        } else {
            val sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(activity)
            val editor = sharedPrefwrite.edit()
            editor.putString("pref_game_download_directory", "0")
            editor.apply()
        }
        updateGameDatabaseRx(observer)
    }

    fun signIn() {
        target_.startActivityForResult(
            AuthUI.getInstance()
                .createSignInIntentBuilder()
                .setAvailableProviders(Arrays.asList( /*new AuthUI.IdpConfig.Builder(AuthUI.TWITTER_PROVIDER).build(),
                                            new AuthUI.IdpConfig.Builder(AuthUI.FACEBOOK_PROVIDER).build(),*/
                    GoogleBuilder().build()))
                .build(),
            RC_SIGN_IN)
    }

    var auth_emitter_: SingleEmitter<FirebaseUser?>? = null
    fun signIn(singleObserver: DisposableSingleObserver<FirebaseUser>?) {
        Single.create(SingleOnSubscribe<FirebaseUser?> { emitter ->
            auth_emitter_ = null
            val auth = FirebaseAuth.getInstance()
            val user = auth.currentUser
            if (user != null) {
                emitter.onSuccess(user)
                return@SingleOnSubscribe
            }
            auth_emitter_ = emitter
            target_.startActivityForResult(
                AuthUI.getInstance()
                    .createSignInIntentBuilder()
                    .setAvailableProviders(Arrays.asList( /*new AuthUI.IdpConfig.Builder(AuthUI.TWITTER_PROVIDER).build(),
                                            new AuthUI.IdpConfig.Builder(AuthUI.FACEBOOK_PROVIDER).build(),*/
                        GoogleBuilder().build()))
                    .build(),
                RC_SIGN_IN)
        })
            .subscribeOn(AndroidSchedulers.mainThread())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(singleObserver!!)
    }

    fun signOut() {
        AuthUI.getInstance()
            .signOut(target_.requireActivity())
            .addOnCompleteListener {
                // user is now signed out
                // startActivity(new Intent(MyActivity.this, SignInActivity.class));
                // finish();
                // listener_.onUpdateGameList();
            }
    }

    fun onSignIn(resultCode: Int, data: Intent?) {
        val response = IdpResponse.fromResultIntent(data)
        // Successfully signed in
        if (resultCode == Activity.RESULT_OK) {
            response!!.idpToken
            val token = response.idpToken
            // listener_.onUpdateGameList();
            val auth = FirebaseAuth.getInstance()

            val currentUser = auth.currentUser
            if (currentUser == null) {
                Log.d(TAG, "Fail to get currentUser even if Auth is successed!")
                return
            }

            val baseref = FirebaseDatabase.getInstance().reference
            val baseurl = "/user-posts/" + currentUser.uid
            if (currentUser.displayName != null) {
                username_ = currentUser.displayName
                baseref.child(baseurl).child("name").setValue(currentUser.displayName)
            }
            if (currentUser.email != null) {
                baseref.child(baseurl).child("email").setValue(currentUser.email)
            }
            // if (currentUser.photoUrl != null) {
            //    baseref.child(baseurl).child("photo").setValue(auth.getCurrentUser().getPhotoUrl());
            //    photo_url_ = auth.getCurrentUser().getPhotoUrl();
            // }
            baseref.child(baseurl).child("android_token").setValue(token)
            FirebaseCrashlytics.getInstance().setUserId(username_ + "_" + currentUser.email)
            mFirebaseAnalytics.setUserId(username_ + "_" + currentUser.email)
            mFirebaseAnalytics.setUserProperty("name", username_ + "_" + currentUser.email)
            val activity: Activity? = target_.activity
            val prefs = activity!!.getSharedPreferences("private", Context.MODE_PRIVATE)
            var hasDonated = false
            if (prefs != null) {
                hasDonated = prefs.getBoolean("donated", false)
            }
            if (BuildConfig.BUILD_TYPE == "pro" || hasDonated) {
                baseref.child(baseurl).child("max_backup_count").setValue(256)
            } else {
                baseref.child(baseurl).child("max_backup_count").setValue(3)
            }

            // startActivity(SignedInActivity.createIntent(this, response));
            // val application = target_.activity!!.application as YabauseApplication
            FirebaseCrashlytics.getInstance().setUserId(currentUser.displayName + "_" + currentUser.email)

            if (auth_emitter_ != null) {
                auth_emitter_!!.onSuccess(currentUser)
                auth_emitter_ = null
            }
            return
        } else {
            if (auth_emitter_ != null) {
                auth_emitter_!!.onError(Throwable("Sigin in failed"))
                auth_emitter_ = null
            }
            username_ = null
            photo_url_ = null

            // Sign in failed
            if (response == null) {
                // User pressed back button
                listener_.onShowMessage(R.string.sign_in_cancelled)
                return
            }
            if (response.error!!.errorCode == ErrorCodes.NO_NETWORK) {
                listener_.onShowMessage(R.string.no_internet_connection)
                return
            }
            if (response.error!!.errorCode == ErrorCodes.UNKNOWN_ERROR) {
                listener_.onShowMessage(R.string.unknown_error)
                return
            }
        }
        if (auth_emitter_ != null) {
            auth_emitter_!!.onError(Throwable("Sigin in failed"))
            auth_emitter_ = null
        }
        listener_.onShowMessage(R.string.unknown_sign_in_response)
    }

    val currentUserName: String?
        get() {
            val auth = FirebaseAuth.getInstance()
            return if (auth.currentUser != null) {
                auth.currentUser!!.displayName
            } else null
        }
    val currentUserPhoto: Uri?
        get() {
            val auth = FirebaseAuth.getInstance()
            return if (auth.currentUser != null) {
                auth.currentUser!!.photoUrl
            } else null
        }

    fun checkSignIn() {
        if (target_.activity == null) {
            return // Activity has benn detached.
        }
        val check_prefernce = PreferenceManager.getDefaultSharedPreferences(
            target_.activity)
        val do_not_ask = check_prefernce.getBoolean("pref_dont_ask_signin", false)
        if (do_not_ask == true) {
            val auth = FirebaseAuth.getInstance()
            if (auth.currentUser != null) {
                FirebaseCrashlytics.getInstance().setUserId(auth.currentUser!!
                    .displayName + "_" + auth.currentUser!!.email)
                mFirebaseAnalytics.setUserId(auth.currentUser!!
                    .displayName + "_" + auth.currentUser!!.email)
                mFirebaseAnalytics.setUserProperty("name", auth.currentUser!!
                    .displayName + "_" + auth.currentUser!!.email)
            }
            return
        }
        val view = target_.requireActivity()
            .layoutInflater.inflate(R.layout.signin, null)
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            val builder = AlertDialog.Builder(ContextThemeWrapper(
                target_.activity, R.style.Theme_AppCompat))
            builder.setTitle(R.string.do_you_want_to_sign_in)
                .setCancelable(false)
                .setView(view)
                .setPositiveButton(target_.resources.getString(R.string.accept)) { dialog, _ ->
                    val cb = view.findViewById<View>(R.id.checkBox_never_ask) as CheckBox
                    val sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(target_.activity)
                    val editor = sharedPrefwrite.edit()
                    editor.putBoolean("pref_dont_ask_signin", cb.isChecked)
                    editor.apply()
                    dialog.dismiss()
                    target_.startActivityForResult(
                        AuthUI.getInstance()
                            .createSignInIntentBuilder()
                            .setTheme(R.style.Theme_AppCompat)
                            .setTosAndPrivacyPolicyUrls(
                                "https://www.uoyabause.org/static_pages/eula.html",
                                "https://www.uoyabause.org/static_pages/privacy_policy")
                            .setAvailableProviders(Arrays.asList(GoogleBuilder().build()))
                            .build(),
                        RC_SIGN_IN)
                }
                .setNegativeButton(target_.resources.getString(R.string.decline)) { dialog, _ ->
                    val cb = view.findViewById<CheckBox>(R.id.checkBox_never_ask)
                    if (cb != null) {
                        val sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(
                            target_.activity)
                        val editor = sharedPrefwrite.edit()
                        editor.putBoolean("pref_dont_ask_signin", cb.isChecked)
                        editor.apply()
                    }
                    dialog.cancel()
                }
            builder.create().show()
        } else {
            FirebaseCrashlytics.getInstance().setUserId(auth.currentUser!!
                .displayName + "_" + auth.currentUser!!.email)
            mFirebaseAnalytics.setUserId(auth.currentUser!!
                .displayName + "_" + auth.currentUser!!.email)
            mFirebaseAnalytics.setUserProperty("name", auth.currentUser!!
                .displayName + "_" + auth.currentUser!!.email)
        }
    }

    fun fileSelected(file: File?) {
        val apath: String
        if (file == null) { // canceled
            return
        }
        val context = target_.activity ?: return
        apath = file.absolutePath

        // save last selected dir
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        val editor = sharedPref.edit()
        editor.putString("pref_last_dir", file.parent)
        editor.apply()
        var gameinfo = GameInfo.getFromFileName(apath)
        if (gameinfo == null) {
            gameinfo = if (apath.endsWith("CUE") || apath.endsWith("cue")) {
                GameInfo.genGameInfoFromCUE(apath)
            } else if (apath.endsWith("MDS") || apath.endsWith("mds")) {
                GameInfo.genGameInfoFromMDS(apath)
            } else if (apath.endsWith("CCD") || apath.endsWith("ccd")) {
                GameInfo.genGameInfoFromMDS(apath)
            } else {
                GameInfo.genGameInfoFromIso(apath)
            }
        }
        if (gameinfo != null) {
            gameinfo.updateState()
            val c = Calendar.getInstance()
            gameinfo.lastplay_date = c.time
            gameinfo.save()
        } else {
            return
        }

        /*
        Bundle bundle = new Bundle();
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title);
        mFirebaseAnalytics.logEvent(
                FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        );
        */
        val intent = Intent(context, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", apath)
        context.startActivity(intent)
    }

    companion object {
        const val RC_SIGN_IN = 123
    }

    init {
        target_ = target
        listener_ = listener
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(target_.requireActivity())
    }
}
