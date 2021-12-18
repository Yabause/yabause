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
/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package org.uoyabause.android.tv

import android.Manifest
import android.app.Activity
import android.app.ProgressDialog
import android.app.UiModeManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.drawable.Drawable
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.util.DisplayMetrics
import android.util.Log
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.leanback.app.BackgroundManager
import androidx.leanback.app.BrowseSupportFragment
import androidx.leanback.widget.ArrayObjectAdapter
import androidx.leanback.widget.HeaderItem
import androidx.leanback.widget.ListRow
import androidx.leanback.widget.ListRowPresenter
import androidx.leanback.widget.OnItemViewClickedListener
import androidx.leanback.widget.OnItemViewSelectedListener
import androidx.leanback.widget.Presenter
import androidx.leanback.widget.Row
import androidx.leanback.widget.RowPresenter
import androidx.preference.PreferenceManager
import com.activeandroid.query.Select
import com.google.android.gms.ads.AdListener
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.InterstitialAd
import com.google.android.gms.ads.MobileAds
import com.google.android.gms.analytics.HitBuilders
import com.google.android.gms.analytics.HitBuilders.ScreenViewBuilder
import com.google.android.gms.analytics.Tracker
import com.google.firebase.analytics.FirebaseAnalytics
import com.google.firebase.auth.FirebaseAuth
import io.reactivex.Observer
import io.reactivex.disposables.Disposable
import java.io.File
import java.lang.Exception
import java.net.InetAddress
import java.net.NetworkInterface
import java.net.URI
import java.net.URLDecoder
import java.util.Calendar
import java.util.Collections
import java.util.Timer
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
import org.uoyabause.android.ShowPinInFragment.Companion.newInstance
import org.uoyabause.android.Yabause
import org.uoyabause.android.YabauseApplication
import org.uoyabause.android.YabauseStorage.Companion.storage
import org.uoyabause.android.download.IsoDownload
import org.uoyabause.android.tv.GameSelectFragment.GridItemPresenter
import org.uoyabause.android.tv.GameSelectFragment.ItemViewClickedListener
import org.uoyabause.android.tv.GameSelectFragment.ItemViewSelectedListener

class GameSelectFragment : BrowseSupportFragment(), FileSelectedListener,
    GameSelectPresenterListener {
    private val mHandler = Handler()
    private var mRowsAdapter: ArrayObjectAdapter? = null
    private var mDefaultBackground: Drawable? = null
    private var mMetrics: DisplayMetrics? = null
    private val mBackgroundTimer: Timer? = null
    private val mBackgroundURI: URI? = null
    private var mBackgroundManager: BackgroundManager? = null
    private var mTracker: Tracker? = null
    private var mInterstitialAd: InterstitialAd? = null
    private var mFirebaseAnalytics: FirebaseAnalytics? = null
    var isfisrtupdate = true
    var v_: View? = null
    var presenter_: GameSelectPresenter? = null
    var alphabet = arrayOf("A",
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
        "Z")
    private var mProgressDialog: ProgressDialog? = null

    /**
     * Called when the 'show camera' button is clicked.
     * Callback is defined in resource layout definition.
     */
    fun checkStoragePermission(): Int {
        if (Build.VERSION.SDK_INT >= 23) {
            // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(requireActivity(),
                    Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(requireActivity(),
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED
            ) {
                // Contacts permissions have not been granted.
                Log.i(TAG, "Storage permissions has NOT been granted. Requesting permissions.")
                // if (shouldShowRequestPermissionRationale(Manifest.permission.READ_EXTERNAL_STORAGE)
                //        || shouldShowRequestPermissionRationale(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                // } else {
                requestPermissions(PERMISSIONS_STORAGE, REQUEST_STORAGE)
                // }
                return -1
            }
        }
        return 0
    }

    fun verifyPermissions(grantResults: IntArray): Boolean {
        // At least one result must be checked.
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
            // We have requested multiple permissions for contacts, so all of them need to be
            // checked.
            if (verifyPermissions(grantResults) == true) {
                updateGameList()
            } else {
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
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

    override fun fileSelected(file: File) {
        presenter_!!.fileSelected(file)
    }

    // @Override
    // public void onUpdateGameList() {
    //    loadRows();
    //    dismissDialog();
    //    if(isfisrtupdate) {
    //        isfisrtupdate = false;
    //        presenter_.checkSignIn();
    //    }
    // }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        retainInstance = true
        presenter_ = GameSelectPresenter(this, this)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            if (requireContext().display?.isMinimalPostProcessingSupported()!!) {
                requireActivity().window.setPreferMinimalPostProcessing(true)
            } else {
            }
        }
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        Log.i(TAG, "onCreate")
        super.onActivityCreated(savedInstanceState)
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(requireActivity())
        val application = requireActivity().application as YabauseApplication
        mTracker = application.defaultTracker
        MobileAds.initialize(requireContext())
        mInterstitialAd = InterstitialAd(activity)
        mInterstitialAd!!.adUnitId = activity!!.getString(R.string.banner_ad_unit_id)
        requestNewInterstitial()
        mInterstitialAd!!.adListener = object : AdListener() {
            override fun onAdClosed() {
                requestNewInterstitial()
            }
        }
        val intent = requireActivity().intent
        val uri = intent.data
        if (uri != null && !uri.pathSegments.isEmpty()) {
/*
            ComponentName componentName = new ComponentName(getActivity(), CheckPSerivce.class);
            JobInfo.Builder builder = new JobInfo.Builder(1, componentName);
            builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
            JobScheduler scheduler = (JobScheduler) getActivity().getSystemService(Context.JOB_SCHEDULER_SERVICE);
            scheduler.schedule(builder.build());
*/
            val pathSegments = uri.pathSegments
            var filename = pathSegments[1]
            Log.d(TAG, "filename: $filename")
            try {
                filename = URLDecoder.decode(filename, "UTF-8")
            } catch (e: Exception) {
            }
            val game = GameInfo.getFromFileName(filename)
            if (game != null) {
                val c = Calendar.getInstance()
                game.lastplay_date = c.time
                game.save()
                if (mTracker != null) {
                    mTracker!!.send(HitBuilders.EventBuilder()
                        .setCategory("Action")
                        .setAction(game.game_title)
                        .build())
                }
                val bundle = Bundle()
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, game.product_number)
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, game.game_title)
                mFirebaseAnalytics!!.logEvent(
                    "yab_start_game", bundle
                )
                val intent_game = Intent(activity, Yabause::class.java)
                intent_game.putExtra("org.uoyabause.android.FileNameEx", game.file_path)
                intent.putExtra("org.uoyabause.android.gamecode", game.product_number)
                startActivityForResult(intent_game, YABAUSE_ACTIVITY)
            }
        }
        prepareBackgroundManager()
        setupUIElements()
        setupEventListeners()
        if (mRowsAdapter == null) {
            mRowsAdapter = ArrayObjectAdapter(ListRowPresenter())
            val gridHeader = HeaderItem(0, "PREFERENCES")
            val mGridPresenter = GridItemPresenter()
            val gridRowAdapter = ArrayObjectAdapter(mGridPresenter)
            gridRowAdapter.add(resources.getString(R.string.setting))
            val uiModeManager =
                requireActivity().getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
            if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
                //    gridRowAdapter.add(getResources().getString(R.string.invite));
            }
            // val prefs = activity!!.getSharedPreferences("private", Context.MODE_PRIVATE)
            // Boolean hasDonated = prefs.getBoolean("donated", false);
            // if( !hasDonated) {
            //    gridRowAdapter.add(getResources().getString(R.string.donation));
            // }
            gridRowAdapter.add(getString(R.string.load_game))
            gridRowAdapter.add(resources.getString(R.string.refresh_db))
            // gridRowAdapter.add("GoogleDrive");
            val auth = FirebaseAuth.getInstance()
            if (auth.currentUser != null) {
                gridRowAdapter.add(resources.getString(R.string.sign_out))
            } else {
                gridRowAdapter.add(resources.getString(R.string.sign_in))
            }
            gridRowAdapter.add(resources.getString(R.string.sign_in_to_other_devices))
            mRowsAdapter!!.add(ListRow(gridHeader, gridRowAdapter))
            setSelectedPosition(0, false)
            adapter = mRowsAdapter
        }
        if (checkStoragePermission() == 0) {
            updateBackGraound()
            updateGameList()
        }
        /*
        View rootView = getTitleView();
        TextView tv = (TextView) rootView.findViewById(R.id.title_text);
        if( tv != null ) {
            tv.setTextSize(14);
        }
*/
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        v_ = super.onCreateView(inflater, container, savedInstanceState)
        val uiModeManager = requireActivity().getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
            val rootView = titleView
            val tv = rootView.findViewById<View>(R.id.title_text) as TextView?
            if (tv != null) {
                tv.textSize = 24f
            }
        }
        return v_
    }

    private var observer: Observer<*>? = null
    fun updateGameList() {
        // showDialog();
        if (observer != null) return
        val tmpobserver: Observer<String> = object : Observer<String> {
            // GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();
            override fun onSubscribe(d: Disposable) {
                observer = this
                showDialog()
            }

            override fun onNext(response: String) {
                updateDialogString(response)
            }

            override fun onError(e: Throwable) {
                observer = null
                dismissDialog()
            }

            override fun onComplete() {
                observer = null
                loadRows()
                dismissDialog()
                if (isfisrtupdate) {
                    isfisrtupdate = false
                    val ac: Activity? = this@GameSelectFragment.activity
                    if (ac != null && ac.intent.getBooleanExtra("showPin", false)) {
                        newInstance(presenter_!!).show(childFragmentManager, "sample")
                    } else {
                        presenter_!!.checkSignIn()
                    }
                }
            }
        }
        presenter_!!.updateGameList(refresh_level, tmpobserver)
        refresh_level = 0
    }

    override fun onResume() {
        super.onResume()
        isForeground = this
        if (mTracker != null) {
            mTracker!!.setScreenName(TAG)
            mTracker!!.send(ScreenViewBuilder().build())
        }
        updateGameList()
    }

    override fun onPause() {
        isForeground = null
        dismissDialog()
        super.onPause()
    }

    override fun onDestroy() {
        //this.setSelectedPosition(-1, false)
        System.gc()
        super.onDestroy()
        /*
        if (null != mBackgroundTimer) {
            Log.d(TAG, "onDestroy: " + mBackgroundTimer.toString());
            mBackgroundTimer.cancel();
        }
*/
    }

    private fun loadRows() {
        if (!isAdded) return
        var addindex = 0
        mRowsAdapter = ArrayObjectAdapter(ListRowPresenter())

        // -----------------------------------------------------------------
        // Recent Play Game
        var rlist: List<GameInfo?>? = null
        try {
            rlist = Select()
                .from(GameInfo::class.java)
                .orderBy("lastplay_date DESC")
                .limit(5)
                .execute()
        } catch (e: Exception) {
            println(e)
        }
        val recentHeader = HeaderItem(addindex.toLong(), "RECENT")
        val itx = rlist!!.iterator()
        val cardPresenter_recent = CardPresenter()
        val listRowAdapter_recent = ArrayObjectAdapter(cardPresenter_recent)
        var hit = false
        while (itx.hasNext()) {
            val game = itx.next()
            listRowAdapter_recent.add(game)
            hit = true
        }

        // ----------------------------------------------------------------------
        // Refernce
        if (hit) {
            mRowsAdapter!!.add(ListRow(recentHeader, listRowAdapter_recent))
            addindex++
        }
        val gridHeader = HeaderItem(addindex.toLong(), "PREFERENCES")
        val mGridPresenter = GridItemPresenter()
        val gridRowAdapter = ArrayObjectAdapter(mGridPresenter)
        // gridRowAdapter.add("Backup");
        gridRowAdapter.add(resources.getString(R.string.setting))
        val uiModeManager = requireActivity().getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
            //    gridRowAdapter.add(getResources().getString(R.string.invite));
        }
        // val prefs = activity!!.getSharedPreferences("private", Context.MODE_PRIVATE)
        // Boolean hasDonated = prefs.getBoolean("donated", false);
        // if( !hasDonated) {
        //    gridRowAdapter.add(getResources().getString(R.string.donation));
        // }
        gridRowAdapter.add(getString(R.string.load_game))
        gridRowAdapter.add(resources.getString(R.string.refresh_db))
        // gridRowAdapter.add("GoogleDrive");
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser != null) {
            gridRowAdapter.add(resources.getString(R.string.sign_out))
        } else {
            gridRowAdapter.add(resources.getString(R.string.sign_in))
        }
        gridRowAdapter.add(resources.getString(R.string.sign_in_to_other_devices))
        mRowsAdapter!!.add(ListRow(gridHeader, gridRowAdapter))
        addindex++

        // -----------------------------------------------------------------
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

//        itx = list.iterator();
//        while(itx.hasNext()){
//            GameInfo game = itx.next();
//            Log.d("GameSelect",game.game_title + ":" + game.file_path + ":" + game.iso_file_path );
//        }
        var i: Int
        i = 0
        while (i < alphabet.size) {
            hit = false
            val cardPresenter = CardPresenter()
            val listRowAdapter = ArrayObjectAdapter(cardPresenter)
            val it = list!!.iterator()
            while (it.hasNext()) {
                val game = it.next()
                if (game.game_title.toUpperCase().indexOf(alphabet[i]) == 0) {
                    listRowAdapter.add(game)
                    Log.d("GameSelect", alphabet[i] + ":" + game.game_title)
                    it.remove()
                    hit = true
                }
            }
            if (hit) {
                val header = HeaderItem(addindex.toLong(), alphabet[i])
                mRowsAdapter!!.add(ListRow(header, listRowAdapter))
                addindex++
            }
            i++
        }
        val cardPresenter = CardPresenter()
        val listRowAdapter = ArrayObjectAdapter(cardPresenter)
        val it: Iterator<GameInfo> = list!!.iterator()
        while (it.hasNext()) {
            val game = it.next()
            Log.d("GameSelect", "Others:" + game.game_title)
            listRowAdapter.add(game)
        }
        val header = HeaderItem(addindex.toLong(), "Others")
        mRowsAdapter!!.add(ListRow(header, listRowAdapter))
        adapter = mRowsAdapter
    }

    private fun prepareBackgroundManager() {
        mBackgroundManager = BackgroundManager.getInstance(activity)
        mBackgroundManager!!.setAutoReleaseOnStop(false)
        mBackgroundManager!!.attach(requireActivity().window)
        mDefaultBackground =
            null // getBrandColor(); //getResources().getDrawable(R.drawable.saturn);
        mMetrics = DisplayMetrics()
        // requireActivity().windowManager.defaultDisplay.getMetrics(mMetrics)
        requireContext().display?.getRealMetrics(mMetrics)
    }

    private fun updateBackGraound() {
        // val sp = PreferenceManager.getDefaultSharedPreferences(activity)
        val image_path = "err" // sp.getString("select_image", "err");
        if (image_path == "err") {
            mDefaultBackground = null // getResources().getDrawable(R.drawable.saturn);
            mBackgroundManager!!.drawable = mDefaultBackground
        } else {
            try {
                val options = BitmapFactory.Options()
                options.inPreferredConfig = Bitmap.Config.ARGB_8888
                val bitmap = BitmapFactory.decodeFile(image_path, options)

/*
                getActivity().grantUriPermission("org.uoyabause.android",
                        Uri.parse(image_path),
                        Intent.FLAG_GRANT_READ_URI_PERMISSION);

                InputStream inputStream = getActivity().getContentResolver().openInputStream(Uri.parse(image_path));

                BitmapFactory.Options imageOptions = new BitmapFactory.Options();
                imageOptions.inJustDecodeBounds = true;
                BitmapFactory.decodeStream(inputStream, null, imageOptions);
                Log.v("image", "Original Image Size: " + imageOptions.outWidth + " x " + imageOptions.outHeight);

                inputStream.close();

                Bitmap bitmap;
                int imageSizeMax = 1920;
                inputStream = getActivity().getContentResolver().openInputStream(Uri.parse(image_path));
                float imageScaleWidth = (float)imageOptions.outWidth / imageSizeMax;
                float imageScaleHeight = (float)imageOptions.outHeight / imageSizeMax;

                if (imageScaleWidth > 2 && imageScaleHeight > 2) {
                    BitmapFactory.Options imageOptions2 = new BitmapFactory.Options();
                    int imageScale = (int)Math.floor((imageScaleWidth > imageScaleHeight ? imageScaleHeight : imageScaleWidth));
                    for (int i = 2; i <= imageScale; i *= 2) {
                        imageOptions2.inSampleSize = i;
                    }
                    bitmap = BitmapFactory.decodeStream(inputStream, null, imageOptions2);
                    Log.v("image", "Sample Size: 1/" + imageOptions2.inSampleSize);
                } else {
                    bitmap = BitmapFactory.decodeStream(inputStream);
                }

                inputStream.close();
                //mDefaultBackground = Drawable.createFromStream(inputStream, image_path );
     */mBackgroundManager!!.setBitmap(bitmap)
            } catch (e: Exception) {
                mDefaultBackground = null // getResources().getDrawable(R.drawable.saturn);
                mBackgroundManager!!.drawable = mDefaultBackground
            }
        }
    }

    private fun setupUIElements() {
        // setBadgeDrawable(getActivity().getResources().getDrawable( R.drawable.banner));
        title =
            getString(R.string.app_name) + getVersionName(activity) // Badge, when set, takes precedent
        // over title
        headersState = HEADERS_HIDDEN
        isHeadersTransitionOnBackEnabled = true

        // set fastLane (or headers) background color
        brandColor = ContextCompat.getColor(requireContext(), R.color.fastlane_background)
        // set search icon color
        searchAffordanceColor = ContextCompat.getColor(requireContext(), R.color.search_opaque)
    }

    private fun setupEventListeners() {
/*
        setOnSearchClickedListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                Toast.makeText(getActivity(), "Implement your own in-app search", Toast.LENGTH_LONG)
                        .show();
            }
        });
*/
        setOnSearchClickedListener(null)
        onItemViewClickedListener = ItemViewClickedListener()
        onItemViewSelectedListener = ItemViewSelectedListener()
    }

    /*
        protected void updateBackground(String uri) {
            int width = mMetrics.widthPixels;
            int height = mMetrics.heightPixels;
            Glide.with(getActivity())
                    .load(uri)
                    .centerCrop()
                    .error(mDefaultBackground)
                    .into(new SimpleTarget<GlideDrawable>(width, height) {
                        @Override
                        public void onResourceReady(GlideDrawable resource,
                                                    GlideAnimation<? super GlideDrawable>
                                                            glideAnimation) {
                            mBackgroundManager.setDrawable(resource);
                        }
                    });
            mBackgroundTimer.cancel();
        }
    */
    /*
    private void startBackgroundTimer() {
        if (null != mBackgroundTimer) {
            mBackgroundTimer.cancel();
        }
        mBackgroundTimer = new Timer();
        mBackgroundTimer.schedule(new UpdateBackgroundTask(), BACKGROUND_UPDATE_DELAY);
    }
*/
    val SETTING_ACTIVITY = 0x01
    val YABAUSE_ACTIVITY = 0x02
    val DOWNLOAD_ACTIVITY = 0x03

    private inner class ItemViewClickedListener : OnItemViewClickedListener {
        override fun onItemClicked(
            itemViewHolder: Presenter.ViewHolder,
            item: Any,
            rowViewHolder: RowPresenter.ViewHolder,
            row: Row
        ) {
            if (item is GameInfo) {
                val game = item
                val c = Calendar.getInstance()
                game.lastplay_date = c.time
                game.save()
                if (mTracker != null) {
                    mTracker!!.send(HitBuilders.EventBuilder()
                        .setCategory("Action")
                        .setAction(game.game_title)
                        .build())
                }
                val bundle = Bundle()
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, game.product_number)
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, game.game_title)
                mFirebaseAnalytics!!.logEvent(
                    "yab_start_game", bundle
                )
                val intent = Intent(activity, Yabause::class.java)
                intent.putExtra("org.uoyabause.android.FileNameEx", game.file_path)
                intent.putExtra("org.uoyabause.android.gamecode", game.product_number)
                startActivityForResult(intent, YABAUSE_ACTIVITY)
            } else if (item is String) {
                if (item == getString(R.string.sign_in)) {
                    presenter_!!.signIn()
                } else if (item == getString(R.string.sign_out)) {
                    presenter_!!.signOut()
                } else if (item == getString(R.string.sign_in_to_other_devices)) {
                    newInstance(presenter_!!).show(childFragmentManager, "sample")
                } else if (item.indexOf("Backup") >= 0) {
                    val intent = Intent(activity, IsoDownload::class.java)
                    val savepath: String?
                    val ys = storage
                    if (ys.hasExternalSD() == false) {
                        savepath = ys.gamePath
                    } else {
                        val sharedPref = PreferenceManager.getDefaultSharedPreferences(
                            activity)
                        val sel = sharedPref.getString("pref_game_download_directory", "0")
                        if (sel != null) {
                            if (sel == "0") { // internal
                                savepath = ys.gamePath
                            } else if (sel == "1") { // external
                                savepath = ys.externalGamePath
                            } else {
                                savepath = ys.gamePath // Error
                            }
                        } else {
                            savepath = ys.gamePath // Error
                        }
                    }
                    intent.putExtra("save_path", savepath)
                    startActivityForResult(intent, DOWNLOAD_ACTIVITY)
                    // FragmentTransaction ft = getFragmentManager().beginTransaction();
                    // DownloadDialog newFragment = new DownloadDialog();
                    // newFragment.show(ft, "dialog");
                } else if (item.indexOf(getString(R.string.setting)) >= 0) {
                    val intent = Intent(activity, SettingsActivity::class.java)
                    startActivityForResult(intent, SETTING_ACTIVITY)
                } else if (item.indexOf(getString(R.string.load_game)) >= 0) {

                    val yabroot = File(storage.rootPath)
                    val sharedPref = PreferenceManager.getDefaultSharedPreferences(
                        activity)
                    val last_dir = sharedPref.getString("pref_last_dir", yabroot.path)
                    val fd = FileDialog(activity, last_dir)
                    fd.addFileListener(this@GameSelectFragment)
                    fd.showDialog()
                } else if (item.indexOf(getString(R.string.refresh_db)) >= 0) {
                    refresh_level = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                    // }else if(  ((String) item).indexOf(getString(R.string.donation)) >= 0){
                    //    Intent intent = new Intent(getActivity(), DonateActivity.class);
                    //    startActivity(intent);
                } else if (item.indexOf(getString(R.string.invite)) >= 0) {
                    onInviteClicked()
                } else if (item.indexOf("GoogleDrive") >= 0) {
                    onGoogleDriveClciked()
                }
            }
        }
    }

    private fun onGoogleDriveClciked() {
        val pm = requireActivity().packageManager
        try {
            pm.getPackageInfo("org.uoyabause.gdrive", PackageManager.GET_ACTIVITIES)
            val intent = Intent("org.uoyabause.gdrive.LAUNCH")
            startActivity(intent)
        } catch (e: PackageManager.NameNotFoundException) {
            val intent =
                Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=org.uoyabause.android"))
            try {
                requireActivity().startActivity(intent)
            } catch (ex: Exception) {
                ex.printStackTrace()
            }
        }
    }

    private fun onInviteClicked() {
/*
        Intent intent = new AppInviteInvitation.IntentBuilder(getString(R.string.invitation_title))
                .setMessage(getString(R.string.invitation_message))
                .setDeepLink(Uri.parse(getString(R.string.invitation_deep_link)))
                .setCustomImage(Uri.parse(getString(R.string.invitation_custom_image)))
                .setCallToActionText(getString(R.string.invitation_cta))
                .build();
        startActivityForResult(intent, REQUEST_INVITE);

        SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
        Date date = new Date(System.currentTimeMillis());
        SharedPreferences.Editor editor = prefs.edit();
        editor.putLong("introduce", date.getTime());
        editor.commit();
*/
    }

    private inner class ItemViewSelectedListener : OnItemViewSelectedListener {
        override fun onItemSelected(
            itemViewHolder: Presenter.ViewHolder?,
            item: Any?,
            rowViewHolder: RowPresenter.ViewHolder?,
            row: Row?
        ) {
            // if (item instanceof Movie) {
            //   mBackgroundURI = ((Movie) item).getBackgroundImageURI();
            //    startBackgroundTimer();
            // }
        }
    }

    /*
    private class UpdateBackgroundTask extends TimerTask {

        @Override
        public void run() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mBackgroundURI != null) {
                        updateBackground(mBackgroundURI.toString());
                    }
                }
            });

        }
    }
*/
    private inner class GridItemPresenter : Presenter() {
        override fun onCreateViewHolder(parent: ViewGroup): ViewHolder {
            val view = TextView(parent.context)
            view.layoutParams = ViewGroup.LayoutParams(GRID_ITEM_WIDTH,
                GRID_ITEM_HEIGHT)
            view.isFocusable = true
            view.isFocusableInTouchMode = true
            view.setBackgroundColor(ContextCompat.getColor(requireContext(), R.color.default_background))
            view.setTextColor(Color.WHITE)
            view.gravity = Gravity.CENTER
            return ViewHolder(view)
        }

        override fun onBindViewHolder(viewHolder: ViewHolder, item: Any) {
            (viewHolder.view as TextView).text = item as String
        }

        override fun onUnbindViewHolder(viewHolder: ViewHolder) {}
    }

    override fun onShowMessage(string_id: Int) {
        showSnackbar(string_id)
    }

    private fun showSnackbar(id: Int) {
        Toast.makeText(activity, getString(id), Toast.LENGTH_SHORT).show()
        /*
        Snackbar
                .make(v_, getString(id), Snackbar.LENGTH_SHORT)
                .show();
*/

/*
        new AlertDialog.Builder(this)
                .setMessage(getString(id))
                .setPositiveButton("OK", null)
                .show();
*/
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            GameSelectPresenter.RC_GAME_SIGN_IN -> {
                presenter_!!.onGameIdSignIn(resultCode, data)
            }
            GameSelectPresenter.RC_SIGN_IN -> {
                presenter_!!.onSignIn(resultCode, data)
            }
            DOWNLOAD_ACTIVITY -> {
                if (resultCode == 0) {
                    refresh_level = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                }
                if (resultCode == GAMELIST_NEED_TO_UPDATED) {
                    refresh_level = 3
                    if (checkStoragePermission() == 0) {
                        updateGameList()
                    }
                    updateBackGraound()
                } else if (resultCode == GAMELIST_NEED_TO_RESTART) {
                    val intent = Intent(activity, StartupActivity::class.java)
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                    startActivity(intent)
                    requireActivity().finish()
                } else {
                    updateBackGraound()
                }
            }
            SETTING_ACTIVITY -> if (resultCode == GAMELIST_NEED_TO_UPDATED) {
                refresh_level = 3
                if (checkStoragePermission() == 0) {
                    updateGameList()
                }
                updateBackGraound()
            } else if (resultCode == GAMELIST_NEED_TO_RESTART) {
                val intent = Intent(activity, StartupActivity::class.java)
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
                startActivity(intent)
                requireActivity().finish()
            } else {
                updateBackGraound()
            }
            YABAUSE_ACTIVITY -> if (BuildConfig.BUILD_TYPE != "pro") {
                val prefs = requireActivity().getSharedPreferences("private", Context.MODE_PRIVATE)
                val hasDonated = prefs.getBoolean("donated", false)
                if (hasDonated == false) {
                    val rn = Math.random()
                    if (rn <= 0.5) {
                        val uiModeManager =
                            requireActivity().getSystemService(Context.UI_MODE_SERVICE) as UiModeManager
                        if (uiModeManager.currentModeType != Configuration.UI_MODE_TYPE_TELEVISION) {
                            if (mInterstitialAd!!.isLoaded) {
                                mInterstitialAd!!.show()
                            } else {
                                val intent = Intent(activity, AdActivity::class.java)
                                startActivity(intent)
                            }
                        } else {
                            val intent = Intent(activity, AdActivity::class.java)
                            startActivity(intent)
                        }
                    } else if (rn > 0.5) {
                        val intent = Intent(activity, AdActivity::class.java)
                        startActivity(intent)
                    }
                }
            }
            else -> {
            }
        }
    }

    private fun requestNewInterstitial() {
        val adRequest = AdRequest.Builder() // .addTestDevice("YOUR_DEVICE_HASH")
            .build()
        mInterstitialAd!!.loadAd(adRequest)
    }

    companion object {
        private const val TAG = "GameSelectFragment"
        private const val BACKGROUND_UPDATE_DELAY = 300
        private const val GRID_ITEM_WIDTH = 266
        private const val GRID_ITEM_HEIGHT = 200
        private const val NUM_ROWS = 6
        private const val NUM_COLS = 15
        private const val REQUEST_INVITE = 0x1121

        // public static final int RC_SIGN_IN = 123;
        @JvmField
        var refresh_level = 2
        @JvmField
        var isForeground: GameSelectFragment? = null
        private const val REQUEST_STORAGE = 1
        private val PERMISSIONS_STORAGE = arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE)

        /**
         * Get IP address from first non-localhost interface
         *
         * @param ipv4 true=return ipv4, false=return ipv6
         * @return address or empty string
         */
        fun getIPAddress(useIPv4: Boolean): String {
            try {
                val interfaces: List<NetworkInterface> =
                    Collections.list(NetworkInterface.getNetworkInterfaces())
                for (intf in interfaces) {
                    val addrs: List<InetAddress> = Collections.list(intf.inetAddresses)
                    for (addr in addrs) {
                        if (!addr.isLoopbackAddress) {
                            val sAddr = addr.hostAddress
                            // boolean isIPv4 = InetAddressUtils.isIPv4Address(sAddr);
                            val isIPv4 = sAddr.indexOf(':') < 0
                            if (useIPv4) {
                                if (isIPv4) return sAddr
                            } else {
                                if (!isIPv4) {
                                    val delim = sAddr.indexOf('%') // drop ip6 zone suffix
                                    return if (delim < 0) sAddr.toUpperCase() else sAddr.substring(0,
                                        delim).toUpperCase()
                                }
                            }
                        }
                    }
                }
            } catch (ex: Exception) {
            } // for now eat exceptions
            return ""
        }

        /**
         * @param context
         * @return
         */
        fun getVersionName(context: Context?): String {

//        return getIPAddress(true);
            val pm = context!!.packageManager
            var versionName = ""
            try {
                val packageInfo = pm.getPackageInfo(context.packageName, 0)
                versionName = packageInfo.versionName
            } catch (e: PackageManager.NameNotFoundException) {
                e.printStackTrace()
            }
            return versionName
        }

        const val GAMELIST_NEED_TO_UPDATED = 0x8001
        const val GAMELIST_NEED_TO_RESTART = 0x8002
        const val READ_REQUEST_CODE = 0x8003
    }
}
