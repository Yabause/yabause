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
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.database.Cursor
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.FileUtils
import android.os.ParcelFileDescriptor
import android.provider.OpenableColumns
import android.util.Log
import android.view.LayoutInflater
import android.view.MenuItem
import android.view.View
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
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager.widget.ViewPager
import com.activeandroid.query.Select
import com.bumptech.glide.Glide
import com.google.android.gms.ads.AdRequest
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
import java.util.Calendar
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.cattaka.android.adaptertoolbox.thirdparty.MergeRecyclerAdapter
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

internal class GameViewPagerAdapter(var fm: FragmentManager?) :
    FragmentStatePagerAdapter(fm!!) {

    var gameListPageFragments: MutableList<Fragment>? = null
    var gameListPages_: MutableList<GameListPage>? = null

    fun setGameList(gameListPages: MutableList<GameListPage>?) {
/*
        val fragments = fm?.fragments
        if (fragments != null) {
            val ft = fm?.beginTransaction()
            for (f in fragments) {
                if (f is GameListFragment) {
                    ft?.remove(f)
                }
            }
            ft?.commitNow()
        }
*/
        // gameListPageFragments?.clear()
        // gameListPages_?.clear()
        // this.notifyDataSetChanged()

        // gameListPageFragments?.clear()
        // this.notifyDataSetChanged()

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
        gameListPages_ = gameListPages
        this.notifyDataSetChanged()
    }

    override fun getItem(position: Int): Fragment {
        return gameListPageFragments!![position]
    }

    override fun getCount(): Int {
        return gameListPages_?.size ?: return 0
    }

    override fun getPageTitle(position: Int): CharSequence? {
        return gameListPages_!![position].pageTitle
    }

    override fun destroyItem(container: ViewGroup, position: Int, `object`: Any) {
        super.destroyItem(container, position, `object`)
    }

    override fun getItemPosition(xobj: Any): Int {

        // val target = xobj as Fragment
        // if (gameListPageFragments?.contains(target) == true) {
        //    return POSITION_UNCHANGED
        // }

/*
        for ((i, myobj) in gameListPageFragments!!.withIndex()) {
            if (xobj == myobj) {
                return i
            }
        }
*/
        return POSITION_NONE
    }
}

class GameSelectFragmentPhone : Fragment(),
        GameItemAdapter.OnItemClickListener,
        NavigationView.OnNavigationItemSelectedListener,
        FileSelectedListener,
        GameSelectPresenterListener {
    var refresh_level = 0
    var mStringsHeaderAdapter: GameIndexAdapter? = null
    lateinit var presenter_: GameSelectPresenter
    val SETTING_ACTIVITY = 0x01
    val YABAUSE_ACTIVITY = 0x02
    val DOWNLOAD_ACTIVITY = 0x03
    val AD_ACTIVITY = 0x04
    private var mDrawerLayout: DrawerLayout? = null
    // private var mInterstitialAd: InterstitialAd? = null
    private var mTracker: Tracker? = null
    private var mFirebaseAnalytics: FirebaseAnalytics? = null
    var isfisrtupdate = true
    var mNavigationView: NavigationView? = null
    private lateinit var tabpage_adapter: GameViewPagerAdapter

    lateinit var rootview_: View
    lateinit var mDrawerToggle: ActionBarDrawerToggle
    lateinit var mMergeRecyclerAdapter: MergeRecyclerAdapter<RecyclerView.Adapter<*>?>
    lateinit var layoutManager: RecyclerView.LayoutManager
    lateinit var tablayout_: TabLayout
    lateinit var llProgressBar: View
    lateinit var progress_message: TextView

    var alphabet = arrayOf(
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
    val TAG = "GameSelectFragmentPhone"
    private var mListener: OnFragmentInteractionListener? =
        null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        instance = this
        presenter_ = GameSelectPresenter(this as Fragment, this)
        tabpage_adapter = GameViewPagerAdapter(this@GameSelectFragmentPhone.childFragmentManager)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        rootview_ = inflater.inflate(R.layout.fragment_game_select_fragment_phone, container, false)
        llProgressBar = rootview_.findViewById(R.id.llProgressBar)
        llProgressBar.visibility = View.GONE
        progress_message = rootview_.findViewById(R.id.pbText)
        return rootview_
    }

    fun onButtonPressed(uri: Uri?) {
        if (mListener != null) {
            mListener!!.onFragmentInteraction(uri)
        }
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is OnFragmentInteractionListener) {
            mListener =
                context
        } else { //            throw new RuntimeException(context.toString()
//                    + " must implement OnFragmentInteractionListener");
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    interface OnFragmentInteractionListener {
        // TODO: Update argument type and name
        fun onFragmentInteraction(uri: Uri?)
    }

    val READ_REQUEST_CODE = 0xFFE0

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        mDrawerLayout!!.closeDrawers()
        val id = item.itemId
        when (id) {
            R.id.menu_item_setting -> {
                val intent = Intent(activity, SettingsActivity::class.java)
                startActivityForResult(intent, SETTING_ACTIVITY)
            }
            R.id.menu_item_load_game -> {

                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
                    val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                    intent.addCategory(Intent.CATEGORY_OPENABLE)
                    intent.type = "*/*"
                    startActivityForResult(intent, READ_REQUEST_CODE)
                } else {
                    val sharedPref =
                        PreferenceManager.getDefaultSharedPreferences(activity)
                    val last_dir =
                        sharedPref.getString("pref_last_dir", YabauseStorage.storage.gamePath)
                    val fd =
                        FileDialog(activity, last_dir)
                    fd.addFileListener(this)
                    fd.showDialog()
                }
            }
            R.id.menu_item_update_game_db -> {
                refresh_level = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            }
            R.id.menu_item_login -> if (item.title == getString(R.string.sign_out) == true) {
                presenter_.signOut()
                item.setTitle(R.string.sign_in)
            } else {
                presenter_.signIn()
            }
            R.id.menu_privacy_policy -> {
                val uri =
                    Uri.parse("https://www.uoyabause.org/static_pages/privacy_policy.html")
                val i = Intent(Intent.ACTION_VIEW, uri)
                startActivity(i)
            }
            R.id.menu_item_login_to_other -> {
                ShowPinInFragment.newInstance(presenter_).show(childFragmentManager, "sample")
            }
        }
        return false
    }

    fun checkStoragePermission(): Int {
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

    fun verifyPermissions(grantResults: IntArray): Boolean { // At least one result must be checked.
        if (grantResults.size < 1) {
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
            if (verifyPermissions(grantResults) == true) {
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
            .make(rootview_.rootView, getString(id), Snackbar.LENGTH_SHORT)
            .show()
    }

    fun updateRecent() {
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
                    Log.d(TAG, e.localizedMessage)
                }
                val resent_adapter = GameItemAdapter(rlist)
                resent_adapter.setOnItemClickListener(this)
                it.gameList = resent_adapter
            }
        }
        tabpage_adapter.setGameList(gameListPages)
        tabpage_adapter.notifyDataSetChanged()
    }

    val scope = CoroutineScope(Dispatchers.Default)
    fun openGameFileDirect(uri: Uri) {
        scope.launch {
            withContext(Dispatchers.Main) {
                showDialog("Opening ...")
            }

            var mParcelFileDescriptor: ParcelFileDescriptor? = null
            val uristring = uri.toString().toLowerCase()
            var apath = ""
            try {
                mParcelFileDescriptor =
                    requireActivity()!!.contentResolver.openFileDescriptor(uri, "r")
                if (mParcelFileDescriptor != null) {
                    val fd: Int? = mParcelFileDescriptor?.getFd()
                    if (fd != null) {
                        apath = "/proc/self/fd/$fd"
                    }
                }
            } catch (fne: FileNotFoundException) {
                apath = ""
            }
            if (apath == "") {
                Toast.makeText(requireContext(), "Fail to open $uristring", Toast.LENGTH_LONG)
                    .show()
                mParcelFileDescriptor?.close()
                withContext(Dispatchers.Main) {
                    dismissDialog()
                }
                return@launch
            }

            var gameinfo: GameInfo? = null
            gameinfo = GameInfo.genGameInfoFromCHD(apath)
            if (gameinfo != null) {

                val bundle = Bundle()
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                mFirebaseAnalytics!!.logEvent(
                    "yab_start_game", bundle
                )
                mParcelFileDescriptor!!.close()
                val intent = Intent(activity, Yabause::class.java)
                intent.putExtra("org.uoyabause.android.FileNameUri", uri.toString())
                intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
                startActivityForResult(intent, YABAUSE_ACTIVITY)
            } else {
                Toast.makeText(requireContext(), "Fail to open $apath", Toast.LENGTH_LONG).show()
                mParcelFileDescriptor?.close()
            }
            withContext(Dispatchers.Main) {
                dismissDialog()
            }
        }
    }

    fun installGameFile(uri: Uri) {
        scope.launch {
            withContext(Dispatchers.Main) {
                showDialog("Installing ...")
            }
            try {
                var gameinfo: GameInfo? = null

                var mParcelFileDescriptor: ParcelFileDescriptor? = null

                val uristring = uri.toString().toLowerCase()
                var apath = ""
                try {
                    mParcelFileDescriptor = requireActivity()!!.contentResolver.openFileDescriptor(uri, "r")
                    if (mParcelFileDescriptor != null) {
                        val fd: Int? = mParcelFileDescriptor?.getFd()
                        if (fd != null) {
                            apath = "/proc/self/fd/$fd"
                        }
                    }
                } catch (e: Exception) {
                    Toast.makeText(requireContext(), "Fail to open $uristring with ${e.localizedMessage}", Toast.LENGTH_LONG).show()
                    withContext(Dispatchers.Main) {
                        dismissDialog()
                    }
                    return@launch
                }

                if (apath == "") {
                    Toast.makeText(requireContext(), "Fail to open $uristring", Toast.LENGTH_LONG).show()
                    withContext(Dispatchers.Main) {
                        dismissDialog()
                    }
                    return@launch
                }

                gameinfo = GameInfo.genGameInfoFromCHD(apath)
                if (gameinfo != null) {

                    val bundle = Bundle()
                    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
                    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
                    mFirebaseAnalytics!!.logEvent(
                        "yab_start_game", bundle
                    )
                    mParcelFileDescriptor!!.close()
                } else {
                    Toast.makeText(requireContext(), "Fail to open $apath", Toast.LENGTH_LONG).show()
                    mParcelFileDescriptor?.close()
                    return@launch
                }

                withContext(Dispatchers.Main) {
                    updateDialogString("Installing ${gameinfo.game_title}")
                }

                val fd =
                    File(YabauseStorage.storage.gamePath + "/" + gameinfo.product_number + ".chd")
                var parcelFileDescriptor =
                    requireActivity().contentResolver.openFileDescriptor(uri, "r")
                val fileDescriptor: FileDescriptor = parcelFileDescriptor!!.getFileDescriptor()

                FileInputStream(fileDescriptor).use { inputStream ->
                    FileOutputStream(fd).use { outputStream ->
                        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
                            FileUtils.copy(inputStream, outputStream)
                        } else {
                            TODO("VERSION.SDK_INT < Q")
                        }
                    }
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
            var uri: Uri? = null
            if (data != null) {
                uri = data.data
                if (uri != null) {
                    Log.i(TAG, "Uri: $uri")

                    if (!uri.toString().toLowerCase().endsWith("chd")) {
                        Toast.makeText(requireContext(),
                            getString(R.string.only_chd_is_supported_for_load_game),
                            Toast.LENGTH_LONG).show()
                        return
                    }

                    val cursor: Cursor? = requireActivity().contentResolver.query(uri,
                        null, null, null, null)

                    cursor!!.moveToFirst()
                    var size: Long = cursor.getLong(cursor.getColumnIndex(OpenableColumns.SIZE))
                    cursor.close()

                    size = size / 1024 / 1024
                    val message = getString(R.string.install_game_message) + " " + size + getString(R.string.install_game_message_after)

                    val show = AlertDialog.Builder(requireActivity())
                        .setTitle(getString(R.string.do_you_want_to_install))
                        // .setMessage("If you install, you can play from this game list directly. But it costs ${size} MBytes")
                        .setMessage(message)
                        .setPositiveButton(R.string.yes) { dialog, which -> installGameFile(uri) }
                        .setNegativeButton(R.string.no) { dialog, which -> openGameFileDirect(uri) }
                        .setCancelable(true)
                        .show()
                }
            }
            return
        }

        when (requestCode) {
            GameSelectPresenter.RC_SIGN_IN -> {
                presenter_.onSignIn(resultCode, data)
                if (presenter_.currentUserName != null) {
                    val m = mNavigationView!!.menu
                    val mi_login = m.findItem(R.id.menu_item_login)
                    mi_login.setTitle(R.string.sign_out)
                }
            }
            DOWNLOAD_ACTIVITY -> {
                if (resultCode == 0) {
                    refresh_level = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                }
                if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_UPDATED) {
                    refresh_level = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                } else if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_RESTART) {
                    val intent = Intent(activity, StartupActivity::class.java)
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(intent)
                    activity?.finish()
                } else {
                }
            }
            SETTING_ACTIVITY -> if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_UPDATED) {
                refresh_level = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            } else if (resultCode == GameSelectFragment.GAMELIST_NEED_TO_RESTART) {
                val intent = Intent(activity, StartupActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                startActivity(intent)
                activity?.finish()
            } else {
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
        val apath: String
        if (file == null) { // canceled
            return
        }
        apath = file.absolutePath
        val storage = YabauseStorage.storage
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
            gameinfo.updateState()
            val c = Calendar.getInstance()
            gameinfo.lastplay_date = c.time
            gameinfo.save()
        } else {
            return
        }
        val bundle = Bundle()
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, gameinfo.product_number)
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
        mFirebaseAnalytics!!.logEvent(
            "yab_start_game", bundle
        )
        val intent = Intent(activity, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", apath)
        intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
        startActivityForResult(intent, YABAUSE_ACTIVITY)
    }

    fun showDialog(message: String?) {
        if (message != null) {
            progress_message.text = message
        } else {
            progress_message.text = "Updating..."
        }
        llProgressBar.visibility = View.VISIBLE
    }

    fun updateDialogString(msg: String) {
        progress_message.text = msg
    }

    fun dismissDialog() {
        llProgressBar.visibility = View.GONE
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        val activity = activity as AppCompatActivity
        // Log.i(TAG, "onCreate");
        super.onActivityCreated(savedInstanceState)
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(activity)
        val application = activity.application as YabauseApplication
        mTracker = application.defaultTracker
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
            rootview_.findViewById<View>(R.id.toolbar) as Toolbar
        toolbar.setLogo(R.mipmap.ic_launcher)
        toolbar.title = getString(R.string.app_name)
        toolbar.subtitle = getVersionName(activity)
        activity.setSupportActionBar(toolbar)
        tablayout_ = rootview_.findViewById(R.id.tab_game_index)
        tablayout_.removeAllTabs()

        mDrawerLayout =
            rootview_.findViewById<View>(R.id.drawer_layout_game_select) as DrawerLayout
        mDrawerToggle = object : ActionBarDrawerToggle(
            getActivity(), /* host Activity */
            mDrawerLayout, /* DrawerLayout object */
            R.string.drawer_open, /* "open drawer" description */
            R.string.drawer_close /* "close drawer" description */
        ) {
            /** Called when a drawer has settled in a completely closed state.  */
            override fun onDrawerClosed(view: View) {
                super.onDrawerClosed(view)
                // activity.getSupportActionBar().setTitle("aaa");
            }

            override fun onDrawerStateChanged(newState: Int) {}
            /** Called when a drawer has settled in a completely open state.  */
            override fun onDrawerOpened(drawerView: View) {
                super.onDrawerOpened(drawerView)
                // activity.getSupportActionBar().setTitle("bbb");
                val tx =
                    rootview_!!.findViewById<View>(R.id.menu_title) as TextView
                val uname = presenter_!!.currentUserName
                if (tx != null && uname != null) {
                    tx.text = uname
                } else {
                    tx.text = ""
                }
                val iv =
                    rootview_!!.findViewById<View>(R.id.navi_header_image) as ImageView
                val PhotoUri = presenter_!!.currentUserPhoto
                if (iv != null && PhotoUri != null) {
                    Glide.with(drawerView.context)
                        .load(PhotoUri)
                        .into(iv)
                } else {
                    iv.setImageResource(R.mipmap.ic_launcher)
                }
            }
        }
        // Set the drawer toggle as the DrawerListener
        mDrawerLayout!!.setDrawerListener(mDrawerToggle)
        activity.supportActionBar!!.setDisplayHomeAsUpEnabled(true)
        activity.supportActionBar!!.setHomeButtonEnabled(true)
        mDrawerToggle.syncState()
        mNavigationView =
            rootview_.findViewById<View>(R.id.nav_view) as NavigationView
        if (mNavigationView != null) {
            mNavigationView!!.setNavigationItemSelectedListener(this)
        }

        if (presenter_.currentUserName != null) {
            val m = mNavigationView!!.menu
            val mi_login = m.findItem(R.id.menu_item_login)
            mi_login.setTitle(R.string.sign_out)
        } else {
            val m = mNavigationView!!.menu
            val mi_login = m.findItem(R.id.menu_item_login)
            mi_login.setTitle(R.string.sign_in)
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
        mDrawerToggle.onConfigurationChanged(newConfig)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean { // Pass the event to ActionBarDrawerToggle, if it returns
// true, then it has handled the app icon touch event
        return if (mDrawerToggle.onOptionsItemSelected(item)) {
            true
        } else super.onOptionsItemSelected(
            item
        )
    }

    private var observer: Observer<*>? = null
    fun updateGameList() {
        if (observer != null) return
        isBackGroundComplete = false
        val tmpObserver = object : Observer<String> {
            // GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();
            override fun onSubscribe(d: Disposable) {
                observer = this
                showDialog(null)
            }

            override fun onNext(response: String) {
                updateDialogString("Updating ..." + response)
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
                val viewPager = rootview_.findViewById(R.id.view_pager_game_index) as? ViewPager
                // viewPager!!.setSaveFromParentEnabled(false)
                // viewPager!!.removeAllViews()
                tabpage_adapter.setGameList(gameListPages)
                viewPager!!.adapter = tabpage_adapter
                // tablayout_.removeAllTabs()
                tablayout_.setupWithViewPager(viewPager)

                dismissDialog()
                if (isfisrtupdate) {
                    isfisrtupdate = false
                    if (this@GameSelectFragmentPhone.activity?.intent!!.getBooleanExtra(
                            "showPin",
                            false
                        )) {
                        ShowPinInFragment.newInstance(presenter_).show(
                            childFragmentManager,
                            "sample"
                        )
                    } else {
                        presenter_.checkSignIn()
                    }
                }
                viewPager.adapter!!.notifyDataSetChanged()
                observer = null
            }
        }
        presenter_.updateGameList(refresh_level, tmpObserver)
        refresh_level = 0
    }

    private fun showRestartMessage() { // need_to_accept
        val viewMessage = rootview_.findViewById(R.id.empty_message) as TextView
        val viewPager = rootview_.findViewById(R.id.view_pager_game_index) as? ViewPager
        viewMessage?.visibility = View.VISIBLE
        viewPager?.visibility = View.GONE

        val welcomeMessage = getResources().getString(R.string.need_to_accept)
        viewMessage.text = welcomeMessage
    }

    internal var gameListPages: MutableList<GameListPage>? = null

    public fun getGameItemAdapter(index: String): GameItemAdapter? {

        if (gameListPages == null) {
            loadRows()
        }

        if (gameListPages != null) {
            for (page in this.gameListPages!!) {
                if (page.pageTitle.equals(index)) {
                    return page.gameList
                }
            }
        }
        return null
    }

    private fun loadRows() {

        // -----------------------------------------------------------------
        // Recent Play Game
        try {
            val checklist = Select()
                    .from(GameInfo::class.java)
                    .limit(1)
                    .execute<GameInfo?>()
            if (checklist.size == 0) {

                val viewMessage = rootview_.findViewById(R.id.empty_message) as TextView
                val viewPager = rootview_.findViewById(R.id.view_pager_game_index) as? ViewPager
                viewMessage?.visibility = View.VISIBLE
                viewPager?.visibility = View.GONE

                val markwon = Markwon.create(this.activity as Context)
                val welcomeMessage = getResources().getString(
                    R.string.welcome,
                    YabauseStorage.storage.gamePath
                )
                markwon.setMarkdown(viewMessage, welcomeMessage)

                return
            }
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage)
        }

        val viewMessage = rootview_.findViewById(R.id.empty_message) as? View
        val viewPager = rootview_.findViewById(R.id.view_pager_game_index) as? ViewPager
        viewMessage?.visibility = View.GONE
        viewPager?.visibility = View.VISIBLE

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
            Log.d(TAG, e.localizedMessage)
        }
        val resent_adapter = GameItemAdapter(rlist)

        gameListPages = mutableListOf()

        val resent_page = GameListPage("recent", resent_adapter)
        resent_adapter.setOnItemClickListener(this)
        gameListPages!!.add(resent_page)

        // -----------------------------------------------------------------
        //
        var list: MutableList<GameInfo>? = null
        try {
            list = Select()
                .from(GameInfo::class.java)
                .orderBy("game_title ASC")
                .execute()
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage)
        }
        var hit: Boolean
        var i: Int
        i = 0
        while (i < alphabet.size) {
            hit = false
            val alphaindexed_list: MutableList<GameInfo?>? =
                ArrayList()
            val it = list!!.iterator()
            while (it.hasNext()) {
                val game = it.next()
                if (game.game_title.toUpperCase().indexOf(alphabet[i]) == 0) {
                    alphaindexed_list!!.add(game)
                    Log.d(
                        "GameSelect",
                        alphabet[i] + ":" + game.game_title
                    )
                    it.remove()
                    hit = true
                }
            }
            if (hit) {
                val pageAdapter = GameItemAdapter(alphaindexed_list)
                pageAdapter.setOnItemClickListener(this)
                var a_page = GameListPage(alphabet[i], pageAdapter)
                gameListPages!!.add(a_page)
            }
            i++
        }
        val others_list: MutableList<GameInfo?>? = ArrayList()
        val it: Iterator<GameInfo> = list!!.iterator()
        while (it.hasNext()) {
            val game = it.next()
            Log.d("GameSelect", "Others:" + game.game_title)
            others_list!!.add(game)
        }

        if (others_list!!.size > 0) {
            val otherAdapter = GameItemAdapter(others_list)
            otherAdapter.setOnItemClickListener(this)
            var otherPage = GameListPage("OTHERS", otherAdapter)
            gameListPages!!.add(otherPage)
        }
    }

    override fun onItemClick(position: Int, item: GameInfo?, v: View?) {
        val c = Calendar.getInstance()
        item?.lastplay_date = c.time
        item?.save()
        if (mTracker != null) {
            mTracker!!.send(
                EventBuilder()
                    .setCategory("Action")
                    .setAction(item?.game_title)
                    .build()
            )
        }
        val bundle = Bundle()
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, item?.product_number)
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, item?.game_title)
        mFirebaseAnalytics!!.logEvent(
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
                    tablayout_.removeTabAt(index)
                }

                val removeAll = gameListPages!!.removeAll { it.pageTitle == title.toString() }
                tabpage_adapter.notifyDataSetChanged()
            }
        }

        val othersPage = gameListPages!!.find { it.pageTitle == "OTHERS" }
        if (othersPage != null) {
            othersPage.gameList.removeItem(item.id)
            if (othersPage.gameList.itemCount == 0) {

                val index = gameListPages!!.indexOfFirst { it.pageTitle == "OTHERS" }
                if (index != -1) {
                    tablayout_.removeTabAt(index)
                }

                val removeAll = gameListPages!!.removeAll { it.pageTitle == "OTHERS" }
                tabpage_adapter.notifyDataSetChanged()
            }
        }
    }

    private fun requestNewInterstitial() {
        val adRequest =
            AdRequest.Builder() // .addTestDevice("YOUR_DEVICE_HASH")
                .build()
        // mInterstitialAd!!.loadAd(adRequest)
    }

    private var isBackGroundComplete = false

    override fun onResume() {
        super.onResume()
        if (mTracker != null) { // mTracker.setScreenName(TAG);
            mTracker!!.send(ScreenViewBuilder().build())
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
        private val adapter: RecyclerView.Adapter<*>? = null
        private var recyclerView: RecyclerView? = null
        private val data: ArrayList<GameInfo>? = null
        private var instance: GameSelectFragmentPhone? = null
        @JvmField
        var myOnClickListener: View.OnClickListener? = null
        private const val RC_SIGN_IN = 123
        private const val REQUEST_STORAGE = 1
        private val PERMISSIONS_STORAGE = arrayOf(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
        )

        fun newInstance(param1: String?, param2: String?): GameSelectFragmentPhone {
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
