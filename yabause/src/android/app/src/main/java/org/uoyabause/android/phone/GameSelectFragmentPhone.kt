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
import android.app.Fragment
import android.app.ProgressDialog
import android.app.UiModeManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.preference.PreferenceManager
import android.util.Log
import android.view.LayoutInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.core.app.ActivityCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.recyclerview.widget.DefaultItemAnimator
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.activeandroid.query.Select
import com.bumptech.glide.Glide
import com.google.android.gms.ads.AdListener
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.InterstitialAd
import com.google.android.gms.ads.MobileAds
import com.google.android.gms.analytics.HitBuilders.EventBuilder
import com.google.android.gms.analytics.HitBuilders.ScreenViewBuilder
import com.google.android.gms.analytics.Tracker
import com.google.android.material.navigation.NavigationView
import com.google.android.material.snackbar.Snackbar
import com.google.firebase.analytics.FirebaseAnalytics
import io.reactivex.Observer
import io.reactivex.disposables.Disposable
import net.cattaka.android.adaptertoolbox.thirdparty.MergeRecyclerAdapter
import org.uoyabause.android.AdActivity
import org.uoyabause.android.FileDialog
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.GameInfo
import org.uoyabause.android.GameSelectPresenter
import org.uoyabause.android.GameSelectPresenter.GameSelectPresenterListener
import org.uoyabause.android.Yabause
import org.uoyabause.android.YabauseApplication
import org.uoyabause.android.YabauseSettings
import org.uoyabause.android.YabauseStorage
import org.uoyabause.android.tv.GameSelectFragment
import org.uoyabause.uranus.BuildConfig
import org.uoyabause.uranus.R
import org.uoyabause.uranus.StartupActivity
import java.io.File
import java.util.*
import kotlin.Any as Any1

class GameSelectFragmentPhone : Fragment(),
    GameItemAdapter.OnItemClickListener, NavigationView.OnNavigationItemSelectedListener,
    FileSelectedListener, GameSelectPresenterListener {
    var refresh_level = 0
    var mStringsHeaderAdapter: GameIndexAdapter? = null
    lateinit var presenter_: GameSelectPresenter
    val SETTING_ACTIVITY = 0x01
    val YABAUSE_ACTIVITY = 0x02
    val DOWNLOAD_ACTIVITY = 0x03
    private var mDrawerLayout: DrawerLayout? = null
    private var mInterstitialAd: InterstitialAd? = null
    private var mTracker: Tracker? = null
    private var mFirebaseAnalytics: FirebaseAnalytics? = null
    var isfisrtupdate = true
    var mNavigationView: NavigationView? = null

    lateinit var rootview_: View
    lateinit var mDrawerToggle: ActionBarDrawerToggle
    private var mProgressDialog: ProgressDialog? = null
    lateinit var mMergeRecyclerAdapter: MergeRecyclerAdapter<RecyclerView.Adapter<*>?>
    lateinit var layoutManager: RecyclerView.LayoutManager


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
        presenter_ = GameSelectPresenter(this as Fragment, this)
        if (arguments != null) {
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        rootview_ = inflater.inflate(R.layout.fragment_game_select_fragment_phone, container, false)
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

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        mDrawerLayout!!.closeDrawers()
        //if( item.getTitle().equals(getResources().getString(R.string.donation)) == true ){
//    Intent intent = new Intent(getActivity(), DonateActivity.class);
//    startActivity(intent);
//    return false;
//}
        val id = item.itemId
        when (id) {
            R.id.menu_item_setting -> {
                val intent = Intent(activity, YabauseSettings::class.java)
                startActivityForResult(intent, SETTING_ACTIVITY)
            }
            R.id.menu_item_load_game -> {
                val yabroot =
                    File(Environment.getExternalStorageDirectory(), "yabause")
                val sharedPref =
                    PreferenceManager.getDefaultSharedPreferences(activity)
                val last_dir =
                    sharedPref.getString("pref_last_dir", yabroot.path)
                val fd =
                    FileDialog(activity, last_dir)
                fd.addFileListener(this)
                fd.showDialog()
            }
            R.id.menu_item_update_game_db -> {
                refresh_level = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            }
            R.id.menu_item_login -> if (item.title == getString(R.string.sign_out) == true) {
                presenter_!!.signOut()
                item.setTitle(R.string.sign_in)
            } else {
                presenter_!!.signIn()
            }
            R.id.menu_privacy_policy -> {
                val uri =
                    Uri.parse("http://www.uoyabause.org/static_pages/privacy_policy.html")
                val i = Intent(Intent.ACTION_VIEW, uri)
                startActivity(i)
            }
        }
        return false
    }

    fun checkStoragePermission(): Int {
        if (Build.VERSION.SDK_INT >= 23) { // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(
                    activity,
                    Manifest.permission.READ_EXTERNAL_STORAGE
                )
                != PackageManager.PERMISSION_GRANTED
                || ActivityCompat.checkSelfPermission(
                    activity,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                )
                != PackageManager.PERMISSION_GRANTED
            ) { // Contacts permissions have not been granted.
                Log.i(
                    TAG,
                    "Storage permissions has NOT been granted. Requesting permissions."
                )
                //if (shouldShowRequestPermissionRationale(Manifest.permission.READ_EXTERNAL_STORAGE)
//        || shouldShowRequestPermissionRationale(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
//
//} else {
                requestPermissions(
                    PERMISSIONS_STORAGE,
                    REQUEST_STORAGE
                )
                //}
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
        requestCode: Int, permissions: Array<String>,
        grantResults: IntArray
    ) {
        if (requestCode == REQUEST_STORAGE) {
            Log.i(TAG, "Received response for contact permissions request.")
            // We have requested multiple permissions for contacts, so all of them need to be
// checked.
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
            .make(rootview_!!.rootView, getString(id), Snackbar.LENGTH_SHORT)
            .show()
    }

    override fun onActivityResult(
        requestCode: Int,
        resultCode: Int,
        data: Intent
    ) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            GameSelectPresenter.RC_SIGN_IN -> {
                presenter_!!.onSignIn(resultCode, data)
                if (presenter_!!.currentUserName != null) {
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
                    activity.finish()
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
                activity.finish()
            } else {
            }
            YABAUSE_ACTIVITY -> if (BuildConfig.BUILD_TYPE != "pro") {
                val prefs = activity.getSharedPreferences(
                    "private",
                    Context.MODE_PRIVATE
                )
                val hasDonated = prefs.getBoolean("donated", false)
                if (hasDonated == false) {
                    val rn = Math.random()
                    if (rn <= 0.5) {
                        val uiModeManager =
                            activity.getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
                        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
                            if (mInterstitialAd!!.isLoaded) {
                                mInterstitialAd!!.show()
                            } else {
                                val intent = Intent(
                                    activity,
                                    AdActivity::class.java
                                )
                                startActivity(intent)
                            }
                        } else {
                            val intent =
                                Intent(activity, AdActivity::class.java)
                            startActivity(intent)
                        }
                    } else if (rn > 0.5) {
                        val intent =
                            Intent(activity, AdActivity::class.java)
                        startActivity(intent)
                    }
                }
            }
            else -> {
            }
        }
    }

    override fun fileSelected(file: File) {
        val apath: String
        if (file == null) { // canceled
            return
        }
        apath = file.absolutePath
        val storage = YabauseStorage.getStorage()
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
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY")
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title)
        mFirebaseAnalytics!!.logEvent(
            FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        )
        val intent = Intent(activity, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", apath)
        intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number)
        startActivity(intent)
    }

    fun showDialog() {
        if (mProgressDialog == null) {
            mProgressDialog = ProgressDialog(activity)
            mProgressDialog!!.setMessage("Updating...")
            mProgressDialog!!.show()
        }
    }

    fun updateDialogString(msg: String) {
        if (mProgressDialog != null) {
            mProgressDialog!!.setMessage("Updating... $msg")
        }
    }

    fun dismissDialog() {
        if (mProgressDialog != null) {
            if (mProgressDialog!!.isShowing) {
                mProgressDialog!!.dismiss()
            }
            mProgressDialog = null
        }
    }

    /*
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState){
        v_ = super.onCreateView(inflater,container,savedInstanceState);

        UiModeManager uiModeManager = (UiModeManager) getActivity().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
        }
        return v_;
    }
*/
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        val activity = activity as AppCompatActivity
        //Log.i(TAG, "onCreate");
        super.onActivityCreated(savedInstanceState)
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(getActivity())
        val application = getActivity().application as YabauseApplication
        mTracker = application.defaultTracker
        MobileAds.initialize(getActivity().applicationContext, getString(R.string.ad_app_id))
        mInterstitialAd = InterstitialAd(getActivity())
        mInterstitialAd!!.adUnitId = getString(R.string.banner_ad_unit_id)
        requestNewInterstitial()
        mInterstitialAd!!.adListener = object : AdListener() {
            override fun onAdClosed() {
                requestNewInterstitial()
            }
        }
        val toolbar =
            rootview_!!.findViewById<View>(R.id.toolbar) as Toolbar
        toolbar.setLogo(R.mipmap.ic_launcher)
        toolbar.title = getString(R.string.app_name)
        toolbar.subtitle = getVersionName(getActivity())
        activity.setSupportActionBar(toolbar)
        //activity.getSupportActionBar().setTitle(getString(R.string.app_name) + getVersionName(getActivity()));
        recyclerView =
            rootview_!!.findViewById<View>(R.id.my_recycler_view) as RecyclerView
        recyclerView!!.setHasFixedSize(true)
        layoutManager = LinearLayoutManager(getActivity())
        recyclerView!!.layoutManager = layoutManager
        recyclerView!!.itemAnimator = DefaultItemAnimator()
        mDrawerLayout =
            rootview_!!.findViewById<View>(R.id.drawer_layout_game_select) as DrawerLayout
        mDrawerToggle = object : ActionBarDrawerToggle(
            getActivity(),  /* host Activity */
            mDrawerLayout,  /* DrawerLayout object */
            R.string.drawer_open,  /* "open drawer" description */
            R.string.drawer_close /* "close drawer" description */
        ) {
            /** Called when a drawer has settled in a completely closed state.  */
            override fun onDrawerClosed(view: View) {
                super.onDrawerClosed(view)
                //activity.getSupportActionBar().setTitle("aaa");
            }

            override fun onDrawerStateChanged(newState: Int) {}
            /** Called when a drawer has settled in a completely open state.  */
            override fun onDrawerOpened(drawerView: View) {
                super.onDrawerOpened(drawerView)
                //activity.getSupportActionBar().setTitle("bbb");
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
            rootview_!!.findViewById<View>(R.id.nav_view) as NavigationView
        if (mNavigationView != null) {
            mNavigationView!!.setNavigationItemSelectedListener(this)
        }
        //SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
//Boolean hasDonated = prefs.getBoolean("donated", false);
//if( !hasDonated) {
//    Menu m = mNavigationView.getMenu();
//    m.add(getResources().getString(R.string.donation));
//}
        if (presenter_!!.currentUserName != null) {
            val m = mNavigationView!!.menu
            val mi_login = m.findItem(R.id.menu_item_login)
            mi_login.setTitle(R.string.sign_out)
        } else {
            val m = mNavigationView!!.menu
            val mi_login = m.findItem(R.id.menu_item_login)
            mi_login.setTitle(R.string.sign_in)
        }
        //updateGameList();
        if (checkStoragePermission() == 0) {
            updateGameList()
        }
    }

    //@Override
//protected void onPostCreate(Bundle savedInstanceState) {
//    super.onPostCreate(savedInstanceState);
//    // Sync the toggle state after onRestoreInstanceState has occurred.
//    mDrawerToggle.syncState();
//}
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        mDrawerToggle!!.onConfigurationChanged(newConfig)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean { // Pass the event to ActionBarDrawerToggle, if it returns
// true, then it has handled the app icon touch event
        return if (mDrawerToggle!!.onOptionsItemSelected(item)) {
            true
        } else super.onOptionsItemSelected(
            item
        )
    }

    private var observer: Observer<*>? = null
    fun updateGameList() {
        observer = object : Observer<String> {
            //GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();
            override fun onSubscribe(d: Disposable) {
                showDialog()
            }

            override fun onNext(response: String) {
                updateDialogString(response)
            }

            override fun onError(e: Throwable) {
                dismissDialog()
            }

            override fun onComplete() {
                loadRows()
                dismissDialog()
                if (isfisrtupdate) {
                    isfisrtupdate = false
                    presenter_!!.checkSignIn()
                }
            }
        }
        presenter_!!.updateGameList(refresh_level, observer)
        refresh_level = 0
    }

    private fun showRestartMessage() { //need_to_accept
        recyclerView!!.layoutManager = LinearLayoutManager(
            activity,
            LinearLayoutManager.VERTICAL,
            false
        )
        mMergeRecyclerAdapter = MergeRecyclerAdapter<RecyclerView.Adapter<*>?>(activity)
        mStringsHeaderAdapter = GameIndexAdapter(getString(R.string.need_to_accept))
        mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter)
        recyclerView!!.adapter = mMergeRecyclerAdapter
    }

    private fun loadRows() {
        recyclerView!!.layoutManager = LinearLayoutManager(
            activity,
            LinearLayoutManager.VERTICAL,
            false
        )
        mMergeRecyclerAdapter = MergeRecyclerAdapter<RecyclerView.Adapter<*>?>(activity)
        //-----------------------------------------------------------------
// Recent Play Game
        var cheklist: List<GameInfo?>? = null
        try {
            cheklist = Select()
                .from(GameInfo::class.java)
                .limit(1)
                .execute<GameInfo?>()
        } catch (e: Exception) {
            println(e)
        }
        if (cheklist!!.size == 0) {
            val dir = YabauseStorage.getStorage().gamePath
            mStringsHeaderAdapter =
                GameIndexAdapter("Place a game ISO image file to $dir or select directory in the setting menu")
            mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter)
            recyclerView!!.adapter = mMergeRecyclerAdapter
            return
        }
        run {
            // create strings header adapter
            mStringsHeaderAdapter = GameIndexAdapter("RECENT")
            mMergeRecyclerAdapter!!.addAdapter(mStringsHeaderAdapter)
        }
        //-----------------------------------------------------------------
// Recent Play Game
        var rlist: List<GameInfo?>? = null
        try {
            rlist = Select()
                .from(GameInfo::class.java)
                .orderBy("lastplay_date DESC")
                .limit(5)
                .execute<GameInfo?>()
        } catch (e: Exception) {
            println(e)
        }
        val resent_adapter = GameItemAdapter(rlist)
        resent_adapter.setOnItemClickListener(this)
        mMergeRecyclerAdapter.addAdapter(resent_adapter)
        //-----------------------------------------------------------------
//
        var list: MutableList<GameInfo>? = null
        try {
            list = Select()
                .from(GameInfo::class.java)
                .orderBy("game_title ASC")
                .execute()
        } catch (e: Exception) {
            println(e)
        }
        var hit: Boolean
        var i: Int
        i = 0
        while (i < alphabet.size) {
            hit = false
            val alphaindexed_list: MutableList<GameInfo> =
                ArrayList()
            val it = list!!.iterator()
            while (it.hasNext()) {
                val game = it.next()
                if (game.game_title.toUpperCase().indexOf(alphabet[i]) == 0) {
                    alphaindexed_list.add(game)
                    Log.d(
                        "GameSelect",
                        alphabet[i] + ":" + game.game_title
                    )
                    it.remove()
                    hit = true
                }
            }
            if (hit) {
                mStringsHeaderAdapter = GameIndexAdapter(alphabet[i])
                mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter)
                val alphabet_adapter = GameItemAdapter(alphaindexed_list)
                alphabet_adapter.setOnItemClickListener(this)
                mMergeRecyclerAdapter.addAdapter(alphabet_adapter)
            }
            i++
        }
        val others_list: MutableList<GameInfo> = ArrayList()
        val it: Iterator<GameInfo> = list!!.iterator()
        while (it.hasNext()) {
            val game = it.next()
            Log.d("GameSelect", "Others:" + game.game_title)
            others_list.add(game)
        }
        mStringsHeaderAdapter = GameIndexAdapter("OTHERS")
        mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter)
        val others_adapter = GameItemAdapter(others_list)
        others_adapter.setOnItemClickListener(this)
        mMergeRecyclerAdapter.addAdapter(others_adapter)
        recyclerView!!.adapter = mMergeRecyclerAdapter
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
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY")
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, item?.game_title)
        mFirebaseAnalytics!!.logEvent(
            FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        )
        val intent = Intent(activity, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", item?.file_path)
        intent.putExtra("org.uoyabause.android.gamecode", item?.product_number)
        startActivityForResult(intent, YABAUSE_ACTIVITY)
    }

    private fun requestNewInterstitial() {
        val adRequest =
            AdRequest.Builder() //.addTestDevice("YOUR_DEVICE_HASH")
                .build()
        mInterstitialAd!!.loadAd(adRequest)
    }

    override fun onResume() {
        super.onResume()
        if (mTracker != null) { //mTracker.setScreenName(TAG);
            mTracker!!.send(ScreenViewBuilder().build())
        }
    }

    override fun onPause() {
        dismissDialog()
        super.onPause()
    }

    override fun onDestroy() {
        System.gc()
        super.onDestroy()
    }

    //@Override
//public void onUpdateGameList( ){
//    loadRows();
//    dismissDialog();
//    if(isfisrtupdate) {
//        isfisrtupdate = false;
//        presenter_.checkSignIn();
//    }
//}
    override fun onShowMessage(string_id: Int) {
        showSnackbar(string_id)
    }

    companion object {
        private val adapter: RecyclerView.Adapter<*>? = null
        private var recyclerView: RecyclerView? = null
        private val data: ArrayList<GameInfo>? = null
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