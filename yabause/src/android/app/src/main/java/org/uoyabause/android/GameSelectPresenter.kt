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
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.database.Cursor
import android.media.MediaDrm
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.FileUtils
import android.os.ParcelFileDescriptor
import android.provider.OpenableColumns
import android.util.Log
import android.view.View
import android.widget.CheckBox
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.fragment.app.Fragment
import androidx.multidex.MultiDexApplication
import androidx.preference.PreferenceManager
import com.firebase.ui.auth.AuthUI
import com.firebase.ui.auth.AuthUI.IdpConfig.GoogleBuilder
import com.firebase.ui.auth.ErrorCodes
import com.firebase.ui.auth.IdpResponse
import com.google.android.gms.analytics.HitBuilders
import com.google.android.gms.analytics.Tracker
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInClient
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import com.google.android.gms.common.api.ApiException
import com.google.firebase.analytics.FirebaseAnalytics
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.crashlytics.FirebaseCrashlytics
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.storage.FirebaseStorage
import com.google.firebase.storage.StorageException
import io.reactivex.Observable
import io.reactivex.Observer
import io.reactivex.Single
import io.reactivex.SingleEmitter
import io.reactivex.SingleOnSubscribe
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.observers.DisposableSingleObserver
import io.reactivex.schedulers.Schedulers
import java.io.*
import java.nio.channels.FileChannel
import java.util.*
import java.util.zip.ZipFile
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.apache.commons.compress.archivers.sevenz.SevenZFile
import org.devmiyax.yabasanshiro.BuildConfig
import org.uoyabause.android.YabauseStorage.Companion.storage
import java.security.MessageDigest
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream
import androidx.appcompat.view.ContextThemeWrapper as ContextThemeWrapper1
import org.devmiyax.yabasanshiro.R
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.util.zip.ZipInputStream

class GameSelectPresenter(
    target: Fragment,
    private val yabauseActivityLauncher: ActivityResultLauncher<Intent>,
    listener: GameSelectPresenterListener) {
    private val mFirebaseAnalytics: FirebaseAnalytics
    private var mGoogleSignInClient: GoogleSignInClient? = null
    private val TAG = "GameSelectPresenter"
    private var tracker: Tracker? = null
    private val scope = CoroutineScope(Dispatchers.IO)

    enum class BackupSyncState {
        IDLE,
        CHECKING_DOWNLOAD,
        CHECKING_UPLOAD
    }

    private var syncState = BackupSyncState.IDLE


    private var gameSignInActivityLauncher = target.registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            val task = GoogleSignIn.getSignedInAccountFromIntent(result.data)
            try {
                task.getResult(ApiException::class.java)
            } catch (apiException: ApiException) {
                var message = apiException.message
                if (message == null || message.isEmpty()) {
                    message = "Fail to login" // target_.getString(R.string.signin_other_error)
                }
                AlertDialog.Builder(target_.requireActivity())
                    .setMessage(message)
                    .setNeutralButton(android.R.string.ok, null)
                    .show()
            }
        }
    }


    interface GameSelectPresenterListener {
        // void onUpdateGameList( );
        fun onShowMessage(string_id: Int)
        fun onShowDialog(message: String)
        fun onUpdateDialogMessage(message: String)
        fun onDismissDialog()
        fun onLoadRows()
    }

    private fun updateGameDatabaseRx(observer: Observer<String>?) {
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
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
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

    fun signIn( launcher : ActivityResultLauncher<Intent> ) {
        val intent = AuthUI.getInstance()
            .createSignInIntentBuilder()
            .setAvailableProviders(Arrays.asList(GoogleBuilder().build()))
            .build()
        launcher.launch(intent)
    }

    var authEmitter: SingleEmitter<FirebaseUser?>? = null
    fun signIn(singleObserver: DisposableSingleObserver<FirebaseUser>?) {
        Single.create(SingleOnSubscribe<FirebaseUser?> { emitter ->
            authEmitter = null
            val auth = FirebaseAuth.getInstance()
            val user = auth.currentUser
            if (user != null) {
                emitter.onSuccess(user)
                return@SingleOnSubscribe
            }
            authEmitter = emitter

            target_.startActivity(
                AuthUI.getInstance()
                    .createSignInIntentBuilder()
                    .setAvailableProviders(Arrays.asList(GoogleBuilder().build()))
                    .build())
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

        if (GoogleSignIn.getLastSignedInAccount(target_.requireActivity()) != null) {
            mGoogleSignInClient?.signOut()?.addOnCompleteListener(target_.requireActivity()
            ) { task ->
                val successful = task.isSuccessful
                Log.d(TAG,
                    "signOut(): " + if (successful) "success" else "failed")
            }
        }
    }

    fun onSignIn(resultCode: Int, data: Intent?) {
        val response = IdpResponse.fromResultIntent(data)
        // Successfully signed in
        if (resultCode == Activity.RESULT_OK) {

            val bundle = Bundle()
            mFirebaseAnalytics.logEvent(FirebaseAnalytics.Event.LOGIN, bundle)

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

            if (authEmitter != null) {
                authEmitter!!.onSuccess(currentUser)
                authEmitter = null
            }

            mGoogleSignInClient = GoogleSignIn.getClient(target_.requireActivity(),
                GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN).build())
            if (mGoogleSignInClient != null) {
                gameSignInActivityLauncher.launch(mGoogleSignInClient!!.getSignInIntent())
            }

            return
        } else {
            if (authEmitter != null) {
                authEmitter!!.onError(Throwable("Sigin in failed"))
                authEmitter = null
            }
            username_ = null
            photo_url_ = null

            // Sign in failed
            if (response == null) {
                // User pressed back button
                listener_.onShowMessage(org.devmiyax.yabasanshiro.R.string.sign_in_cancelled)
                return
            }
/*
            if (response.error!!.errorCode == MediaDrm.ErrorCodes.NO_NETWORK) {
                listener_.onShowMessage(org.devmiyax.yabasanshiro.R.string.no_internet_connection)
                return
            }
            if (response.error!!.errorCode == MediaDrm.ErrorCodes.UNKNOWN_ERROR) {
                listener_.onShowMessage(org.devmiyax.yabasanshiro.R.string.unknown_error)
                return
            }

 */
        }
        if (authEmitter != null) {
            authEmitter!!.onError(Throwable("Sigin in failed"))
            authEmitter = null
        }
        listener_.onShowMessage(org.devmiyax.yabasanshiro.R.string.unknown_sign_in_response)
    }

    fun onSelectFile(uri: Uri) {
        Log.i(TAG, "Uri: $uri")

        val cursor: Cursor? = target_.requireActivity().contentResolver.query(uri,
            null, null, null, null)

        cursor!!.moveToFirst()
        val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
        val path = cursor.getString(nameIndex)
        if (path.lowercase(Locale.ROOT).endsWith("chd")) {
            val index = cursor.getColumnIndex(OpenableColumns.SIZE)

            var size: Long = cursor.getLong(index)
            cursor.close()

            size = size / 1024 / 1024

            val prefs: SharedPreferences = target_.requireActivity().getSharedPreferences("private",
                AppCompatActivity.MODE_PRIVATE)
            val count = prefs.getInt("InstallCount", 3)

            var message =
                target_.getString(R.string.install_game_message) + " " + size + target_.getString(R.string.install_game_message_after)

            if (BuildConfig.BUILD_TYPE != "pro") {
                message += target_.getString(R.string.remaining_installation_count_is) + " " + count + "."
            }

            AlertDialog.Builder(
                ContextThemeWrapper1(
                target_.activity, R.style.Theme_AppCompat)
            )
                .setTitle(target_.getString(R.string.do_you_want_to_install))
                .setMessage(message)
                .setPositiveButton(R.string.yes) { _, _ ->
                    selectStorage {
                        installGameFile(uri)
                    }
                }
                .setNegativeButton(R.string.no) { _, _ ->
                    decrementInstallCount()
                    openGameFileDirect(uri)
                }
                .setCancelable(true)
                .show()
        } else if (path.lowercase(Locale.getDefault()).endsWith("zip") || path.lowercase(Locale.getDefault())
                .endsWith("7z")) {

            selectStorage {
                installZipGameFile(uri, path)
            }
        } else {
            Toast.makeText(target_.requireContext(),
                target_.getString(R.string.only_chd_is_supported_for_load_game),
                Toast.LENGTH_LONG).show()
        }
        return
    }

    fun selectStorage(onOk: () -> Unit) {
        if (storage.hasExternalSD()) {
            val ctx = YabauseApplication.appContext
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx)
            val path = sharedPref.getString("pref_install_location", "0")
            var selectItem = path?.toInt() ?: 0

            var option: Array<String> = arrayOf()
            option += "Internal" + " (" + storage.getAvailableInternalMemorySize() + " free)"
            option += "External" + " (" + storage.getAvailableExternalMemorySize() + " free)"

            AlertDialog.Builder(target_.requireActivity())
                .setTitle(target_.getString(R.string.which_storage))
                .setSingleChoiceItems(option, selectItem
                ) { _, which ->
                    selectItem = which
                }
                .setPositiveButton(R.string.ok) { _, _ ->

                    val editor = sharedPref.edit()
                    editor.putString("pref_install_location", selectItem.toString())
                    editor.apply()
                    onOk()
                }
                .setNegativeButton(R.string.cancel) { _, _ -> }
                .setCancelable(true)
                .show()
        } else {
            onOk()
        }
    }

    @Suppress("BlockingMethodInNonBlockingContext")
    private fun openGameFileDirect(uri: Uri ) {
        scope.launch {
            withContext(Dispatchers.Main) {
                listener_.onShowDialog("Opening ...")
            }

            var parcelFileDescriptor: ParcelFileDescriptor? = null
            val uriString = uri.toString().lowercase(Locale.ROOT)
            var apath = ""
            try {
                parcelFileDescriptor =
                    target_.requireActivity().contentResolver.openFileDescriptor(uri, "r")
                if (parcelFileDescriptor != null) {
                    val fd: Int = parcelFileDescriptor.fd
                    apath = "/proc/self/fd/$fd"
                }
            } catch (fne: FileNotFoundException) {
                apath = ""
            }
            if (apath == "") {
                Toast.makeText(target_.requireContext(), "Fail to open $uriString", Toast.LENGTH_LONG)
                    .show()
                parcelFileDescriptor?.close()
                withContext(Dispatchers.Main) {
                    listener_.onDismissDialog()
                }
                return@launch
            }

            val gameinfo = GameInfo.genGameInfoFromCHD(apath)
            if (gameinfo != null) {

                val bundle = Bundle()
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                mFirebaseAnalytics.logEvent(
                    "yab_start_game", bundle
                )
                parcelFileDescriptor!!.close()
                val intent = Intent(target_.requireActivity(), Yabause::class.java)
                intent.putExtra("org.uoyabause.android.FileNameUri", uri.toString())
                intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
                yabauseActivityLauncher.launch(intent)

            } else {
                Toast.makeText(target_.requireContext(), "Fail to open $apath", Toast.LENGTH_LONG).show()
                parcelFileDescriptor?.close()
            }
            withContext(Dispatchers.Main) {
                listener_.onDismissDialog()
            }
        }
    }

    fun decrementInstallCount() {
        val prefs = target_.requireActivity().getSharedPreferences("private",
            MultiDexApplication.MODE_PRIVATE)
        var InstallCount = prefs.getInt("InstallCount", 3)
        InstallCount -= 1
        if (InstallCount < 0) {
            InstallCount = 0
        }
        with(prefs.edit()) {
            putInt("InstallCount", InstallCount)
            apply()
        }
    }

    @Throws(IOException::class)
    fun copyFileO(sourceFile: FileInputStream, destFile: FileOutputStream) {
        var source: FileChannel? = null
        var destination: FileChannel? = null
        try {
            source = sourceFile.channel
            destination = destFile.channel
            destination.transferFrom(source, 0, source.size())
        } finally {
            source?.close()
            destination?.close()
        }
    }

    fun installZipGameFile(uri: Uri, path: String) {
        scope.launch {
            withContext(Dispatchers.Main) {
                listener_.onShowDialog("Installing ...")
            }

            var zipFileName = ""
            try {

                val f = File(path)
                zipFileName = storage.getInstallDir().absolutePath + "/" + f.name
                val fd = File(zipFileName)
                val parcelFileDescriptor =
                    target_.requireActivity().contentResolver.openFileDescriptor(uri, "r")
                val fileDescriptor: FileDescriptor = parcelFileDescriptor!!.fileDescriptor
                FileInputStream(fileDescriptor).use { inputStream ->
                    FileOutputStream(fd).use { outputStream ->
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                            FileUtils.copy(inputStream, outputStream)
                        } else {
                            copyFileO(inputStream, outputStream)
                        }
                        inputStream.close()
                        outputStream.close()
                    }
                }
                parcelFileDescriptor.close()


                var targetFileName = ""

                withContext(Dispatchers.Main) {
                    listener_.onUpdateDialogMessage("Extracting ${fd.name}")
                }

                if (zipFileName.lowercase(Locale.getDefault()).endsWith("zip")) {

                    ZipFile(zipFileName).use { zip ->
                        zip.entries().asSequence().forEach { entry ->

                            if (entry.name.lowercase(Locale.ROOT).endsWith("ccd") ||
                                entry.name.lowercase(Locale.ROOT).endsWith("cue") ||
                                entry.name.lowercase(Locale.ROOT).endsWith("mds")
                            ) {
                                targetFileName = storage.getInstallDir().absolutePath + "/" + entry.name
                            }
                            zip.getInputStream(entry).use { input ->
                                if (entry.isDirectory) {
                                    val unzipdir =
                                        File(storage.getInstallDir().absolutePath + "/" + entry.name)
                                    if (!unzipdir.exists()) {
                                        unzipdir.mkdirs()
                                    } else {
                                        unzipdir.delete()
                                        unzipdir.mkdirs()
                                    }
                                } else {
                                    File(storage.getInstallDir().absolutePath + "/" + entry.name).outputStream()
                                        .use { output ->
                                            input.copyTo(output)
                                        }
                                }
                            }
                        }
                    }
                } else if (zipFileName.lowercase(Locale.getDefault()).endsWith("7z")) {
                    SevenZFile(File(zipFileName)).use { sz ->
                        sz.entries.asSequence().forEach { entry ->
                            if (entry.name.lowercase(Locale.ROOT).endsWith("ccd") ||
                                entry.name.lowercase(Locale.ROOT).endsWith("cue") ||
                                entry.name.lowercase(Locale.ROOT).endsWith("mds")
                            ) {
                                targetFileName = storage.getInstallDir().absolutePath + "/" + entry.name
                            }

                            if (entry.isDirectory) {
                                val unzipdir =
                                    File(storage.getInstallDir().absolutePath + "/" + entry.name)
                                if (!unzipdir.exists()) {
                                    unzipdir.mkdirs()
                                } else {
                                    unzipdir.delete()
                                    unzipdir.mkdirs()
                                }
                            } else {
                                sz.getInputStream(entry).use { input ->
                                    File(storage.getInstallDir().absolutePath + "/" + entry.name).outputStream()
                                        .use { output ->
                                            input.copyTo(output)
                                        }
                                }
                            }
                        }
                    }
                }

                if (targetFileName != "") {
                    withContext(Dispatchers.Main) {
                        decrementInstallCount()
                        fileSelected(File(targetFileName))
                    }
                } else {

                    withContext(Dispatchers.Main) {
                        Toast.makeText(target_.requireContext(),
                            "ISO image is not found!!",
                            Toast.LENGTH_LONG).show()
                    }
                }
            } catch (e: Exception) {

                withContext(Dispatchers.Main) {
                    Toast.makeText(target_.requireContext(),
                        "Fail to copy " + e.localizedMessage,
                        Toast.LENGTH_LONG).show()
                }
            } catch (e: IOException) {

                withContext(Dispatchers.Main) {
                    Toast.makeText(target_.requireContext(),
                        "Fail to copy " + e.localizedMessage,
                        Toast.LENGTH_LONG).show()
                }
            } finally {

                val fd = File(zipFileName)
                if (fd.isFile && fd.exists()) {
                    fd.delete()
                }

                withContext(Dispatchers.Main) {
                    listener_.onDismissDialog()
                }
            }
        }
    }

    @Suppress("BlockingMethodInNonBlockingContext")
    fun installGameFile(uri: Uri) {
        scope.launch {
            withContext(Dispatchers.Main) {
                listener_.onShowDialog("Installing ...")
            }
            try {
                val parcelFileDescriptor1: ParcelFileDescriptor?
                val uriString = uri.toString().lowercase(Locale.ROOT)
                var apath = ""
                try {
                    parcelFileDescriptor1 =
                        target_.requireActivity().contentResolver.openFileDescriptor(uri, "r")
                    if (parcelFileDescriptor1 != null) {
                        val fd: Int = parcelFileDescriptor1.fd
                        apath = "/proc/self/fd/$fd"
                    }
                } catch (e: Exception) {
                    Toast.makeText(target_.requireContext(),
                        "Fail to open $uriString with ${e.localizedMessage}",
                        Toast.LENGTH_LONG).show()
                    withContext(Dispatchers.Main) {
                        listener_.onDismissDialog()
                    }
                    return@launch
                }

                if (apath == "") {
                    Toast.makeText(target_.requireContext(), "Fail to open $uriString", Toast.LENGTH_LONG)
                        .show()
                    withContext(Dispatchers.Main) {
                        listener_.onDismissDialog()
                    }
                    return@launch
                }

                val gameinfo = GameInfo.genGameInfoFromCHD(apath)
                if (gameinfo != null) {

                    val bundle = Bundle()
                    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                    mFirebaseAnalytics.logEvent(
                        "yab_start_game", bundle
                    )
                    parcelFileDescriptor1!!.close()
                } else {
                    Toast.makeText(target_.requireContext(), "Fail to open $apath", Toast.LENGTH_LONG)
                        .show()
                    parcelFileDescriptor1?.close()
                    return@launch
                }

                withContext(Dispatchers.Main) {
                    listener_.onUpdateDialogMessage("Installing ${gameinfo.game_title}")
                }

                val fd =
                    File(storage.getInstallDir().absolutePath + "/" + gameinfo.product_number + ".chd")
                val parcelFileDescriptor =
                    target_.requireActivity().contentResolver.openFileDescriptor(uri, "r")
                val fileDescriptor: FileDescriptor = parcelFileDescriptor!!.fileDescriptor
                FileInputStream(fileDescriptor).use { inputStream ->
                    FileOutputStream(fd).use { outputStream ->
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                            FileUtils.copy(inputStream, outputStream)
                        } else {
                            copyFileO(inputStream, outputStream)
                        }
                        inputStream.close()
                        outputStream.close()
                    }
                }
                withContext(Dispatchers.Main) {

                    decrementInstallCount()

                    fileSelected(fd)
                }
                parcelFileDescriptor.close()
            } catch (e: Exception) {

                withContext(Dispatchers.Main) {
                    Toast.makeText(target_.requireContext(),
                        "Fail to copy " + e.localizedMessage,
                        Toast.LENGTH_LONG).show()
                }
            } finally {
                withContext(Dispatchers.Main) {
                    listener_.onDismissDialog()
                }
            }
        }
        return
    }

    fun startGame(item: GameInfo, launcher : ActivityResultLauncher<Intent>) {
        val c = Calendar.getInstance()
        item.lastplay_date = c.time
        item.save()

        val application = target_.requireActivity().application as YabauseApplication
        tracker = application.defaultTracker
        tracker?.send(
            HitBuilders.EventBuilder()
                .setCategory("Action")
                .setAction(item.game_title)
                .build()
        )
        val bundle = Bundle()
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, item.product_number)
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, item.game_title)
        mFirebaseAnalytics.logEvent(
            "yab_start_game", bundle
        )

        if (item.file_path.contains("content://") == true) {
            val intent = Intent(target_.activity, Yabause::class.java)
            intent.putExtra("org.uoyabause.android.FileNameUri", item.file_path)
            intent.putExtra("org.uoyabause.android.FileDir", item.iso_file_path)
            intent.putExtra("org.uoyabause.android.gamecode", item.product_number)
            launcher.launch(intent)
        } else {
            val intent = Intent(target_.activity, Yabause::class.java)
            intent.putExtra("org.uoyabause.android.FileNameEx", item.file_path)
            intent.putExtra("org.uoyabause.android.gamecode", item.product_number)
            launcher.launch(intent)
        }
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

    fun checkSignIn(  launcher : ActivityResultLauncher<Intent> ) {
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
            val builder = AlertDialog.Builder(
                ContextThemeWrapper1(
                    target_.activity, R.style.Theme_AppCompat)
            )
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
                    launcher.launch(
                        AuthUI.getInstance()
                            .createSignInIntentBuilder()
                            .setTheme(R.style.Theme_AppCompat)
                            .setTosAndPrivacyPolicyUrls(
                                "https://www.uoyabause.org/static_pages/eula.html",
                                "https://www.uoyabause.org/static_pages/privacy_policy")
                            .setAvailableProviders(Arrays.asList(GoogleBuilder().build()))
                            .build())
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

    fun fileSelected(file: File) {
        val apath: String = file.absolutePath
        // save last selected dir
        val sharedPref =
            PreferenceManager.getDefaultSharedPreferences(target_.requireActivity())
        val editor = sharedPref.edit()
        editor.putString("pref_last_dir", file.parent)
        editor.apply()
        var gameinfo: GameInfo? = GameInfo.getFromFileName(apath)
        if (gameinfo == null) {
            gameinfo = if (apath.endsWith("CUE") || apath.endsWith("cue")) {
                GameInfo.genGameInfoFromCUE(apath)
            } else if (apath.endsWith("MDS") || apath.endsWith("mds")) {
                GameInfo.genGameInfoFromMDS(apath)
            } else if (apath.endsWith("CHD") || apath.endsWith("chd")) {
                GameInfo.genGameInfoFromCHD(apath)
            } else if (apath.endsWith("CCD") || apath.endsWith("ccd")) {
                GameInfo.genGameInfoFromMDS(apath)
            } else {
                GameInfo.genGameInfoFromIso(apath)
            }
        }
        if (gameinfo != null) {
            scope.launch {
                gameinfo.updateState()
                val c = Calendar.getInstance()
                gameinfo.lastplay_date = c.time
                gameinfo.save()
                withContext(Dispatchers.Main) {
                    listener_.onLoadRows()
                    val bundle = Bundle()
                    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                    mFirebaseAnalytics.logEvent(
                        "yab_start_game", bundle
                    )
                    val intent = Intent(target_.requireActivity(), Yabause::class.java)
                    intent.putExtra("org.uoyabause.android.FileNameEx", apath)
                    intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
                    this@GameSelectPresenter.yabauseActivityLauncher.launch(intent)
                }
            }
        } else {
            return
        }
    }

    companion object {
//        const val RC_SIGN_IN = 123
        const val YABAUSE_ACTIVITY = 0x02
    }

    init {
        target_ = target
        listener_ = listener
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(target_.requireActivity())
    }

    fun calculateMD5(file: File): String {
        val md = MessageDigest.getInstance("MD5")
        val inputStream = file.inputStream()
        inputStream.use { input ->
            val buffer = ByteArray(4096)
            var bytesRead = input.read(buffer)
            while (bytesRead != -1) {
                md.update(buffer, 0, bytesRead)
                bytesRead = input.read(buffer)
            }
        }
        val bytes = md.digest()
        return Base64.getEncoder().encodeToString(bytes)
    }


    fun zip(sourceFile: File, zipFile: File) {
        Log.d(TAG,"zip time = ${sourceFile.lastModified()}")
        val buffer = ByteArray(4096)
        ZipOutputStream(zipFile.outputStream()).use { zipOut ->
            FileInputStream(sourceFile).use { fileIn ->
                val entry = ZipEntry(sourceFile.name)
                entry.setTime(sourceFile.lastModified())
                zipOut.putNextEntry(entry)
                var len: Int
                while (fileIn.read(buffer).also { len = it } > 0) {
                    zipOut.write(buffer, 0, len)
                }
                zipOut.closeEntry()
            }
        }
    }

    fun unzip(zipFile: File, destinationDirectory: File) {
        val buffer = ByteArray(4096)
        val zipIn = ZipInputStream(zipFile.inputStream())
        var entry = zipIn.nextEntry
        while (entry != null) {
            val outputFile = File(destinationDirectory, entry.name)
            if (entry.isDirectory) {
                outputFile.mkdirs()
            } else {
                outputFile.parentFile?.mkdirs()
                val fos = FileOutputStream(outputFile)
                var len = zipIn.read(buffer)
                while (len > 0) {
                    fos.write(buffer, 0, len)
                    len = zipIn.read(buffer)
                }
                fos.close()
            }
            // ファイルの日付情報を設定
            Log.d(TAG,"unzip time = ${entry.time}")
            outputFile.setLastModified(entry.time)
            entry = zipIn.nextEntry
        }
        zipIn.closeEntry()
        zipIn.close()
    }

    fun unixTimeToDateString(unixTime: Long): String {
        val instant = Instant.ofEpochSecond(unixTime/1000L)
        val zoneId = ZoneId.systemDefault()
        val formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")
        return formatter.format(instant.atZone(zoneId))
    }

    fun syncBackup() {

        target_.activity?.runOnUiThread {
            syncState = BackupSyncState.CHECKING_DOWNLOAD
            listener_.onShowDialog("Syncing Backup Memory ...")
        }

        val auth = FirebaseAuth.getInstance()
        val currentUser = auth.currentUser
        if (currentUser == null) {
            Log.d(TAG, "Fail to get currentUser even if Auth is successed!")
            syncState = BackupSyncState.IDLE
            listener_?.onDismissDialog();
            return
        }


        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")

        val storage = FirebaseStorage.getInstance()
        val storageRef = storage.reference.child(currentUser.uid).child("memory.zip")
        val localFile = File(mem)
        val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
        val memzip = YabauseStorage.storage.getMemoryPath("memory.zip")
        val localZipFile = File(memzip)

        Log.d(TAG,"syncBackup")
        storageRef.downloadUrl
            .addOnSuccessListener { uri ->
                // クラウドのファイルの更新日時を取得
                storageRef.metadata.addOnSuccessListener { metadata ->

                    // ローカルにファイルが存在しない場合
                    if( localFile.exists() == false ){
                        Log.d(TAG,"Local file not exits force download")
                        storageRef.getFile(localZipFile).addOnCompleteListener { task ->
                            if (task.isSuccessful) {

                                Log.d(TAG,"Download OK")

                                // ダウンロード成功時の処理
                                unzip(localZipFile,destinationDirectory)

                            } else {
                                // ダウンロード失敗時の処理
                            }

                            target_.activity?.runOnUiThread {
                                syncState = BackupSyncState.IDLE
                                listener_?.onDismissDialog();
                            }
                        }

                    // ローカルにファイルが存在する場合
                    }else {

                        val cloudUpdateTime = metadata.updatedTimeMillis
                        val localUpdateTime = localFile.lastModified()

                        Log.d(TAG,"Local file exits cloudTime:${unixTimeToDateString(cloudUpdateTime)},  loalTime:${unixTimeToDateString(localUpdateTime)}")

                        // クラウドのほうが古い場合、アップロード
                        if (cloudUpdateTime < localUpdateTime) {

                            Log.d(TAG,"Try uploading")

                            zip(localFile,localZipFile)

                            // 念のためハッシュ値を比較
                            val md5 = calculateMD5(localZipFile)
                            Log.d(TAG,"check md5hash cloud:${metadata.md5Hash},  local:${md5}")
                            if( md5 != metadata.md5Hash ) {

                                Log.d(TAG,"Start uploading")

                                listener_?.onUpdateDialogMessage("Upload backup file to Cloud");

                                // クラウドのファイルが古い場合、ローカルのファイルをアップロード
                                val inputStream = FileInputStream(localZipFile)
                                storageRef.putStream(inputStream).addOnCompleteListener { task ->
                                    if (task.isSuccessful) {
                                        // アップロード成功時の処理
                                        Log.d(TAG,"Upload OK")
                                    } else {
                                        // アップロード失敗時の処理
                                        Log.d(TAG,"Upload Fail")
                                    }
                                    target_.activity?.runOnUiThread {
                                        syncState = BackupSyncState.IDLE
                                        listener_?.onDismissDialog();
                                    }
                                }

                            }else{

                                Log.d(TAG,"Same data.")

                                target_.activity?.runOnUiThread {
                                    syncState = BackupSyncState.IDLE
                                    listener_?.onDismissDialog();
                                }
                            }

                        // クラウドのほうが新しい場合、ダウンロード
                        } else {

                            Log.d(TAG,"Try Downloading")

                            zip(localFile,localZipFile)
                            val md5 = calculateMD5(localZipFile)
                            Log.d(TAG,"check md5hash cloud:${metadata.md5Hash},  local:${md5}")

                            if( md5 != metadata.md5Hash ) {

                                Log.d(TAG,"Start Downloading")

                                listener_?.onUpdateDialogMessage("Downloading backup file from Cloud");
                                storageRef.getFile(localZipFile).addOnCompleteListener { task ->
                                    if (task.isSuccessful) {

                                        Log.d(TAG,"Download OK")

                                        val md51 = calculateMD5(localZipFile)
                                        Log.d(TAG," md5hash is updated cloud:${metadata.md5Hash},  local:${md51}")

                                        // ダウンロード成功時の処理
                                        unzip(localZipFile,destinationDirectory)

                                        // 以下ハッシュ値が同じになるかのチェック
                                        //val memzip2 = YabauseStorage.storage.getMemoryPath("memory2.zip")
                                        //val localZipFile2 = File(memzip2)
                                        //zip(localFile,localZipFile2)

                                        // 念のためハッシュ値を比較
                                        //val md5 = calculateMD5(localZipFile2)
                                        //Log.d(TAG," md5hash is updated cloud:${metadata.md5Hash},  local:${md5}")


                                    } else {
                                        // ダウンロード失敗時の処理
                                        Log.d(TAG,"Download Fail")
                                    }
                                    target_.activity?.runOnUiThread {
                                        syncState = BackupSyncState.IDLE
                                        listener_?.onDismissDialog();
                                    }
                                }
                            }else{

                                Log.d(TAG,"Same data.")

                                target_.activity?.runOnUiThread {
                                    syncState = BackupSyncState.IDLE
                                    listener_?.onDismissDialog();
                                }
                            }
                        }
                    }
                }.addOnFailureListener { exception ->
                    // メタデータの取得に失敗した場合の処理
                    target_.activity?.runOnUiThread {
                        syncState = BackupSyncState.IDLE
                        listener_?.onDismissDialog();
                    }
                }

            }
            .addOnFailureListener { exception ->
                if (exception is StorageException && exception.errorCode == StorageException.ERROR_OBJECT_NOT_FOUND) {

                    Log.d(TAG,"Cloud file not exits force upload")

                    zip(localFile,localZipFile)
                    val inputStream = FileInputStream(localZipFile)
                    storageRef.putStream(inputStream).addOnCompleteListener { task ->
                        if (task.isSuccessful) {
                            // アップロード成功時の処理
                        } else {
                            // アップロード失敗時の処理
                        }
                        target_.activity?.runOnUiThread {
                            syncState = BackupSyncState.IDLE
                            listener_?.onDismissDialog();
                        }
                    }

                } else {
                    // その他のエラーが発生した場合の処理
                    target_.activity?.runOnUiThread {
                        syncState = BackupSyncState.IDLE
                        listener_?.onDismissDialog();
                    }
                }
            }

    }
}
