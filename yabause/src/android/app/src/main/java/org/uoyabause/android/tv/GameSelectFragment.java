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

package org.uoyabause.android.tv;

import java.io.File;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.URI;
import java.net.URLDecoder;
import java.util.Calendar;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Timer;

import android.Manifest;
import android.app.ProgressDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import androidx.leanback.app.BackgroundManager;
import androidx.leanback.app.BrowseFragment;
import androidx.leanback.widget.ArrayObjectAdapter;
import androidx.leanback.widget.HeaderItem;
import androidx.leanback.widget.ListRow;
import androidx.leanback.widget.ListRowPresenter;
import androidx.leanback.widget.OnItemViewClickedListener;
import androidx.leanback.widget.OnItemViewSelectedListener;
import androidx.leanback.widget.Presenter;
import androidx.leanback.widget.Row;
import androidx.leanback.widget.RowPresenter;
import androidx.core.app.ActivityCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.activeandroid.query.Select;

import org.uoyabause.android.AdActivity;
import org.uoyabause.android.DonateActivity;
import org.uoyabause.android.FileDialog;
import org.uoyabause.android.GameInfo;
import org.uoyabause.android.GameSelectPresenter;
import org.uoyabause.android.download.IsoDownload;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;
import org.uoyabause.android.Yabause;
import org.uoyabause.android.YabauseApplication;
import org.uoyabause.android.YabauseSettings;
import org.uoyabause.android.YabauseStorage;
import org.uoyabause.uranus.StartupActivity;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.InterstitialAd;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.analytics.Tracker;
import com.google.android.gms.analytics.HitBuilders;
import com.google.firebase.analytics.FirebaseAnalytics;
import com.google.firebase.auth.FirebaseAuth;

import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;


public class GameSelectFragment extends BrowseFragment
        implements FileDialog.FileSelectedListener,GameSelectPresenter.GameSelectPresenterListener  {
    private static final String TAG = "GameSelectFragment";

    private static final int BACKGROUND_UPDATE_DELAY = 300;
    private static final int GRID_ITEM_WIDTH = 266;
    private static final int GRID_ITEM_HEIGHT = 200;
    private static final int NUM_ROWS = 6;
    private static final int NUM_COLS = 15;

    private static final int REQUEST_INVITE = 0x1121;

    private final Handler mHandler = new Handler();
    private ArrayObjectAdapter mRowsAdapter = null;
    private Drawable mDefaultBackground;
    private DisplayMetrics mMetrics;
    private Timer mBackgroundTimer;
    private URI mBackgroundURI;
    private BackgroundManager mBackgroundManager;
    private Tracker mTracker;
    private InterstitialAd mInterstitialAd;
    private FirebaseAnalytics mFirebaseAnalytics;
    boolean isfisrtupdate = true;

    //public static final int RC_SIGN_IN = 123;
    public static int refresh_level = 2;
    public static  GameSelectFragment isForeground = null;
    View v_;

    GameSelectPresenter presenter_;

    String alphabet[]={ "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z" };

    private ProgressDialog mProgressDialog = null;

    private static final int REQUEST_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE};

    /**
     * Get IP address from first non-localhost interface
     * @param ipv4  true=return ipv4, false=return ipv6
     * @return  address or empty string
     */
    public static String getIPAddress(boolean useIPv4) {
        try {
            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface intf : interfaces) {
                List<InetAddress> addrs = Collections.list(intf.getInetAddresses());
                for (InetAddress addr : addrs) {
                    if (!addr.isLoopbackAddress()) {
                        String sAddr = addr.getHostAddress();
                        //boolean isIPv4 = InetAddressUtils.isIPv4Address(sAddr);
                        boolean isIPv4 = sAddr.indexOf(':')<0;

                        if (useIPv4) {
                            if (isIPv4)
                                return sAddr;
                        } else {
                            if (!isIPv4) {
                                int delim = sAddr.indexOf('%'); // drop ip6 zone suffix
                                return delim<0 ? sAddr.toUpperCase() : sAddr.substring(0, delim).toUpperCase();
                            }
                        }
                    }
                }
            }
        } catch (Exception ex) { } // for now eat exceptions
        return "";
    }
    /**
     * Called when the 'show camera' button is clicked.
     * Callback is defined in resource layout definition.
     */
    public int checkStoragePermission() {

        if (Build.VERSION.SDK_INT >= 23) {
            // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(getActivity(), Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED
                    || ActivityCompat.checkSelfPermission(getActivity(), Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                // Contacts permissions have not been granted.
                Log.i(TAG, "Storage permissions has NOT been granted. Requesting permissions.");
                //if (shouldShowRequestPermissionRationale(Manifest.permission.READ_EXTERNAL_STORAGE)
                //        || shouldShowRequestPermissionRationale(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                //} else {
                    requestPermissions(PERMISSIONS_STORAGE, REQUEST_STORAGE);
                //}
                return -1;

            }
        }
        return 0;
    }


    boolean verifyPermissions(int[] grantResults) {
        // At least one result must be checked.
        if(grantResults.length < 1){
            return false;
        }

        // Verify that each required permission has been granted, otherwise return false.
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        if (requestCode == REQUEST_STORAGE) {
            Log.i(TAG, "Received response for contact permissions request.");
            // We have requested multiple permissions for contacts, so all of them need to be
            // checked.
            if (verifyPermissions(grantResults) == true ){
                    updateGameList();
            } else {
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }


    public void showDialog() {
        if( mProgressDialog == null  ) {
            mProgressDialog = new ProgressDialog(getActivity());
            mProgressDialog.setMessage("Updating...");
            mProgressDialog.show();
        }
    }

    public void updateDialogString( String msg) {
        if( mProgressDialog != null  ) {
            mProgressDialog.setMessage("Updating... " + msg);
        }
    }

    public void dismissDialog() {
        if( mProgressDialog != null ) {
            if( mProgressDialog.isShowing() ) {
                mProgressDialog.dismiss();
            }
            mProgressDialog = null;
        }
    }

    @Override
    public void fileSelected(File file) {
        presenter_.fileSelected(file);
    }

    //@Override
    //public void onUpdateGameList() {
    //    loadRows();
    //    dismissDialog();
    //    if(isfisrtupdate) {
    //        isfisrtupdate = false;
    //        presenter_.checkSignIn();
    //    }
    //}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
        presenter_ = new GameSelectPresenter((android.app.Fragment)this,this);
    }


    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate");
        super.onActivityCreated(savedInstanceState);

        mFirebaseAnalytics = FirebaseAnalytics.getInstance(getActivity());
        YabauseApplication application = (YabauseApplication) getActivity().getApplication();
        mTracker = application.getDefaultTracker();
        MobileAds.initialize(application, getActivity().getString(R.string.ad_app_id));
        mInterstitialAd = new InterstitialAd(getActivity());
        mInterstitialAd.setAdUnitId(getActivity().getString(R.string.banner_ad_unit_id));
        requestNewInterstitial();

        mInterstitialAd.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                requestNewInterstitial();
            }
        });

        Intent intent = getActivity().getIntent();
        Uri uri = intent.getData();
        if ( uri != null && !uri.getPathSegments().isEmpty()) {
/*
            ComponentName componentName = new ComponentName(getActivity(), CheckPSerivce.class);
            JobInfo.Builder builder = new JobInfo.Builder(1, componentName);
            builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
            JobScheduler scheduler = (JobScheduler) getActivity().getSystemService(Context.JOB_SCHEDULER_SERVICE);
            scheduler.schedule(builder.build());
*/
            List<String> pathSegments = uri.getPathSegments();
            String filename = pathSegments.get(1);
            Log.d(TAG,"filename: " + filename );
            try { filename = URLDecoder.decode(filename,"UTF-8"); } catch( Exception e ){}
            GameInfo game = (GameInfo) GameInfo.getFromFileName(filename);
            if( game != null ) {
                Calendar c = Calendar.getInstance();
                game.lastplay_date = c.getTime();
                game.save();

                if (mTracker != null) {
                    mTracker.send(new HitBuilders.EventBuilder()
                            .setCategory("Action")
                            .setAction(game.game_title)
                            .build());
                }

                Bundle bundle = new Bundle();
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, game.game_title);
                mFirebaseAnalytics.logEvent(
                        FirebaseAnalytics.Event.SELECT_CONTENT, bundle
                );

                Intent intent_game = new Intent(getActivity(), Yabause.class);
                intent_game.putExtra("org.uoyabause.android.FileNameEx", game.file_path);
                intent.putExtra("org.uoyabause.android.gamecode", game.product_number );
                startActivityForResult(intent_game, YABAUSE_ACTIVITY);
            }
        }

        prepareBackgroundManager();
        setupUIElements();
        setupEventListeners();

        if( mRowsAdapter == null ) {
            mRowsAdapter = new ArrayObjectAdapter(new ListRowPresenter());
            HeaderItem gridHeader = new HeaderItem(0, "PREFERENCES");
            GridItemPresenter mGridPresenter = new GridItemPresenter();
            ArrayObjectAdapter gridRowAdapter = new ArrayObjectAdapter(mGridPresenter);

            gridRowAdapter.add(getResources().getString(R.string.setting));

            UiModeManager uiModeManager = (UiModeManager) getActivity().getSystemService(Context.UI_MODE_SERVICE);
            if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
                //    gridRowAdapter.add(getResources().getString(R.string.invite));
            }
            SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
            //Boolean hasDonated = prefs.getBoolean("donated", false);
            //if( !hasDonated) {
            //    gridRowAdapter.add(getResources().getString(R.string.donation));
            //}
            gridRowAdapter.add(getString(R.string.load_game));
            gridRowAdapter.add(getResources().getString(R.string.refresh_db));
            //gridRowAdapter.add("GoogleDrive");
            FirebaseAuth auth = FirebaseAuth.getInstance();
            if (auth.getCurrentUser() != null) {
                gridRowAdapter.add(getResources().getString(R.string.sign_out));
            } else {
                gridRowAdapter.add(getResources().getString(R.string.sign_in));
            }

            mRowsAdapter.add(new ListRow(gridHeader, gridRowAdapter));
            setSelectedPosition(0,false);
            setAdapter(mRowsAdapter);
        }

        if( checkStoragePermission() == 0 ) {
            updateBackGraound();
            updateGameList();
        }
/*
        View rootView = getTitleView();
        TextView tv = (TextView) rootView.findViewById(R.id.title_text);
        if( tv != null ) {
            tv.setTextSize(14);
        }
*/
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState){
        v_ = super.onCreateView(inflater,container,savedInstanceState);

        UiModeManager uiModeManager = (UiModeManager) getActivity().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
            View rootView = getTitleView();
            TextView tv = (TextView) rootView.findViewById(R.id.title_text);
            if (tv != null) {
                tv.setTextSize(24);
            }
        }
        return v_;
    }

    private Observer observer = null;

    void updateGameList(){
        //showDialog();

        observer = new Observer<String>() {
            //GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();

            @Override
            public void onSubscribe(Disposable d) {
                showDialog();
            }

            @Override
            public void onNext(String response) {
                updateDialogString(response);
            }

            @Override
            public void onError(Throwable e) {
                dismissDialog();
            }

            @Override
            public void onComplete() {
                loadRows();
                dismissDialog();
                if(isfisrtupdate) {
                  isfisrtupdate = false;
                  presenter_.checkSignIn();
                }
            }
        };

        presenter_.updateGameList(refresh_level,observer);
        refresh_level = 0;
    }

    @Override
    public void onResume(){
        super.onResume();
        isForeground = this;
        if( mTracker != null ) {
            mTracker.setScreenName(TAG);
            mTracker.send(new HitBuilders.ScreenViewBuilder().build());
        }
        updateGameList();
    }
    @Override
    public void onPause(){
        isForeground = null;
        dismissDialog();
        super.onPause();
    }
    @Override
    public void onDestroy() {
        this.setSelectedPosition(-1,false );
        System.gc();
        super.onDestroy();
/*
        if (null != mBackgroundTimer) {
            Log.d(TAG, "onDestroy: " + mBackgroundTimer.toString());
            mBackgroundTimer.cancel();
        }
*/
    }

    private void loadRows() {

        if( !isAdded() ) return;

       int addindex = 0;
       mRowsAdapter = new ArrayObjectAdapter(new ListRowPresenter());

        //-----------------------------------------------------------------
        // Recent Play Game
        List<GameInfo> rlist = null;
        try {
            rlist  = new Select()
                    .from(GameInfo.class)
                    .orderBy("lastplay_date DESC")
                    .limit(5)
                    .execute();
        }catch(Exception e ){
            System.out.println(e);
        }

       HeaderItem recentHeader = new HeaderItem(addindex, "RECENT");
        Iterator<GameInfo> itx = rlist.iterator();
        CardPresenter cardPresenter_recent = new CardPresenter();
        ArrayObjectAdapter listRowAdapter_recent = new ArrayObjectAdapter(cardPresenter_recent);
        boolean hit = false;
        while(itx.hasNext()){
            GameInfo game = itx.next();
            listRowAdapter_recent.add(game);
            hit=true;
        }

        //----------------------------------------------------------------------
        // Refernce
        if( hit ) {
            mRowsAdapter.add(new ListRow(recentHeader, listRowAdapter_recent));
            addindex++;
        }

        HeaderItem gridHeader = new HeaderItem(addindex, "PREFERENCES");
        GridItemPresenter mGridPresenter = new GridItemPresenter();
        ArrayObjectAdapter gridRowAdapter = new ArrayObjectAdapter(mGridPresenter);
        //gridRowAdapter.add("Backup");

        gridRowAdapter.add(getResources().getString(R.string.setting));

        UiModeManager uiModeManager = (UiModeManager) getActivity().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
        //    gridRowAdapter.add(getResources().getString(R.string.invite));
        }
        SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
        //Boolean hasDonated = prefs.getBoolean("donated", false);
        //if( !hasDonated) {
        //    gridRowAdapter.add(getResources().getString(R.string.donation));
        //}
        gridRowAdapter.add(getString(R.string.load_game));
        gridRowAdapter.add(getResources().getString(R.string.refresh_db));
        //gridRowAdapter.add("GoogleDrive");
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() != null) {
            gridRowAdapter.add(getResources().getString(R.string.sign_out));
        } else {
            gridRowAdapter.add(getResources().getString(R.string.sign_in));
        }


        mRowsAdapter.add(new ListRow(gridHeader, gridRowAdapter));
        addindex++;


        //-----------------------------------------------------------------
        //
        List<GameInfo> list = null;
        try {
            list =new Select()
                    .from(GameInfo.class)
                    .orderBy("game_title ASC")
                    .execute();
        }catch(Exception e ){
            System.out.println(e);
        }


//        itx = list.iterator();
//        while(itx.hasNext()){
//            GameInfo game = itx.next();
//            Log.d("GameSelect",game.game_title + ":" + game.file_path + ":" + game.iso_file_path );
//        }

        int i;
        for (i = 0; i < alphabet.length; i++) {

            hit = false;
            CardPresenter cardPresenter = new CardPresenter();
            ArrayObjectAdapter listRowAdapter = new ArrayObjectAdapter(cardPresenter);
            Iterator<GameInfo> it = list.iterator();
            while(it.hasNext()){
                GameInfo game = it.next();
                if( game.game_title.toUpperCase().indexOf(alphabet[i]) == 0  ){
                    listRowAdapter.add(game);
                    Log.d("GameSelect", alphabet[i] + ":" + game.game_title);
                    it.remove();
                    hit = true;
                }
            }

            if( hit ) {
                HeaderItem header = new HeaderItem(addindex, alphabet[i]);
                mRowsAdapter.add(new ListRow(header, listRowAdapter));
                addindex++;
            }
        }

        CardPresenter cardPresenter = new CardPresenter();
        ArrayObjectAdapter listRowAdapter = new ArrayObjectAdapter(cardPresenter);
        Iterator<GameInfo> it = list.iterator();
        while(it.hasNext()){
            GameInfo game = it.next();
            Log.d("GameSelect", "Others:" + game.game_title);
            listRowAdapter.add(game);
        }
        HeaderItem header = new HeaderItem(addindex, "Others");
        mRowsAdapter.add(new ListRow(header, listRowAdapter));
        addindex++;


        setAdapter(mRowsAdapter);

    }

    private void prepareBackgroundManager() {
        mBackgroundManager = BackgroundManager.getInstance(getActivity());
        mBackgroundManager.setAutoReleaseOnStop(false);
        mBackgroundManager.attach(getActivity().getWindow());
        mDefaultBackground = null; //getBrandColor(); //getResources().getDrawable(R.drawable.saturn);
        mMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(mMetrics);
    }

    private void updateBackGraound(){

        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getActivity());
        String image_path = sp.getString("select_image", "err");
        if( image_path.equals("err") ) {
            mDefaultBackground = null; //getResources().getDrawable(R.drawable.saturn);
            mBackgroundManager.setDrawable(mDefaultBackground);
        }else{
            try {
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inPreferredConfig = Bitmap.Config.ARGB_8888;
                Bitmap bitmap = BitmapFactory.decodeFile(image_path, options);

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
     */
                mBackgroundManager.setBitmap(bitmap);

            } catch (Exception e) {
                mDefaultBackground = null; //getResources().getDrawable(R.drawable.saturn);
                mBackgroundManager.setDrawable(mDefaultBackground);
            }

        }


    }

    /**
     *
     *
     * @param context
     * @return
     */
    public static String getVersionName(Context context){

//        return getIPAddress(true);

        PackageManager pm = context.getPackageManager();
        String versionName = "";
        try{
            PackageInfo packageInfo = pm.getPackageInfo(context.getPackageName(), 0);
            versionName = packageInfo.versionName;
        }catch(PackageManager.NameNotFoundException e){
            e.printStackTrace();
        }
        return versionName;

    }

    private void setupUIElements() {
        //setBadgeDrawable(getActivity().getResources().getDrawable( R.drawable.banner));

        setTitle(getString(R.string.app_name) + getVersionName(getActivity())); // Badge, when set, takes precedent
        // over title
        setHeadersState(HEADERS_HIDDEN);
        setHeadersTransitionOnBackEnabled(true);

        // set fastLane (or headers) background color
        setBrandColor(getResources().getColor(R.color.fastlane_background));
        // set search icon color
        setSearchAffordanceColor(getResources().getColor(R.color.search_opaque));
    }

    private void setupEventListeners() {
/*
        setOnSearchClickedListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                Toast.makeText(getActivity(), "Implement your own in-app search", Toast.LENGTH_LONG)
                        .show();
            }
        });
*/
        setOnSearchClickedListener(null);
        setOnItemViewClickedListener(new ItemViewClickedListener());
        setOnItemViewSelectedListener(new ItemViewSelectedListener());
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
    final int SETTING_ACTIVITY = 0x01;
    final int YABAUSE_ACTIVITY = 0x02;
    final int DOWNLOAD_ACTIVITY = 0x03;

    private final class ItemViewClickedListener implements OnItemViewClickedListener {
        @Override
        public void onItemClicked(Presenter.ViewHolder itemViewHolder, Object item,
                                  RowPresenter.ViewHolder rowViewHolder, Row row) {

            if (item instanceof GameInfo) {
                GameInfo game = (GameInfo) item;
                Calendar c = Calendar.getInstance();
                game.lastplay_date = c.getTime();
                game.save();

                if( mTracker != null ) {
                    mTracker.send(new HitBuilders.EventBuilder()
                            .setCategory("Action")
                            .setAction(game.game_title)
                            .build());
                }

                Bundle bundle = new Bundle();
                bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
                bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, game.game_title);
                mFirebaseAnalytics.logEvent(
                        FirebaseAnalytics.Event.SELECT_CONTENT, bundle
                );

                Intent intent = new Intent(getActivity(), Yabause.class);
                intent.putExtra("org.uoyabause.android.FileNameEx", game.file_path );
                intent.putExtra("org.uoyabause.android.gamecode", game.product_number );
                startActivityForResult(intent, YABAUSE_ACTIVITY);
            } else if (item instanceof String) {
                if (((String) item).indexOf(getString(R.string.sign_in)) >= 0) {
                    presenter_.signIn();
                }else if (((String) item).indexOf(getString(R.string.sign_out)) >= 0) {
                    presenter_.signOut();
                }else if (((String) item).indexOf("Backup") >= 0) {
                    Intent intent = new Intent(getActivity(), IsoDownload.class);

                    String savepath;
                    YabauseStorage ys = YabauseStorage.getStorage();
                    if( ys.hasExternalSD() == false ) {
                        savepath = ys.getGamePath();
                    }else{
                        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
                        String sel = sharedPref.getString("pref_game_download_directory","0");
                        if( sel != null ) {
                            if (sel.equals("0")) {   // internal
                                savepath = ys.getGamePath();
                            } else if (sel.equals("1")) { // external
                                savepath = ys.getExternalGamePath();
                            } else {
                                savepath = ys.getGamePath(); // Error
                            }
                        }else{
                            savepath = ys.getGamePath(); // Error
                        }
                    }
                    intent.putExtra("save_path", savepath );
                    startActivityForResult(intent, DOWNLOAD_ACTIVITY );
                    //FragmentTransaction ft = getFragmentManager().beginTransaction();
                    //DownloadDialog newFragment = new DownloadDialog();
                    //newFragment.show(ft, "dialog");
                }else if (((String) item).indexOf(getString(R.string.setting)) >= 0) {
                    Intent intent = new Intent(getActivity(), YabauseSettings.class);
                    startActivityForResult(intent, SETTING_ACTIVITY );
                }else if (((String) item).indexOf(getString(R.string.load_game)) >= 0){

                    File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");
                    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
                    String  last_dir = sharedPref.getString("pref_last_dir", yabroot.getPath());
                    FileDialog fd = new FileDialog(getActivity(),last_dir);
                    fd.addFileListener(GameSelectFragment.this);
                    fd.showDialog();

                }else if( ((String) item).indexOf(getString(R.string.refresh_db)) >= 0 ){
                    refresh_level = 3;
                    if( checkStoragePermission() == 0 ) {
                        updateGameList();
                    }
                //}else if(  ((String) item).indexOf(getString(R.string.donation)) >= 0){
                //    Intent intent = new Intent(getActivity(), DonateActivity.class);
                //    startActivity(intent);
                }else if(  ((String) item).indexOf(getString(R.string.invite)) >= 0){
                    onInviteClicked();
                }else if( ((String) item).indexOf("GoogleDrive") >= 0) {
                    onGoogleDriveClciked();
                }
            }
        }
    }

    private void onGoogleDriveClciked() {

        PackageManager pm = getActivity().getPackageManager();
        try {
            pm.getPackageInfo("org.uoyabause.gdrive", PackageManager.GET_ACTIVITIES);

            Intent intent = new Intent("org.uoyabause.gdrive.LAUNCH");
            startActivity(intent);

        } catch (PackageManager.NameNotFoundException e) {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=org.uoyabause.android"));
            try {
                getActivity().startActivity(intent);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }

    }

    private void onInviteClicked() {
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

    private final class ItemViewSelectedListener implements OnItemViewSelectedListener {
        @Override
        public void onItemSelected(Presenter.ViewHolder itemViewHolder, Object item,
                                   RowPresenter.ViewHolder rowViewHolder, Row row) {
            //if (item instanceof Movie) {
            //   mBackgroundURI = ((Movie) item).getBackgroundImageURI();
            //    startBackgroundTimer();
            //}

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

    private class GridItemPresenter extends Presenter {
        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent) {
            TextView view = new TextView(parent.getContext());
            view.setLayoutParams(new ViewGroup.LayoutParams(GRID_ITEM_WIDTH, GRID_ITEM_HEIGHT));
            view.setFocusable(true);
            view.setFocusableInTouchMode(true);
            view.setBackgroundColor(getResources().getColor(R.color.default_background));
            view.setTextColor(Color.WHITE);
            view.setGravity(Gravity.CENTER);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(ViewHolder viewHolder, Object item) {
            ((TextView) viewHolder.view).setText((String) item);
        }

        @Override
        public void onUnbindViewHolder(ViewHolder viewHolder) {
        }
    }


    public static final int GAMELIST_NEED_TO_UPDATED = 0x8001;
    public static final int GAMELIST_NEED_TO_RESTART = 0x8002;

    @Override
    public void onShowMessage( int string_id ){
        showSnackbar( string_id );
    }

    private void showSnackbar( int id ) {
        Toast.makeText(getActivity(), getString(id), Toast.LENGTH_SHORT).show();
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

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        //Bundle bundle = data.getExtras();
        switch (requestCode) {
            case GameSelectPresenter.RC_SIGN_IN:{
                presenter_.onSignIn(resultCode,data);
            }
            break;

            case DOWNLOAD_ACTIVITY:
                if( resultCode == 0 ){
                    refresh_level = 3;
                    if( checkStoragePermission() == 0 ) {
                        updateGameList();
                    }
                }
            case SETTING_ACTIVITY:
                if( resultCode == GAMELIST_NEED_TO_UPDATED ){
                    refresh_level = 3;
                    if( checkStoragePermission() == 0 ) {
                        updateGameList();
                    }
                    this.updateBackGraound();
                }else if( resultCode == GAMELIST_NEED_TO_RESTART ){
                    Intent intent = new Intent(getActivity(), StartupActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                    getActivity().finish();
                }else{
                    this.updateBackGraound();
                }
                break;
            case YABAUSE_ACTIVITY:
                if( !BuildConfig.BUILD_TYPE.equals("pro") ) {
                    SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
                    Boolean hasDonated = prefs.getBoolean("donated", false);
                    if (hasDonated == false) {
                        double rn = Math.random();
                        if (rn <= 0.5) {
                            UiModeManager uiModeManager = (UiModeManager) getActivity().getSystemService(Context.UI_MODE_SERVICE);
                            if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
                                if (mInterstitialAd.isLoaded()) {
                                    mInterstitialAd.show();
                                } else {
                                    Intent intent = new Intent(getActivity(), AdActivity.class);
                                    startActivity(intent);
                                }
                            } else {
                                Intent intent = new Intent(getActivity(), AdActivity.class);
                                startActivity(intent);
                            }
                        } else if (rn > 0.5) {
                            Intent intent = new Intent(getActivity(), AdActivity.class);
                            startActivity(intent);
                        }
                    }
                }
                break;
            default:
                break;
        }

    }


    private void requestNewInterstitial() {
        AdRequest adRequest = new AdRequest.Builder()
                //.addTestDevice("YOUR_DEVICE_HASH")
                .build();
        mInterstitialAd.loadAd(adRequest);
    }
}

