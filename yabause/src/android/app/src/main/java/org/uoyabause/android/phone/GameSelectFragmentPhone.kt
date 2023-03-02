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
import android.graphics.BitmapFactory
import android.graphics.drawable.Icon
import android.net.Uri
import android.os.Build
import android.os.Build.VERSION_CODES
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.MenuItem
import android.view.View
import android.view.View.VISIBLE
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentStatePagerAdapter
import androidx.multidex.MultiDexApplication
import androidx.preference.PreferenceManager
import androidx.viewpager.widget.ViewPager
import com.activeandroid.query.Select
import com.bumptech.glide.Glide
import com.bumptech.glide.load.resource.bitmap.TransformationUtils.centerCrop
import com.google.android.gms.analytics.HitBuilders.ScreenViewBuilder
import com.google.android.gms.analytics.Tracker
import com.google.android.material.navigation.NavigationView
import com.google.android.material.snackbar.Snackbar
import com.google.android.material.tabs.TabLayout
import com.google.android.play.core.review.ReviewManager
import com.google.android.play.core.review.ReviewManagerFactory
import com.google.android.play.core.review.testing.FakeReviewManager
import com.google.firebase.analytics.FirebaseAnalytics
import com.google.firebase.analytics.ktx.logEvent
import io.noties.markwon.Markwon
import io.reactivex.Observer
import io.reactivex.disposables.Disposable
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.devmiyax.yabasanshiro.StartupActivity
import org.uoyabause.android.*
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.GameSelectPresenter.GameSelectPresenterListener
import org.uoyabause.android.tv.GameSelectFragment
import java.io.File
import java.util.*


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

    override fun getPageTitle(position: Int): CharSequence {
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
    private var drawerLayout: DrawerLayout? = null
    private var tracker: Tracker? = null
    private var firebaseAnalytics: FirebaseAnalytics? = null
    private var isFirstUpdate = true
    private var navigationView: NavigationView? = null
    private lateinit var tabPageAdapter: GameViewPagerAdapter

    private lateinit var rootView: View
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
        presenter = GameSelectPresenter(this as Fragment, yabauseActivityLauncher,this)
        tabPageAdapter = GameViewPagerAdapter(this@GameSelectFragmentPhone.childFragmentManager)
    }

    private var readRequestLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if ( result.resultCode == Activity.RESULT_OK) {
            if (result.data != null) {
                val uri = result.data!!.data
                if (uri != null) {
                    presenter.onSelectFile(uri)
                }
            }
        }
    }

    private fun selectGameFile(){
        val prefs = requireActivity().getSharedPreferences("private",
            MultiDexApplication.MODE_PRIVATE)
        val installCount = prefs.getInt("InstallCount", 3)
        if( installCount > 0){
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
            intent.addCategory(Intent.CATEGORY_OPENABLE)
            intent.type = "*/*"
            readRequestLauncher.launch(intent)
        }else {
            val message = resources.getString(org.devmiyax.yabasanshiro.R.string.or_place_file_to, YabauseStorage.storage.gamePath)
            val rtn = YabauseApplication.checkDonated(requireActivity(), message)
            if ( rtn == 0) {
                val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                intent.addCategory(Intent.CATEGORY_OPENABLE)
                intent.type = "*/*"
                readRequestLauncher.launch(intent)
            }
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View {
        rootView = inflater.inflate(org.devmiyax.yabasanshiro.R.layout.fragment_game_select_fragment_phone, container, false)
        progressBar = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.llProgressBar)
        progressBar.visibility = View.GONE
        progressMessage = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.pbText)

        val fab: View = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.fab)
        if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
            fab.setOnClickListener {
                selectGameFile()
            }
        } else {
            fab.visibility = View.GONE
        }

        if( adHeight != 0 ) {
            onAdViewIsShown(adHeight)
        }
        return rootView
    }

    private var adHeight = 0
    fun onAdViewIsShown(height: Int) {
        try {
            val parentLayout = rootView.findViewById<DrawerLayout>(org.devmiyax.yabasanshiro.R.id.drawer_layout_game_select)
            val param = parentLayout.layoutParams as FrameLayout.LayoutParams
            param.bottomMargin = height + 4
            parentLayout.layoutParams = param
        } catch (e: Exception) {
            adHeight = height
        }
    }


    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        drawerLayout!!.closeDrawers()
        when (item.itemId) {
            org.devmiyax.yabasanshiro.R.id.menu_auto_backupsync -> {

                val fragment = BackupBackupItemFragment.newInstance(1,presenter)
                val transaction = requireActivity().supportFragmentManager.beginTransaction()
                transaction.replace(org.devmiyax.yabasanshiro.R.id.ext_fragment, fragment)
                transaction.addToBackStack(null)
                transaction.commit()

            }
            org.devmiyax.yabasanshiro.R.id.menu_item_setting -> {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_item_setting")
                }

                val intent = Intent(activity, SettingsActivity::class.java)
                settingActivityLauncher.launch(intent)
            }
            org.devmiyax.yabasanshiro.R.id.menu_item_load_game -> {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_item_load_game")
                }

                if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
                    selectGameFile()
                } else {
                    val sharedPref =
                        PreferenceManager.getDefaultSharedPreferences(activity)
                    val lastDir =
                        sharedPref.getString("pref_last_dir", YabauseStorage.storage.gamePath)
                    val fd =
                        FileDialog(requireActivity(), lastDir)
                    fd.addFileListener(this)
                    fd.showDialog()
                }
            }
            org.devmiyax.yabasanshiro.R.id.menu_item_update_game_db -> {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_item_update_game_db")
                }

                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
            }
            org.devmiyax.yabasanshiro.R.id.menu_item_login -> if (item.title == getString(org.devmiyax.yabasanshiro.R.string.sign_out)) {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_item_login")
                }

                presenter.signOut()
                item.setTitle(org.devmiyax.yabasanshiro.R.string.sign_in)
            } else {
                presenter.signIn(signInActivityLauncher)
            }
            org.devmiyax.yabasanshiro.R.id.menu_privacy_policy -> {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_privacy_policy")
                }

                val uri =
                    Uri.parse("https://www.uoyabause.org/static_pages/privacy_policy.html")
                val i = Intent(Intent.ACTION_VIEW, uri)
                startActivity(i)
            }
            org.devmiyax.yabasanshiro.R.id.menu_item_login_to_other -> {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "menu_item_login_to_other")
                }

                ShowPinInFragment.newInstance(presenter).show(childFragmentManager, "sample")
            }
        }
        return false
    }

    private fun checkStoragePermission(): Int {
        if ( Build.VERSION.SDK_INT < VERSION_CODES.Q ) { // Verify that all required contact permissions have been granted.
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
                requestStoragePermission.launch(PERMISSIONS_STORAGE)
                return -1
            }
        }
        return 0
    }

    private val requestStoragePermission = registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { result ->
        result.entries.forEach{
            if(!it.value){
                showRestartMessage()
                return@registerForActivityResult
            }
        }
        updateGameList(0)
    }

    private fun showSnackBar(id: Int) {
        Snackbar
            .make(rootView.rootView, getString(id), Snackbar.LENGTH_SHORT)
            .show()
    }

    private fun updateRecent() {
        gameListPages?.forEach { it ->
            if (it.pageTitle == "recent") {
                var resentList: MutableList<GameInfo?>? = null
                try {
                    resentList = Select()
                        .from(GameInfo::class.java)
                        .orderBy("lastplay_date DESC")
                        .limit(5)
                        .execute<GameInfo?>()
                } catch (e: Exception) {
                    Log.d(TAG, e.localizedMessage!!)
                }
                val resentAdapter = GameItemAdapter(resentList)
                resentAdapter.setOnItemClickListener(this)
                it.gameList = resentAdapter
            }
        }
        tabPageAdapter.setGameList(gameListPages)
        tabPageAdapter.notifyDataSetChanged()
    }

    private var adActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
        updateRecent()
    }

    private var settingActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == GameSelectFragment.GAMELIST_NEED_TO_UPDATED) {
            if (checkStoragePermission() == 0) {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "GAMELIST_NEED_TO_UPDATED")
                }

                updateGameList(3)
            }
        }else if (result.resultCode == GameSelectFragment.GAMELIST_NEED_TO_RESTART) {

            firebaseAnalytics?.logEvent("Game Select Fragment"){
                param("event", "GAMELIST_NEED_TO_RESTART")
            }

            val intent = Intent(activity, StartupActivity::class.java)
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(intent)
            activity?.finish()
        }
    }

    private var signInActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->

        firebaseAnalytics?.logEvent("Game Select Fragment"){
            param("event", "onSignIn")
        }

        presenter.onSignIn(result.resultCode, result.data)
        if (presenter.currentUserName != null) {
            val m = navigationView!!.menu
            val miLogin = m.findItem(org.devmiyax.yabasanshiro.R.id.menu_item_login)
            miLogin.setTitle(org.devmiyax.yabasanshiro.R.string.sign_out)
        }
    }

    private var yabauseActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {

        val playtime = it.data?.getLongExtra("playTime",0) ?: 0L

        Log.d(TAG, "Play time is ${playtime}")

        firebaseAnalytics?.logEvent("Game Select Fragment"){
            param("event", "On Game Finished")
            param("playTime",playtime)
        }
        val prefs = requireActivity().getSharedPreferences(
            "private",
            Context.MODE_PRIVATE
        )
        val hasDonated = prefs?.getBoolean("donated", false)

        if (BuildConfig.BUILD_TYPE != "pro" && hasDonated == false ) {

                val rn = Math.random()
                if (rn <= 0.3) {
                    val uiModeManager =
                        activity?.getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
                    if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
                        val intent = Intent(
                            activity,
                            AdActivity::class.java
                        )
                        adActivityLauncher.launch(intent)
                        // }
                    } else {
                        val intent =
                            Intent(activity, AdActivity::class.java)
                        adActivityLauncher.launch(intent)
                    }
                } else if (rn <= 0.6) {
                    val intent =
                        Intent(activity, AdActivity::class.java)
                    adActivityLauncher.launch(intent)
                } else {

                    val lastReviewDateTime = prefs.getInt("last_review_date_time",0)
                    val unixTime = System.currentTimeMillis() / 1000L

                    // ３ヶ月に一度レビューしてもらう
                    if( (unixTime - lastReviewDateTime) > 60*60*24*30 ) {

                        // 5分以上遊んだ？
                        if( playtime < 5*60 ) return@registerForActivityResult

                        var manager : ReviewManager? = null
                        if( BuildConfig.DEBUG ){
                            manager = FakeReviewManager(requireContext())
                        }else{
                            val editor = prefs.edit()
                            editor.putInt("last_review_date_time",lastReviewDateTime)
                            editor.commit()
                            manager = ReviewManagerFactory.create(requireContext())
                        }
                        val request = manager.requestReviewFlow()
                        request.addOnCompleteListener { task ->
                            if (task.isSuccessful) {
                                // We got the ReviewInfo object
                                val reviewInfo = task.result
                                val flow = manager?.launchReviewFlow(requireActivity(), reviewInfo)
                                flow?.addOnCompleteListener { _ ->

                                }
                            } else {
                                task.getException()?.message?.let {
                                        it1 -> Log.d( TAG, it1)
                                }
                            }
                        }

                    }else{
                        val intent =
                            Intent(activity, AdActivity::class.java)
                        adActivityLauncher.launch(intent)
                    }
                }

            updateRecent()

        } else {

            val rn = Math.random()
            val lastReviewDateTime = prefs.getInt("last_review_date_time",0)
            val unixTime = System.currentTimeMillis() / 1000L

            // ３ヶ月に一度レビューしてもらう
            if( rn < 0.3 && (unixTime - lastReviewDateTime) > 60*60*24*30 ){

                // 5分以上遊んだ？
                if( playtime < 5*60 ) return@registerForActivityResult

                var manager : ReviewManager? = null
                if( BuildConfig.DEBUG ){
                    manager = FakeReviewManager(requireContext())
                }else{
                    val editor = prefs.edit()
                    editor.putInt("last_review_date_time",lastReviewDateTime)
                    editor.commit()
                    manager = ReviewManagerFactory.create(requireContext())
                }
                val request = manager.requestReviewFlow()
                request.addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        // We got the ReviewInfo object
                        val reviewInfo = task.result
                        val flow = manager.launchReviewFlow(requireActivity(), reviewInfo)
                        flow.addOnCompleteListener { _ ->

                        }
                    } else {
                        task.getException()?.message?.let {
                                it1 -> Log.d( TAG, it1)
                        }
                    }
                }
            }
            updateRecent()
        }
    }

    override fun fileSelected(file: File?) {

        firebaseAnalytics?.logEvent("Game Select Fragment"){
            param("event", "fileSelected")
        }

        if( file != null ) {
            presenter.fileSelected(file)
        }
    }

    fun showDialog(message: String?) {
        if (message != null) {
            progressMessage.text = message
        } else {
            progressMessage.text = getString(org.devmiyax.yabasanshiro.R.string.updating)
        }
        progressBar.visibility = VISIBLE
    }

    fun updateDialogString(msg: String) {
        progressMessage.text = msg
    }

    fun dismissDialog() {
        progressBar.visibility = View.GONE
    }


    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view,savedInstanceState)
        val activity = requireActivity() as AppCompatActivity
        firebaseAnalytics = FirebaseAnalytics.getInstance(activity)
        val application = activity.application as YabauseApplication
        tracker = application.defaultTracker
        val toolbar =
            rootView.findViewById<View>(org.devmiyax.yabasanshiro.R.id.toolbar) as Toolbar
        toolbar.setLogo(org.devmiyax.yabasanshiro.R.mipmap.ic_launcher)
        toolbar.title = getString(org.devmiyax.yabasanshiro.R.string.app_name)
        toolbar.subtitle = getVersionName(activity)
        activity.setSupportActionBar(toolbar)
        tabLayout = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.tab_game_index)
        tabLayout.removeAllTabs()

        drawerLayout =
            rootView.findViewById<View>(org.devmiyax.yabasanshiro.R.id.drawer_layout_game_select) as DrawerLayout




        drawerToggle = object : ActionBarDrawerToggle(
            getActivity(), /* host Activity */
            drawerLayout, /* DrawerLayout object */
            org.devmiyax.yabasanshiro.R.string.drawer_open, /* "open drawer" description */
            org.devmiyax.yabasanshiro.R.string.drawer_close /* "close drawer" description */
        ) {

/*
            override fun onDrawerOpened(drawerView: View) {
                super.onDrawerOpened(drawerView)
                // activity.getSupportActionBar().setTitle("bbb");
                val tx = rootView.findViewById<TextView?>(org.devmiyax.yabasanshiro.R.id.menu_title)
                val uname = presenter.currentUserName
                if (tx != null && uname != null) {
                    tx.text = uname
                } else {
                    tx.text = ""
                }
                val iv =
                    rootView.findViewById<ImageView?>(org.devmiyax.yabasanshiro.R.id.navi_header_image)
                val uri = presenter.currentUserPhoto
                if (iv != null && uri != null) {
                    Glide.with(drawerView.context)
                        .load(uri)
                        .into(iv)
                } else {
                    iv.setImageResource(org.devmiyax.yabasanshiro.R.mipmap.ic_launcher)
                }
            }

 */
        }
        // Set the drawer toggle as the DrawerListener
        drawerLayout!!.addDrawerListener(drawerToggle)
        activity.supportActionBar!!.setDisplayHomeAsUpEnabled(true)
        activity.supportActionBar!!.setHomeButtonEnabled(true)
        drawerToggle.syncState()
        navigationView =
            rootView.findViewById<View>(org.devmiyax.yabasanshiro.R.id.nav_view) as NavigationView
        if (navigationView != null) {
            navigationView!!.setNavigationItemSelectedListener(this)

            val headerView = navigationView!!.getHeaderView(0)
            val drawerView = headerView!!.findViewById<ImageView>(org.devmiyax.yabasanshiro.R.id.navi_header_image)
            val uri = presenter.currentUserPhoto

            if( uri != null ) {
                val icon: Icon = Icon.createWithResource(
                    requireContext(),
                    org.devmiyax.yabasanshiro.R.mipmap.ic_launcher
                )
                val drawable = icon.loadDrawable(context)
                if (drawable != null) {
                    val width = drawable.intrinsicWidth
                    val height = drawable.intrinsicHeight

                    Glide.with(requireActivity())
                        .load(uri)
                        .override(width, height) // 幅500px、高さ500pxにリサイズ
                        .centerCrop() // センタークロップ
                        .into(drawerView)
                }
            }else{
                drawerView.setImageResource(org.devmiyax.yabasanshiro.R.mipmap.ic_launcher)
            }

            val tx = headerView.findViewById<TextView?>(org.devmiyax.yabasanshiro.R.id.menu_title)
            val uname = presenter.currentUserName
            if (tx != null && uname != null) {
                tx.text = uname
            } else {
                tx.text = ""
            }

        }



        if (presenter.currentUserName != null) {
            val m = navigationView!!.menu
            val miLogin = m.findItem(org.devmiyax.yabasanshiro.R.id.menu_item_login)
            miLogin.setTitle(org.devmiyax.yabasanshiro.R.string.sign_out)
        } else {
            val m = navigationView!!.menu
            val miLogin = m.findItem(org.devmiyax.yabasanshiro.R.id.menu_item_login)
            miLogin.setTitle(org.devmiyax.yabasanshiro.R.string.sign_in)
        }
        if (checkStoragePermission() == 0) {
            updateGameList()
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        drawerToggle.onConfigurationChanged(newConfig)
        tabPageAdapter.setGameList(gameListPages)
        tabPageAdapter.notifyDataSetChanged()
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
                updateDialogString("${getString(org.devmiyax.yabasanshiro.R.string.updating)} $response")
            }

            override fun onError(e: Throwable) {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "updateGameList onError")
                }

                observer = null
                dismissDialog()
                presenter.syncBackup()
            }

            override fun onComplete() {

                firebaseAnalytics?.logEvent("Game Select Fragment"){
                    param("event", "updateGameList onComplete")
                }

                if (!isFront) {
                    observer = null
                    dismissDialog()
                    isBackGroundComplete = true
                    return
                }

                loadRows()
                val viewPager = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.view_pager_game_index) as? ViewPager
                tabPageAdapter.setGameList(gameListPages)
                viewPager!!.adapter = tabPageAdapter
                tabLayout.setupWithViewPager(viewPager)

                dismissDialog()
                if (isFirstUpdate) {
                    isFirstUpdate = false
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
                        presenter.checkSignIn(signInActivityLauncher)
                    }
                }
                viewPager.adapter!!.notifyDataSetChanged()
                observer = null
                presenter.syncBackup()
            }
        }
        presenter.updateGameList(refreshLevel, tmpObserver)
        refreshLevel = 0
    }

    private fun showRestartMessage() { // need_to_accept
        val viewMessage = rootView.findViewById<TextView?>(org.devmiyax.yabasanshiro.R.id.empty_message)
        val viewPager = rootView.findViewById<ViewPager?>(org.devmiyax.yabasanshiro.R.id.view_pager_game_index)
        viewMessage?.visibility = VISIBLE
        viewPager?.visibility = View.GONE

        val welcomeMessage = resources.getString(org.devmiyax.yabasanshiro.R.string.need_to_accept)
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

                val viewMessage = rootView.findViewById<TextView?>(org.devmiyax.yabasanshiro.R.id.empty_message)
                val viewPager = rootView.findViewById<ViewPager?>(org.devmiyax.yabasanshiro.R.id.view_pager_game_index)
                viewMessage!!.visibility = VISIBLE
                viewPager!!.visibility = View.GONE

                val markwon = Markwon.create(this.activity as Context)

                if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
                    val packageName = requireActivity().packageName
                    val welcomeMessage = resources.getString(
                        org.devmiyax.yabasanshiro.R.string.welcome_11,
                        "Android/data/$packageName/files/yabause/games",
                        "Android/data/$packageName/files",
                    )
                    markwon.setMarkdown(viewMessage, welcomeMessage)
                }else {
                    val welcomeMessage = resources.getString(
                        org.devmiyax.yabasanshiro.R.string.welcome,
                        YabauseStorage.storage.gamePath
                    )
                    markwon.setMarkdown(viewMessage, welcomeMessage)
                }
                return
            }
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage!!)
        }

        val viewMessage = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.empty_message) as? View
        val viewPager = rootView.findViewById(org.devmiyax.yabasanshiro.R.id.view_pager_game_index) as? ViewPager
        viewMessage?.visibility = View.GONE
        viewPager?.visibility = VISIBLE

        // -----------------------------------------------------------------
        // Recent Play Game
        var recentList: MutableList<GameInfo?>? = null
        try {
            recentList = Select()
                .from(GameInfo::class.java)
                .orderBy("lastplay_date DESC")
                .limit(5)
                .execute<GameInfo?>()
        } catch (e: Exception) {
            Log.d(TAG, e.localizedMessage!!)
        }
        val resentAdapter = GameItemAdapter(recentList)

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
            val alphabetedList: MutableList<GameInfo?> =
                ArrayList()
            val it = list!!.iterator()
            while (it.hasNext()) {
                val game = it.next()
                if (game.game_title.uppercase(Locale.ROOT).indexOf(alphabet[i]) == 0) {
                    alphabetedList.add(game)
                    Log.d(
                        "GameSelect",
                        alphabet[i] + ":" + game.game_title
                    )
                    it.remove()
                    hit = true
                    it.remove()
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
        val othersList: MutableList<GameInfo?> = ArrayList()
        val it: Iterator<GameInfo> = list!!.iterator()
        while (it.hasNext()) {
            val game = it.next()
            Log.d("GameSelect", "Others:" + game.game_title)
            othersList.add(game)
        }

        if (othersList.size > 0) {
            val otherAdapter = GameItemAdapter(othersList)
            otherAdapter.setOnItemClickListener(this)
            val otherPage = GameListPage("OTHERS", otherAdapter)
            gameListPages!!.add(otherPage)
        }
    }

    override fun onItemClick(position: Int, item: GameInfo?, v: View?) {
        if( item != null ){
            presenter.startGame(item,yabauseActivityLauncher)
        }
    }

    @SuppressLint("DefaultLocale")
    override fun onGameRemoved(item: GameInfo?) {

        firebaseAnalytics?.logEvent("Game Select Fragment"){
            param("event", "onGameRemoved")
        }

        if (item == null) return
        val recentPage = gameListPages!!.find { it.pageTitle == "RECENT" }
        recentPage?.gameList?.removeItem(item.id)

        val title = item.game_title.uppercase(Locale.getDefault())[0]
        val alphabetPage = gameListPages!!.find { it.pageTitle == title.toString() }
        if (alphabetPage != null) {
            alphabetPage.gameList.removeItem(item.id)
            if (alphabetPage.gameList.itemCount == 0) {

                val index = gameListPages!!.indexOfFirst { it.pageTitle == title.toString() }
                if (index != -1) {
                    tabLayout.removeTabAt(index)
                }

                gameListPages!!.removeAll { it.pageTitle == title.toString() }
                tabPageAdapter.notifyDataSetChanged()
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
                tabPageAdapter.notifyDataSetChanged()
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
        presenter.onResume()
    }

    var isFront = true

    override fun onPause() {
        isFront = false
        super.onPause()
        this.presenter.onPause()
    }



    override fun onDestroy() {
        System.gc()
        super.onDestroy()
    }

    override fun onShowMessage(string_id: Int) {
        showSnackBar(string_id)
    }

    override fun onShowDialog(message: String) {
        showDialog(message)
    }

    override fun onUpdateDialogMessage(message: String) {
        updateDialogString(message)
    }

    override fun onDismissDialog() {
        dismissDialog()
    }

    override fun onLoadRows() {
        loadRows()
    }


    override fun onStartSyncBackUp(){
    }

    override fun onFinishSyncBackUp(result: GameSelectPresenter.SyncResult, message: String) {
        if( result == GameSelectPresenter.SyncResult.SUCCESS ){
            Snackbar.make(rootView.rootView, message, Snackbar.LENGTH_LONG).show();
        }

        if( result == GameSelectPresenter.SyncResult.FAIL ){
            val color = ContextCompat.getColor(requireContext(), org.devmiyax.yabasanshiro.R.color.design_default_color_error)
            val snackbar = Snackbar.make(rootView.rootView, message, Snackbar.LENGTH_LONG)
            snackbar.setTextColor( color )
            snackbar.show()
        }


    }

    companion object {
        private const val TAG = "GameSelectFragmentPhone"
        private var instance: GameSelectFragmentPhone? = null

        @JvmField
        var myOnClickListener: View.OnClickListener? = null
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
