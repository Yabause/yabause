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

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.UiModeManager
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.database.Cursor
import android.net.Uri
import android.os.Build
import android.os.Build.VERSION_CODES
import android.os.Bundle
import android.os.FileUtils
import android.os.ParcelFileDescriptor
import android.provider.OpenableColumns
import android.util.Log
import android.view.LayoutInflater
import android.view.MenuItem
import android.view.View
import android.view.View.VISIBLE
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.core.app.ActivityCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentStatePagerAdapter
import androidx.multidex.MultiDexApplication
import androidx.preference.PreferenceManager
import androidx.viewpager.widget.ViewPager
import com.activeandroid.query.Select
import com.bumptech.glide.Glide
import com.google.android.gms.analytics.HitBuilders.EventBuilder
import com.google.android.gms.analytics.HitBuilders.ScreenViewBuilder
import com.google.android.gms.analytics.Tracker
import com.google.android.material.navigation.NavigationView
import com.google.android.material.snackbar.Snackbar
import com.google.android.material.tabs.TabLayout
import com.google.firebase.analytics.FirebaseAnalytics
import io.noties.markwon.Markwon
import io.reactivex.Observer
import io.reactivex.disposables.Disposable
import java.io.File
import java.io.FileDescriptor
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException
import java.nio.channels.FileChannel
import java.util.Calendar
import java.util.Locale
import java.util.zip.ZipFile
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.apache.commons.compress.archivers.sevenz.SevenZFile
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.devmiyax.yabasanshiro.StartupActivity
import org.uoyabause.android.AdActivity
import org.uoyabause.android.FileDialog
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.GameInfo
import org.uoyabause.android.GameSelectPresenter
import org.uoyabause.android.GameSelectPresenter.GameSelectPresenterListener
import org.uoyabause.android.SettingsActivity
import org.uoyabause.android.ShowPinInFragment
import org.uoyabause.android.Yabause
import org.uoyabause.android.YabauseApplication
import org.uoyabause.android.YabauseStorage
import org.uoyabause.android.tv.GameSelectFragment

internal class GameListPage(val pageTitle: String, var gameList: GameItemAdapter)

internal class GameViewPagerAdapter(fm: FragmentManager?) :
    FragmentStatePagerAdapter(fm!!, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT) {

    private var gameListPageFragments: MutableList<Fragment>? = null
    private var gameListPages: MutableList<GameListPage>? = null

    fun setGameList(gameListPages: MutableList<GameListPage>?) {

        var position = 0
        if (gameListPages != null) {
            gameListPageFragments = ArrayList()
            for (item in gameListPages) {
                gameListPageFragments?.add(
                    GameListFragment.getInstance(
                        position,
                        gameListPages[position].pageTitle,
                        gameListPages[position].gameList
                    )
                )
                position += 1
            }
        }
        this.gameListPages = gameListPages
        this.notifyDataSetChanged()
    }

    override fun getItem(position: Int): Fragment {
        return gameListPageFragments!![position]
    }

    override fun getCount(): Int {
        return gameListPages?.size ?: return 0
    }

    override fun getPageTitle(position: Int): CharSequence? {
        return gameListPages!![position].pageTitle
    }

    override fun destroyItem(container: ViewGroup, position: Int, `object`: Any) {
        super.destroyItem(container, position, `object`)
    }

    override fun getItemPosition(xobj: Any): Int {
        return POSITION_NONE
    }
}

class GameSelectFragmentPhone : Fragment(),
    GameItemAdapter.OnItemClickListener,
    NavigationView.OnNavigationItemSelectedListener,
    FileSelectedListener,
    GameSelectPresenterListener {
    lateinit var presenter: GameSelectPresenter
    private var observer: Observer<*>? = null
    private val scope = CoroutineScope(Dispatchers.IO)
    private var refreshLevel = 0
    private var drawerLayout: DrawerLayout? = null
    private var tracker: Tracker? = null
    private var firebaseAnalytics: FirebaseAnalytics? = null
    private var isfisrtupdate = true
    private var navigationView: NavigationView? = null
    private lateinit var tabpageAdapter: GameViewPagerAdapter

    private lateinit var rootview: View
    private lateinit var drawerToggle: ActionBarDrawerToggle
    private lateinit var tabLayout: TabLayout
    private lateinit var progressBar: View
    private lateinit var progressMessage: TextView
    private var isBackGroundComplete = false

    private val alphabet = arrayOf(
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z"
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        instance = this
        presenter = GameSelectPresenter(this as Fragment, this)
        tabpageAdapter = GameViewPagerAdapter(this@GameSelectFragmentPhone.childFragmentManager)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        rootview = inflater.inflate(R.layout.fragment_game_select_fragment_phone, container, false)
        progressBar = rootview.findViewById(R.id.llProgressBar)
        progressBar.visibility = View.GONE
        progressMessage = rootview.findViewById(R.id.pbText)

        val fab: View = rootview.findViewById(R.id.fab)
        if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
            fab.setOnClickListener { _ ->

                val prefs = requireActivity().getSharedPreferences("private",
                    MultiDexApplication.MODE_PRIVATE)
                val InstallCount = prefs.getInt("InstallCount", 3)
                if( InstallCount > 0){
                    val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                    intent.addCategory(Intent.CATEGORY_OPENABLE)
                    intent.type = "*/*"
                    startActivityForResult(intent, READ_REQUEST_CODE)
                }else {
                    if (YabauseApplication.checkDonated(requireActivity()) == 0) {
                        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                        intent.addCategory(Intent.CATEGORY_OPENABLE)
                        intent.type = "*/*"
                        startActivityForResult(intent, READ_REQUEST_CODE)
                    }
                }
            }
        } else {
            fab.visibility = View.GONE
        }

        return rootview
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        drawerLayout!!.closeDrawers()
        when (item.itemId) {
            R.id.menu_item_setting -> {
                val intent = Intent(activity, SettingsActivity::class.java)
                startActivityForResult(intent, SETTING_ACTIVITY)
            }
            R.id.menu_item_load_game -> {
                if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {

                    val prefs = requireActivity().getSharedPreferences("private",
                        MultiDexApplication.MODE_PRIVATE)
                    val InstallCount = prefs.getInt("InstallCount", 3)
                    if( InstallCount > 0){
                        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                        intent.addCategory(Intent.CATEGORY_OPENABLE)
                        intent.type = "*/*"
                        startActivityForResult(intent, READ_REQUEST_CODE)
                    }else {
                        if (YabauseApplication.checkDonated(requireActivity()) == 0) {
                            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                            intent.addCategory(Intent.CATEGORY_OPENABLE)
                            intent.type = "*/*"
                            startActivityForResult(intent, READ_REQUEST_CODE)
                        }
                    }

                } else {
                    val sharedPref =
                        PreferenceManager.getDefaultSharedPreferences(activity)
                    val lastDir =
                        sharedPref.getString("pref_last_dir", YabauseStorage.storage.gamePath)
                    val fd =
                        FileDialog(activity, lastDir)
                    fd.addFileListener(this)
                    fd.showDialog()
                }
            }
            R.id.menu_item_update_game_db -> {
                refreshLevel = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            }
            R.id.menu_item_login -> if (item.title == getString(R.string.sign_out)) {
                presenter.signOut()
                item.setTitle(R.string.sign_in)
            } else {
                presenter.signIn()
            }
            R.id.menu_privacy_policy -> {
                val uri =
                    Uri.parse("https://www.uoyabause.org/static_pages/privacy_policy.html")
                val i = Intent(Intent.ACTION_VIEW, uri)
                startActivity(i)
            }
            R.id.menu_item_login_to_other -> {
                ShowPinInFragment.newInstance(presenter).show(childFragmentManager, "sample")
            }
        }
        return false
    }

    private fun checkStoragePermission(): Int {
        if (Build.VERSION.SDK_INT >= 23) { // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(
                    requireActivity().applicationContext,
                    Manifest.permission.READ_EXTERNAL_STORAGE
                )
                != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(
                    requireActivity().applicationContext,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                )
                != PackageManager.PERMISSION_GRANTED
            ) { // Contacts permissions have not been granted.
                Log.i(
                    TAG,
                    "Storage permissions has NOT been granted. Requesting permissions."
                )
                requestPermissions(
                    PERMISSIONS_STORAGE,
                    REQUEST_STORAGE
                )
                // }
                return -1
            }
        }
        return 0
    }

    private fun verifyPermissions(grantResults: IntArray): Boolean { // At least one result must be checked.
        if (grantResults.isEmpty()) {
            return false
        }
        // Verify that each required permission has been granted, otherwise return false.
        for (result in grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false
            }
        }
        return true
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        if (requestCode == REQUEST_STORAGE) {
            Log.i(TAG, "Received response for contact permissions request.")
            if (verifyPermissions(grantResults)) {
                updateGameList()
            } else {
                showRestartMessage()
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
    }

    private fun showSnackbar(id: Int) {
        Snackbar
            .make(rootview.rootView, getString(id), Snackbar.LENGTH_SHORT)
            .show()
    }

    private fun updateRecent() {
        gameListPages?.forEach { it ->
            if (it.pageTitle == "recent") {
                var rlist: MutableList<GameInfo?>? = null
                try {
                    rlist = Select()
                        .from(GameInfo::class.java)
                        .orderBy("lastplay_date DESC")
                        .limit(5)
                        .execute<GameInfo?>()
                } catch (e: Exception) {
                    Log.d(TAG, e.localizedMessage!!)
                }
                val resentAdapter = GameItemAdapter(rlist)
                resentAdapter.setOnItemClickListener(this)
                it.gameList = resentAdapter
            }
        }
        tabpageAdapter.setGameList(gameListPages)
        tabpageAdapter.notifyDataSetChanged()
    }

    @Suppress("BlockingMethodInNonBlockingContext")
    private fun openGameFileDirect(uri: Uri) {
        scope.launch {
            withContext(Dispatchers.Main) {
                showDialog("Opening ...")
            }

            var parcelFileDescriptor: ParcelFileDescriptor? = null
            val uriString = uri.toString().toLowerCase(Locale.ROOT)
            var apath = ""
            try {
                parcelFileDescriptor =
                    requireActivity().contentResolver.openFileDescriptor(uri, "r")
                if (parcelFileDescriptor != null) {
                    val fd: Int? = parcelFileDescriptor.fd
                    if (fd != null) {
                        apath = "/proc/self/fd/$fd"
                    }
                }
            } catch (fne: FileNotFoundException) {
                apath = ""
            }
            if (apath == "") {
                Toast.makeText(requireContext(), "Fail to open $uriString", Toast.LENGTH_LONG)
                    .show()
                parcelFileDescriptor?.close()
                withContext(Dispatchers.Main) {
                    dismissDialog()
                }
                return@launch
            }

            val gameinfo = GameInfo.genGameInfoFromCHD(apath)
            if (gameinfo != null) {

                val bundle = Bundle()
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                firebaseAnalytics!!.logEvent(
                    "yab_start_game", bundle
                )
                parcelFileDescriptor!!.close()
                val intent = Intent(activity, Yabause::class.java)
                intent.putExtra("org.uoyabause.android.FileNameUri", uri.toString())
                intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
                startActivityForResult(intent, YABAUSE_ACTIVITY)
            } else {
                Toast.makeText(requireContext(), "Fail to open $apath", Toast.LENGTH_LONG).show()
                parcelFileDescriptor?.close()
            }
            withContext(Dispatchers.Main) {
                dismissDialog()
            }
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
                showDialog("Installing ...")
            }
            var zipFileName = ""
            try {

                val f = File(path)
                zipFileName = YabauseStorage.storage.getInstallDir().absolutePath + "/" + f.name
                val fd = File(zipFileName)
                val parcelFileDescriptor =
                    requireActivity().contentResolver.openFileDescriptor(uri, "r")
                val fileDescriptor: FileDescriptor = parcelFileDescriptor!!.fileDescriptor
                FileInputStream(fileDescriptor).use { inputStream ->
                    FileOutputStream(fd).use { outputStream ->
                        if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
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
                    updateDialogString("Extracting ${fd.name}")
                }

                if (zipFileName.toLowerCase().endsWith("zip")) {

                    ZipFile(zipFileName).use { zip ->
                        zip.entries().asSequence().forEach { entry ->

                            if (entry.name.toLowerCase(Locale.ROOT).endsWith("ccd") ||
                                entry.name.toLowerCase(Locale.ROOT).endsWith("cue") ||
                                entry.name.toLowerCase(Locale.ROOT).endsWith("mds")
                            ) {
                                targetFileName = YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name
                            }
                            zip.getInputStream(entry).use { input ->
                                if (entry.isDirectory) {
                                    val unzipdir =
                                        File(YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name)
                                    if (!unzipdir.exists()) {
                                        unzipdir.mkdirs()
                                    } else {
                                        unzipdir.delete()
                                        unzipdir.mkdirs()
                                    }
                                } else {
                                    File(YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name).outputStream()
                                        .use { output ->
                                            input.copyTo(output)
                                        }
                                }
                            }
                        }
                    }
                } else if (zipFileName.toLowerCase().endsWith("7z")) {
                    SevenZFile(File(zipFileName)).use { sz ->
                        sz.entries.asSequence().forEach { entry ->
                            if (entry.name.toLowerCase(Locale.ROOT).endsWith("ccd") ||
                                entry.name.toLowerCase(Locale.ROOT).endsWith("cue") ||
                                entry.name.toLowerCase(Locale.ROOT).endsWith("mds")
                            ) {
                                targetFileName = YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name
                            }

                            if (entry.isDirectory) {
                                val unzipdir =
                                    File(YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name)
                                if (!unzipdir.exists()) {
                                    unzipdir.mkdirs()
                                } else {
                                    unzipdir.delete()
                                    unzipdir.mkdirs()
                                }
                            } else {
                                sz.getInputStream(entry).use { input ->
                                    File(YabauseStorage.storage.getInstallDir().absolutePath + "/" + entry.name).outputStream()
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

                        val prefs = requireActivity().getSharedPreferences("private",
                            MultiDexApplication.MODE_PRIVATE)
                        var InstallCount = prefs.getInt("InstallCount", 3)
                        InstallCount -= 1
                        if( InstallCount < 0 ){
                            InstallCount = 0
                        }
                        with( prefs.edit()) {
                            putInt("InstallCount", InstallCount)
                            apply()
                        }

                        fileSelected(File(targetFileName))
                    }
                } else {
                    Toast.makeText(requireContext(),
                        "ISO image is not found!!",
                        Toast.LENGTH_LONG).show()
                }
            } catch (e: Exception) {
                Toast.makeText(requireContext(),
                    "Fail to copy " + e.localizedMessage,
                    Toast.LENGTH_LONG).show()
            } catch (e: IOException) {
                Toast.makeText(requireContext(),
                    "Fail to copy " + e.localizedMessage,
                    Toast.LENGTH_LONG).show()
            } finally {

                val fd = File(zipFileName)
                if (fd.isFile && fd.exists()) {
                    fd.delete()
                }

                withContext(Dispatchers.Main) {
                    dismissDialog()
                }
            }
        }
    }

    @Suppress("BlockingMethodInNonBlockingContext")
    fun installGameFile(uri: Uri) {
        scope.launch {
            withContext(Dispatchers.Main) {
                showDialog("Installing ...")
            }
            try {
                val parcelFileDescriptor1: ParcelFileDescriptor?
                val uriString = uri.toString().toLowerCase(Locale.ROOT)
                var apath = ""
                try {
                    parcelFileDescriptor1 =
                        requireActivity().contentResolver.openFileDescriptor(uri, "r")
                    if (parcelFileDescriptor1 != null) {
                        val fd: Int? = parcelFileDescriptor1.fd
                        if (fd != null) {
                            apath = "/proc/self/fd/$fd"
                        }
                    }
                } catch (e: Exception) {
                    Toast.makeText(requireContext(),
                        "Fail to open $uriString with ${e.localizedMessage}",
                        Toast.LENGTH_LONG).show()
                    withContext(Dispatchers.Main) {
                        dismissDialog()
                    }
                    return@launch
                }

                if (apath == "") {
                    Toast.makeText(requireContext(), "Fail to open $uriString", Toast.LENGTH_LONG)
                        .show()
                    withContext(Dispatchers.Main) {
                        dismissDialog()
                    }
                    return@launch
                }

                val gameinfo = GameInfo.genGameInfoFromCHD(apath)
                if (gameinfo != null) {

                    val bundle = Bundle()
                    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                    firebaseAnalytics!!.logEvent(
                        "yab_start_game", bundle
                    )
                    parcelFileDescriptor1!!.close()
                } else {
                    Toast.makeText(requireContext(), "Fail to open $apath", Toast.LENGTH_LONG)
                        .show()
                    parcelFileDescriptor1?.close()
                    return@launch
                }

                withContext(Dispatchers.Main) {
                    updateDialogString("Installing ${gameinfo.game_title}")
                }

                val fd =
                    File(YabauseStorage.storage.getInstallDir().absolutePath + "/" + gameinfo.product_number + ".chd")
                val parcelFileDescriptor =
                    requireActivity().contentResolver.openFileDescriptor(uri, "r")
                val fileDescriptor: FileDescriptor = parcelFileDescriptor!!.fileDescriptor
                FileInputStream(fileDescriptor).use { inputStream ->
                    FileOutputStream(fd).use { outputStream ->
                        if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
                            FileUtils.copy(inputStream, outputStream)
                        } else {
                            copyFileO(inputStream, outputStream)
                        }
                        inputStream.close()
                        outputStream.close()
                    }
                }
                withContext(Dispatchers.Main) {

                    val prefs = requireActivity().getSharedPreferences("private",
                        MultiDexApplication.MODE_PRIVATE)
                    var InstallCount = prefs.getInt("InstallCount", 3)
                    InstallCount -= 1
                    if( InstallCount < 0 ){
                        InstallCount = 0
                    }
                    with( prefs.edit()) {
                        putInt("InstallCount", InstallCount)
                        apply()
                    }

                    fileSelected(fd)
                }
                parcelFileDescriptor.close()
            } catch (e: Exception) {
                Toast.makeText(requireContext(),
                    "Fail to copy " + e.localizedMessage,
                    Toast.LENGTH_LONG).show()
            } finally {
                withContext(Dispatchers.Main) {
                    dismissDialog()
                }
            }
        }
        return
    }

    override fun onActivityResult(
        requestCode: Int,
        resultCode: Int,
        data: Intent?
    ) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == READ_REQUEST_CODE && resultCode == Activity.RESULT_OK) {
            // The document selected by the user won't be returned in the intent.
            // Instead, a URI to that document will be contained in the return intent
            // provided to this method as a parameter.  Pull that uri using "resultData.getData()"
            if (data != null) {
                val uri = data.data
                if (uri != null) {
                    Log.i(TAG, "Uri: $uri")

                    val cursor: Cursor? = requireActivity().contentResolver.query(uri,
                        null, null, null, null)

                    cursor!!.moveToFirst()
                    val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    val path = cursor.getString(nameIndex)
                    if (path.toLowerCase(Locale.ROOT).endsWith("chd")) {
                        var size: Long = cursor.getLong(cursor.getColumnIndex(OpenableColumns.SIZE))
                        cursor.close()

                        size = size / 1024 / 1024

                        val prefs: SharedPreferences = requireActivity().getSharedPreferences("private",
                            AppCompatActivity.MODE_PRIVATE)
                        val count = prefs.getInt("InstallCount", 3)

                        var message =
                            getString(R.string.install_game_message) + " " + size + getString(R.string.install_game_message_after)

                        if(BuildConfig.BUILD_TYPE != "pro"){
                            message += getString(R.string.remaining_installation_count_is) + " " + count + "."
                        }

                        AlertDialog.Builder(requireActivity())
                            .setTitle(getString(R.string.do_you_want_to_install))
                            .setMessage(message)
                            .setTitle(R.string.do_you_want_to_install)
                            .setPositiveButton(R.string.yes) { _, _ ->


                                if( YabauseStorage.storage.hasExternalSD() ) {
                                    var selectItem: Int = 0

                                    val ctx = YabauseApplication.appContext
                                    val sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx)
                                    val path = sharedPref.getString("pref_install_location","0")
                                    val defaultItem = path?.toInt() ?: 0

                                    var option: Array<String> = arrayOf()
                                    option += "Internal(" + YabauseStorage.storage.getAvailableInternalMemorySize() +" free)"
                                    option += "External("+ YabauseStorage.storage.getAvailableExternalMemorySize() +" free)"

                                    AlertDialog.Builder(requireActivity())
                                        .setTitle("Which Storage?")
                                        .setSingleChoiceItems(option, defaultItem,
                                            { dialog, which ->
                                                selectItem = which
                                            })
                                        .setPositiveButton(R.string.ok) { _, _ ->

                                            val editor = sharedPref.edit()
                                            editor.putString("pref_install_location",selectItem.toString())
                                            editor.commit()

                                            installGameFile(uri)

                                        }
                                        .setNegativeButton(R.string.cancel){ _, _ -> }
                                        .setCancelable(true)
                                        .show()
                                }else{
                                    installGameFile(uri)
                                }

                             }
                            .setNegativeButton(R.string.no) { _, _ -> openGameFileDirect(uri) }
                            .setCancelable(true)
                            .show()
                    } else if (path.toLowerCase().endsWith("zip") || path.toLowerCase().endsWith("7z")) {

                        if( YabauseStorage.storage.hasExternalSD() ) {
                            var selectItem: Int = 0

                            val ctx = YabauseApplication.appContext
                            val sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx)
                            val location = sharedPref.getString("pref_install_location","0")
                            val defaultItem = location?.toInt() ?: 0

                            var option: Array<String> = arrayOf()
                            option += "Internal(" + YabauseStorage.storage.getAvailableInternalMemorySize() +" free)"
                            option += "External("+ YabauseStorage.storage.getAvailableExternalMemorySize() +" free)"

                            AlertDialog.Builder(requireActivity())
                                .setTitle("Which Storage?")
                                .setSingleChoiceItems(option, defaultItem,
                                    { dialog, which ->
                                        selectItem = which
                                    })
                                .setPositiveButton(R.string.ok) { _, _ ->

                                    val editor = sharedPref.edit()
                                    editor.putString("pref_install_location",selectItem.toString())
                                    editor.commit()
                                    installZipGameFile(uri, path)

                                }
                                .setNegativeButton(R.string.cancel){ _, _ -> }
                                .setCancelable(true)
                                .show()
                        }else{
                            installZipGameFile(uri, path)
                        }

                    } else {
                        Toast.makeText(requireContext(),
                            getString(R.string.only_chd_is_supported_for_load_game),
                            Toast.LENGTH_LONG).show()
                    }
                    return
                }
            }
            return
        }

        when (requestCode) {
            GameSelectPresenter.RC_SIGN_IN -> {
                presenter.onSignIn(resultCode, data)
                if (presenter.currentUserName != null) {
                    val m = navigationView!!.menu
                    val miLogin = m.findItem(R.id.menu_item_login)
                    miLogin.setTitle(R.string.sign_out)
                }
            }
            DOWNLOAD_ACTIVITY -> {
                if (resultCode == 0) {
                    refreshLevel = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                }
                if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_UPDATED) {
                    refreshLevel = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                } else if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_RESTART) {
                    val intent = Intent(activity, StartupActivity::class.java)
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(intent)
                    activity?.finish()
                }
            }
            SETTING_ACTIVITY -> if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_UPDATED) {
                refreshLevel = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            } else if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_RESTART) {
                val intent = Intent(activity, StartupActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                startActivity(intent)
                activity?.finish()
            }
            AD_ACTIVITY -> {
                updateRecent()
            }
            YABAUSE_ACTIVITY -> if (BuildConfig.BUILD_TYPE != "pro") {
                val prefs = activity?.getSharedPreferences(
                    "private",
                    Context.MODE_PRIVATE
                )
                val hasDonated = prefs?.getBoolean("donated", false)
                if (hasDonated == false) {
                    val rn = Math.random()
                    if (rn <= 0.5) {
                        val uiModeManager =
                            activity?.getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
                        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
                            // if (mInterstitialAd!!.isLoaded) {
                            //    mInterstitialAd!!.show()
                            // } else {
                            val intent = Intent(
                                activity,
                                AdActivity::class.java
                            )
                            startActivityForResult(intent, AD_ACTIVITY)
                            // }
                        } else {
                            val intent =
                                Intent(activity, AdActivity::class.java)
                            startActivityForResult(intent, AD_ACTIVITY)
                        }
                    } else if (rn > 0.5) {
                        val intent =
                            Intent(activity, AdActivity::class.java)
                        startActivityForResult(intent, AD_ACTIVITY)
                    }
                }
            } else {
                updateRecent()
            }
            else -> {
                updateRecent()
            }
        }
    }

    override fun fileSelected(file: File?) {
        if (file == null) { // canceled
            return
        }
        val apath: String = file.absolutePath
        // save last selected dir
        val sharedPref =
            PreferenceManager.getDefaultSharedPreferences(activity)
        val editor = sharedPref.edit()
        editor.putString("pref_last_dir", file.parent)
        editor.apply()
        var gameinfo = GameInfo.getFromFileName(apath)
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
                    loadRows()
                    val bundle = Bundle()
                    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                    firebaseAnalytics!!.logEvent(
                        "yab_start_game", bundle
                    )
                    val intent = Intent(activity, Yabause::class.java)
                    intent.putExtra("org.uoyabause.android.FileNameEx", apath)
                    intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
                    startActivityForResult(intent, YABAUSE_ACTIVITY)
                }
            }
        } else {
            return
        }
    }

    fun showDialog(message: String?) {
        if (message != null) {
            progressMessage.text = message
        } else {
            progressMessage.text = getString(R.string.updating)
        }
        progressBar.visibility = VISIBLE
    }

    fun updateDialogString(msg: String) {
        progressMessage.text = msg
    }

    fun dismissDialog() {
        progressBar.visibility = View.GONE
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        val activity = activity as AppCompatActivity
        // Log.i(TAG, "onCreate");
        super.onActivityCreated(savedInstanceState)
        firebaseAnalytics = FirebaseAnalytics.getInstance(activity)
        val application = activity.application as YabauseApplication
        tracker = application.defaultTracker
/*
        MobileAds.initialize(requireActivity(), getString(R.string.ad_app_id))
        mInterstitialAd = InterstitialAd(activity)
        mInterstitialAd!!.adUnitId = getString(R.string.banner_ad_unit_id)
        requestNewInterstitial()
        mInterstitialAd!!.adListener = object : AdListener() {
            override fun onAdClosed() {
                requestNewInterstitial()
            }
        }
*/
        val toolbar =
            rootview.findViewById<View>(R.id.toolbar) as Toolbar
        toolbar.setLogo(R.mipmap.ic_launcher)
        toolbar.title = getString(R.string.app_name)
        toolbar.subtitle = getVersionName(activity)
        activity.setSupportActionBar(toolbar)
        tabLayout = rootview.findViewById(R.id.tab_game_index)
        tabLayout.removeAllTabs()

        drawerLayout =
            rootview.findViewById<View>(R.id.drawer_layout_game_select) as DrawerLayout
        drawerToggle = object : ActionBarDrawerToggle(
            getActivity(), /* host Activity */
            drawerLayout, /* DrawerLayout object */
            R.string.drawer_open, /* "open drawer" description */
            R.string.drawer_close /* "close drawer" description */
        ) {

            override fun onDrawerOpened(drawerView: View) {
                super.onDrawerOpened(drawerView)
                // activity.getSupportActionBar().setTitle("bbb");
                val tx = rootview.findViewById<TextView?>(R.id.menu_title)
                val uname = presenter.currentUserName
                if (tx != null && uname != null) {
                    tx.text = uname
                } else {
                    tx.text = ""
                }
                val iv =
                    rootview.findViewById<ImageView?>(R.id.navi_header_image)
                val uri = presenter.currentUserPhoto
                if (iv != null && uri != null) {
                    Glide.with(drawerView.context)
                        .load(uri)
                        .into(iv)
                } else {
                    iv.setImageResource(R.mipmap.ic_launcher)
                }
            }
        }
        // Set the drawer toggle as the DrawerListener
        drawerLayout!!.addDrawerListener(drawerToggle)
        activity.supportActionBar!!.setDisplayHomeAsUpEnabled(true)
        activity.supportActionBar!!.setHomeButtonEnabled(true)
        drawerToggle.syncState()
        navigationView =
            rootview.findViewById<View>(R.id.nav_view) as NavigationView
        if (navigationView != null) {
            navigationView!!.setNavigationItemSelectedListener(this)
        }

        if (presenter.currentUserName != null) {
            val m = navigationView!!.menu
            val miLogin = m.findItem(R.id.menu_item_login)
            miLogin.setTitle(R.string.sign_out)
        } else {
            val m = navigationView!!.menu
            val miLogin = m.findItem(R.id.menu_item_login)
            miLogin.setTitle(R.string.sign_in)
        }
        // updateGameList();
        if (checkStoragePermission() == 0) {
            updateGameList()
        }

        // if( isfisrtupdate == false && this@GameSelectFragmentPhone.activity?.intent!!.getBooleanExtra("showPin",false) ) {
        //    ShowPinInFragment.newInstance(presenter_).show(childFragmentManager, "sample");
        // }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        drawerToggle.onConfigurationChanged(newConfig)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean { // Pass the event to ActionBarDrawerToggle, if it returns
// true, then it has handled the app icon touch event
        return if (drawerToggle.onOptionsItemSelected(item)) {
            true
        } else super.onOptionsItemSelected(
            item
        )
    }

    private fun updateGameList() {
        if (observer != null) return
        isBackGroundComplete = false
        val tmpObserver = object : Observer<String> {
            // GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();
            override fun onSubscribe(d: Disposable) {
                observer = this
                showDialog(null)
            }

            override fun onNext(response: String) {
                updateDialogString("${getString(R.string.updating)} $response")
            }

            override fun onError(e: Throwable) {
                observer = null
                dismissDialog()
            }

            override fun onComplete() {
                if (!isFront) {
                    observer = null
                    dismissDialog()
                    isBackGroundComplete = true
                    return
                }

                loadRows()
                val viewPager = rootview.findViewById(R.id.view_pager_game_index) as? ViewPager
                // viewPager!!.setSaveFromParentEnabled(false)
                // viewPager!!.removeAllViews()
                tabpageAdapter.setGameList(gameListPages)
                viewPager!!.adapter = tabpageAdapter
                // tablayout_.removeAllTabs()
                tabLayout.setupWithViewPager(viewPager)

                dismissDialog()
                if (isfisrtupdate) {
                    isfisrtupdate = false
                    if (this@GameSelectFragmentPhone.requireActivity().intent!!.getBooleanExtra(
                            "showPin",
                            false
                        )
                    ) {
                        ShowPinInFragment.newInstance(presenter).show(
                            childFragmentManager,
                            "sample"
                        )
                    } else {
                        presenter.checkSignIn()
                    }
                }
                viewPager.adapter!!.notifyDataSetChanged()
                observer = null
            }
        }
        presenter.updateGameList(refreshLevel, tmpObserver)
        refreshLevel = 0
    }

    private fun showRestartMessage() { // need_to_accept
        val viewMessage = rootview.findViewById<TextView?>(R.id.empty_message)
        val viewPager = rootview.findViewById<ViewPager?>(R.id.view_pager_game_index)
        viewMessage?.visibility = VISIBLE
        viewPager?.visibility = View.GONE

        val welcomeMessage = resources.getString(R.string.need_to_accept)
        viewMessage.text = welcomeMessage
    }

    internal var gameListPages: MutableList<GameListPage>? = null

    fun getGameItemAdapter(index: String): GameItemAdapter? {

        if (gameListPages == null) {
            loadRows()
        }

        if (gameListPages != null) {
            for (page in this.gameListPages!!) {
                if (page.pageTitle == index) {
                    return page.gameList
                }
            }
        }
        return null
    }

    @SuppressLint("StringFormatInvalid")
    private fun loadRows() {

        // -----------------------------------------------------------------
        // Recent Play Game
        try {
            val checklist = Select()
                .from(GameInfo::class.java)
                .limit(1)
                .execute<GameInfo?>()
            if (checklist.size == 0) {

                val viewMessage = rootview.findViewById<TextView?>(R.id.empty_message)
                val viewPager = rootview.findViewById<ViewPager?>(R.id.view_pager_game_index)
                viewMessage!!.visibility = VISIBLE
                viewPager!!.visibility = View.GONE

                val markwon = Markwon.create(this.activity as Context)
                val welcomeMessage = resources.getString(
                    R.string.welcome,
                    YabauseStorage.storage.gamePath
                )
                markwon.setMarkdown(viewMessage, welcomeMessage)

                return
            }
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage!!)
        }

        val viewMessage = rootview.findViewById(R.id.empty_message) as? View
        val viewPager = rootview.findViewById(R.id.view_pager_game_index) as? ViewPager
        viewMessage?.visibility = View.GONE
        viewPager?.visibility = VISIBLE

        // -----------------------------------------------------------------
        // Recent Play Game
        var rlist: MutableList<GameInfo?>? = null
        try {
            rlist = Select()
                .from(GameInfo::class.java)
                .orderBy("lastplay_date DESC")
                .limit(5)
                .execute<GameInfo?>()
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage!!)
        }
        val resentAdapter = GameItemAdapter(rlist)

        gameListPages = mutableListOf()

        val resentPage = GameListPage("recent", resentAdapter)
        resentAdapter.setOnItemClickListener(this)
        gameListPages!!.add(resentPage)

        // -----------------------------------------------------------------
        //
        var list: MutableList<GameInfo>? = null
        try {
            list = Select()
                .from(GameInfo::class.java)
                .orderBy("game_title ASC")
                .execute()
        } catch (e: Exception) {
//            Log.d(TAG, "${e.localizedMessage}")
        }
        var i = 0
        while (i < alphabet.size) {
            var hit = false
            val alphabetedList: MutableList<GameInfo?>? =
                ArrayList()
            val it = list!!.iterator()
            while (it.hasNext()) {
                val game = it.next()
                if (game.game_title.toUpperCase(Locale.ROOT).indexOf(alphabet[i]) == 0) {
                    alphabetedList!!.add(game)
                    Log.d(
                        "GameSelect",
                        alphabet[i] + ":" + game.game_title
                    )
                    it.remove()
                    hit = true
                }
            }
            if (hit) {
                val pageAdapter = GameItemAdapter(alphabetedList)
                pageAdapter.setOnItemClickListener(this)
                val aPage = GameListPage(alphabet[i], pageAdapter)
                gameListPages!!.add(aPage)
            }
            i++
        }
        val othersList: MutableList<GameInfo?>? = ArrayList()
        val it: Iterator<GameInfo> = list!!.iterator()
        while (it.hasNext()) {
            val game = it.next()
            Log.d("GameSelect", "Others:" + game.game_title)
            othersList!!.add(game)
        }

        if (othersList!!.size > 0) {
            val otherAdapter = GameItemAdapter(othersList)
            otherAdapter.setOnItemClickListener(this)
            val otherPage = GameListPage("OTHERS", otherAdapter)
            gameListPages!!.add(otherPage)
        }
    }

    override fun onItemClick(position: Int, item: GameInfo?, v: View?) {
        val c = Calendar.getInstance()
        item?.lastplay_date = c.time
        item?.save()
        if (tracker != null) {
            tracker!!.send(
                EventBuilder()
                    .setCategory("Action")
                    .setAction(item?.game_title)
                    .build()
            )
        }
        val bundle = Bundle()
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, item?.product_number)
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, item?.game_title)
        firebaseAnalytics!!.logEvent(
            "yab_start_game", bundle
        )
        val intent = Intent(activity, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", item?.file_path)
        intent.putExtra("org.uoyabause.android.gamecode", item?.product_number)
        startActivityForResult(intent, YABAUSE_ACTIVITY)
    }

    @SuppressLint("DefaultLocale")
    override fun onGameRemoved(item: GameInfo?) {
        if (item == null) return
        val recentPage = gameListPages!!.find { it.pageTitle == "RECENT" }
        recentPage?.gameList?.removeItem(item.id)

        val title = item.game_title.toUpperCase()[0]
        val alphabetPage = gameListPages!!.find { it.pageTitle == title.toString() }
        if (alphabetPage != null) {
            alphabetPage.gameList.removeItem(item.id)
            if (alphabetPage.gameList.itemCount == 0) {

                val index = gameListPages!!.indexOfFirst { it.pageTitle == title.toString() }
                if (index != -1) {
                    tabLayout.removeTabAt(index)
                }

                gameListPages!!.removeAll { it.pageTitle == title.toString() }
                tabpageAdapter.notifyDataSetChanged()
            }
        }

        val othersPage = gameListPages!!.find { it.pageTitle == "OTHERS" }
        if (othersPage != null) {
            othersPage.gameList.removeItem(item.id)
            if (othersPage.gameList.itemCount == 0) {

                val index = gameListPages!!.indexOfFirst { it.pageTitle == "OTHERS" }
                if (index != -1) {
                    tabLayout.removeTabAt(index)
                }

                gameListPages!!.removeAll { it.pageTitle == "OTHERS" }
                tabpageAdapter.notifyDataSetChanged()
            }
        }
    }

    override fun onResume() {
        super.onResume()
        if (tracker != null) { // mTracker.setScreenName(TAG);
            tracker!!.send(ScreenViewBuilder().build())
        }
        isFront = true
        if (isBackGroundComplete) {
            updateGameList()
        }
    }

    var isFront = true

    override fun onPause() {
        isFront = false
        super.onPause()
    }

    override fun onDestroy() {
        System.gc()
        super.onDestroy()
    }

    override fun onShowMessage(string_id: Int) {
        showSnackbar(string_id)
    }

    companion object {
        private const val TAG = "GameSelectFragmentPhone"
        private const val SETTING_ACTIVITY = 0x01
        private const val YABAUSE_ACTIVITY = 0x02
        private const val DOWNLOAD_ACTIVITY = 0x03
        private const val AD_ACTIVITY = 0x04
        private const val READ_REQUEST_CODE = 0xFFE0

        private var instance: GameSelectFragmentPhone? = null

        @JvmField
        var myOnClickListener: View.OnClickListener? = null
        private const val REQUEST_STORAGE = 1
        private val PERMISSIONS_STORAGE = arrayOf(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
        )

        fun newInstance(): GameSelectFragmentPhone {
            val fragment = GameSelectFragmentPhone()
            val args = Bundle()
            fragment.arguments = args
            return fragment
        }

        fun getInstance(): GameSelectFragmentPhone? {
            return instance
        }

        fun getVersionName(context: Context): String {
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
