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

package org.uoyabause.android.phone;

import android.Manifest;
import android.app.ActionBar;
import android.app.ProgressDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;
import android.app.Fragment;


import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.core.app.ActivityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.appcompat.widget.Toolbar;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.activeandroid.query.Select;
import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.request.RequestOptions;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.InterstitialAd;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;
import com.google.firebase.analytics.FirebaseAnalytics;

import net.cattaka.android.adaptertoolbox.thirdparty.MergeRecyclerAdapter;

import org.uoyabause.android.AdActivity;
import org.uoyabause.android.DonateActivity;
import org.uoyabause.android.FileDialog;
import org.uoyabause.android.GameInfo;
import org.uoyabause.android.Yabause;
import org.uoyabause.android.YabauseApplication;
import org.uoyabause.android.YabauseSettings;
import org.uoyabause.android.YabauseStorage;
import org.uoyabause.android.GameSelectPresenter;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;
import org.uoyabause.uranus.StartupActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Iterator;
import java.util.List;

import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;

import static org.uoyabause.android.tv.GameSelectFragment.GAMELIST_NEED_TO_UPDATED;
import static org.uoyabause.android.tv.GameSelectFragment.GAMELIST_NEED_TO_RESTART;


public class GameSelectFragmentPhone extends Fragment
        implements GameItemAdapter.OnItemClickListener,
        NavigationView.OnNavigationItemSelectedListener,
        FileDialog.FileSelectedListener,
        GameSelectPresenter.GameSelectPresenterListener {

    private static RecyclerView.Adapter adapter;
    private RecyclerView.LayoutManager layoutManager;
    private static RecyclerView recyclerView;
    private static ArrayList<GameInfo> data;
    static View.OnClickListener myOnClickListener;
    int refresh_level = 0;
    private ProgressDialog mProgressDialog = null;
    MergeRecyclerAdapter mMergeRecyclerAdapter;
    GameIndexAdapter mStringsHeaderAdapter;
    private ActionBarDrawerToggle mDrawerToggle;

    GameSelectPresenter presenter_;

    final int SETTING_ACTIVITY = 0x01;
    final int YABAUSE_ACTIVITY = 0x02;
    final int DOWNLOAD_ACTIVITY = 0x03;
    private static final int RC_SIGN_IN = 123;
    private DrawerLayout mDrawerLayout;
    private InterstitialAd mInterstitialAd;
    private Tracker mTracker;
    private FirebaseAnalytics mFirebaseAnalytics;
    boolean isfisrtupdate = true;
    NavigationView mNavigationView;

    View rootview_;

    String alphabet[]={ "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z" };

    final String TAG = "GameSelectFragmentPhone";

    private static final int REQUEST_STORAGE = 1;
    private static String[] PERMISSIONS_STORAGE = {Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE};


    private OnFragmentInteractionListener mListener;

    public GameSelectFragmentPhone() {
    }

    public static GameSelectFragmentPhone newInstance(String param1, String param2) {
        GameSelectFragmentPhone fragment = new GameSelectFragmentPhone();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        presenter_ = new GameSelectPresenter((Fragment)this,this);
        if (getArguments() != null) {
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        rootview_ = inflater.inflate(R.layout.fragment_game_select_fragment_phone, container, false);
        return rootview_;
    }

    public void onButtonPressed(Uri uri) {
        if (mListener != null) {
            mListener.onFragmentInteraction(uri);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
//            throw new RuntimeException(context.toString()
//                    + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    public interface OnFragmentInteractionListener {
        // TODO: Update argument type and name
        void onFragmentInteraction(Uri uri);
    }

    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {

        mDrawerLayout.closeDrawers();

        //if( item.getTitle().equals(getResources().getString(R.string.donation)) == true ){
        //    Intent intent = new Intent(getActivity(), DonateActivity.class);
        //    startActivity(intent);
        //    return false;
        //}

        int id = item.getItemId();
        switch(id){
            case R.id.menu_item_setting:
                Intent intent = new Intent(getActivity(), YabauseSettings.class);
                startActivityForResult(intent, SETTING_ACTIVITY );
                break;
            case R.id.menu_item_load_game:
                File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");
                SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
                String  last_dir = sharedPref.getString("pref_last_dir", yabroot.getPath());
                FileDialog fd = new FileDialog(getActivity(),last_dir);
                fd.addFileListener(this);
                fd.showDialog();

                break;
            case R.id.menu_item_update_game_db:
                refresh_level = 3;
                if( checkStoragePermission() == 0 ) {
                    updateGameList();
                }
                break;
            case R.id.menu_item_login:
                if( item.getTitle().equals( getString(R.string.sign_out)) == true){
                    presenter_.signOut();
                    item.setTitle(R.string.sign_in);
                }else {
                    presenter_.signIn();
                }
                break;
            case R.id.menu_privacy_policy:
                Uri uri = Uri.parse("http://www.uoyabause.org/static_pages/privacy_policy.html");
                Intent i = new Intent(Intent.ACTION_VIEW,uri);
                startActivity(i);
                break;
        }
        return false;
    }

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
                //
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
                showRestartMessage();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }


    private void showSnackbar( int id ) {
        Snackbar
                .make(rootview_.getRootView(), getString(id), Snackbar.LENGTH_SHORT)
                .show();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        //Bundle bundle = data.getExtras();
        switch (requestCode) {
            case GameSelectPresenter.RC_SIGN_IN:{
                presenter_.onSignIn(resultCode,data);
                if( presenter_.getCurrentUserName() != null ) {
                    Menu m = mNavigationView.getMenu();
                    MenuItem mi_login = m.findItem(R.id.menu_item_login);
                    mi_login.setTitle(R.string.sign_out);
                }
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
                }else if( resultCode == GAMELIST_NEED_TO_RESTART ){
                    Intent intent = new Intent(getActivity(), StartupActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                    getActivity().finish();
                }else{
                }
                break;
            case YABAUSE_ACTIVITY:

                if( !BuildConfig.BUILD_TYPE.equals("pro") )  {
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

    @Override
    public void fileSelected(File file) {
        String apath;
        if( file == null ){ // canceled
            return;
        }

        apath = file.getAbsolutePath();

        YabauseStorage storage = YabauseStorage.getStorage();

        // save last selected dir
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("pref_last_dir", file.getParent());
        editor.apply();

        GameInfo gameinfo = GameInfo.getFromFileName(apath);
        if( gameinfo == null ) {
            if (apath.endsWith("CUE") || apath.endsWith("cue")) {
                gameinfo = GameInfo.genGameInfoFromCUE(apath);
            } else if (apath.endsWith("MDS") || apath.endsWith("mds")) {
                gameinfo = GameInfo.genGameInfoFromMDS(apath);
            } else if (apath.endsWith("CCD") || apath.endsWith("ccd")) {
                gameinfo = GameInfo.genGameInfoFromMDS(apath);
            } else {
                gameinfo = GameInfo.genGameInfoFromIso(apath);
            }
        }
        if( gameinfo != null ) {
            gameinfo.updateState();
            Calendar c = Calendar.getInstance();
            gameinfo.lastplay_date = c.getTime();
            gameinfo.save();
        }else{
            return;
        }

        Bundle bundle = new Bundle();
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title);
        mFirebaseAnalytics.logEvent(
                FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        );
        Intent intent = new Intent(getActivity(), Yabause.class);
        intent.putExtra("org.uoyabause.android.FileNameEx", apath);
        intent.putExtra("org.uoyabause.android.gamecode", gameinfo.product_number);
        startActivity(intent);

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

    public static String getVersionName(Context context){
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

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {

        final AppCompatActivity activity = (AppCompatActivity)getActivity();
        //Log.i(TAG, "onCreate");
        super.onActivityCreated(savedInstanceState);

        mFirebaseAnalytics = FirebaseAnalytics.getInstance(getActivity());
        YabauseApplication application = (YabauseApplication) getActivity().getApplication();
        mTracker = application.getDefaultTracker();

        MobileAds.initialize(getActivity().getApplicationContext(), getString(R.string.ad_app_id));
        mInterstitialAd = new InterstitialAd(getActivity());
        mInterstitialAd.setAdUnitId(getString(R.string.banner_ad_unit_id));
        requestNewInterstitial();

        mInterstitialAd.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                requestNewInterstitial();
            }
        });


        Toolbar toolbar = (Toolbar) rootview_.findViewById(R.id.toolbar);
        toolbar.setLogo(R.mipmap.ic_launcher);
        toolbar.setTitle(getString(R.string.app_name));
        toolbar.setSubtitle(getVersionName(getActivity()));
        activity.setSupportActionBar(toolbar);
        //activity.getSupportActionBar().setTitle(getString(R.string.app_name) + getVersionName(getActivity()));


        recyclerView = (RecyclerView) rootview_.findViewById(R.id.my_recycler_view);
        recyclerView.setHasFixedSize(true);

        layoutManager = new LinearLayoutManager(getActivity());
        recyclerView.setLayoutManager(layoutManager);
        recyclerView.setItemAnimator(new DefaultItemAnimator());

        mDrawerLayout = (DrawerLayout) rootview_.findViewById(R.id.drawer_layout_game_select);
        mDrawerToggle = new ActionBarDrawerToggle(
                getActivity(),                  /* host Activity */
                mDrawerLayout,         /* DrawerLayout object */
                R.string.drawer_open,  /* "open drawer" description */
                R.string.drawer_close  /* "close drawer" description */
        ) {

            /** Called when a drawer has settled in a completely closed state. */
            public void onDrawerClosed(View view) {
                super.onDrawerClosed(view);
                //activity.getSupportActionBar().setTitle("aaa");
            }

            public void onDrawerStateChanged(int newState){

            }

            /** Called when a drawer has settled in a completely open state. */
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);
                //activity.getSupportActionBar().setTitle("bbb");
                TextView tx = (TextView) rootview_.findViewById(R.id.menu_title);
                String uname = presenter_.getCurrentUserName();
                if (tx != null && uname != null) {
                    tx.setText(uname);
                }else{
                    tx.setText("");
                }

                ImageView iv = (ImageView) rootview_.findViewById(R.id.navi_header_image);
                Uri PhotoUri = presenter_.getCurrentUserPhoto();
                if (iv != null && PhotoUri != null) {
                    Glide.with(drawerView.getContext())
                            .load(PhotoUri)
                            .apply(new RequestOptions().transforms(new CenterCrop() ))
                            .into(iv);
                }else{
                    iv.setImageResource(R.mipmap.ic_launcher);
                }
            }
        };

        // Set the drawer toggle as the DrawerListener
        mDrawerLayout.setDrawerListener(mDrawerToggle);

        activity.getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        activity.getSupportActionBar().setHomeButtonEnabled(true);

        mDrawerToggle.syncState();

        mNavigationView = (NavigationView) rootview_.findViewById(R.id.nav_view);
        if (mNavigationView != null) {
            mNavigationView.setNavigationItemSelectedListener(this);
        }

        //SharedPreferences prefs = getActivity().getSharedPreferences("private", Context.MODE_PRIVATE);
        //Boolean hasDonated = prefs.getBoolean("donated", false);
        //if( !hasDonated) {
        //    Menu m = mNavigationView.getMenu();
        //    m.add(getResources().getString(R.string.donation));
        //}

        if( presenter_.getCurrentUserName() != null ) {
            Menu m = mNavigationView.getMenu();
            MenuItem mi_login = m.findItem(R.id.menu_item_login);
            mi_login.setTitle(R.string.sign_out);
        }else{
            Menu m = mNavigationView.getMenu();
            MenuItem mi_login = m.findItem(R.id.menu_item_login);
            mi_login.setTitle(R.string.sign_in);
        }
        //updateGameList();
        if( checkStoragePermission() == 0 ) {
            updateGameList();
        }
    }

    //@Override
    //protected void onPostCreate(Bundle savedInstanceState) {
    //    super.onPostCreate(savedInstanceState);
    //    // Sync the toggle state after onRestoreInstanceState has occurred.
    //    mDrawerToggle.syncState();
    //}



    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Pass the event to ActionBarDrawerToggle, if it returns
        // true, then it has handled the app icon touch event
        if (mDrawerToggle.onOptionsItemSelected(item)) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private Observer observer = null;


    void updateGameList(){

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

    private void showRestartMessage(){
        //need_to_accept
        recyclerView.setLayoutManager(new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false));
        mMergeRecyclerAdapter = new MergeRecyclerAdapter<>(getActivity());
        mStringsHeaderAdapter = new GameIndexAdapter(getString(R.string.need_to_accept));
        mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter);
        recyclerView.setAdapter(mMergeRecyclerAdapter);

    }

    private void loadRows() {

        recyclerView.setLayoutManager(new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false));
        mMergeRecyclerAdapter = new MergeRecyclerAdapter<>(getActivity());


        //-----------------------------------------------------------------
        // Recent Play Game
        List<GameInfo> cheklist = null;
        try {
            cheklist  = new Select()
                    .from(GameInfo.class)
                    .limit(1)
                    .execute();
        }catch(Exception e ){
            System.out.println(e);
        }

        if( cheklist.size() == 0 ){

            String dir = YabauseStorage.getStorage().getGamePath();

            mStringsHeaderAdapter = new GameIndexAdapter("Place a game ISO image file to " + dir + " or select directory in the setting menu");
            mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter);
            recyclerView.setAdapter(mMergeRecyclerAdapter);
            return;
        }


        {   // create strings header adapter
            mStringsHeaderAdapter = new GameIndexAdapter("RECENT");
            mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter);
        }

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
        GameItemAdapter resent_adapter = new GameItemAdapter(rlist);
        resent_adapter.setOnItemClickListener(this);
        mMergeRecyclerAdapter.addAdapter(resent_adapter);

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

        boolean hit;
        int i;
        for (i = 0; i < alphabet.length; i++) {
            hit = false;
            List<GameInfo> alphaindexed_list = new ArrayList<GameInfo>();
            Iterator<GameInfo> it = list.iterator();
            while(it.hasNext()){
                GameInfo game = it.next();
                if( game.game_title.toUpperCase().indexOf(alphabet[i]) == 0  ){
                    alphaindexed_list.add(game);
                    Log.d("GameSelect", alphabet[i] + ":" + game.game_title);
                    it.remove();
                    hit = true;
                }
            }

            if( hit ) {

                mStringsHeaderAdapter = new GameIndexAdapter(alphabet[i]);
                mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter);
                GameItemAdapter alphabet_adapter  = new GameItemAdapter(alphaindexed_list);
                alphabet_adapter.setOnItemClickListener(this);
                mMergeRecyclerAdapter.addAdapter(alphabet_adapter);

            }

        }

        List<GameInfo> others_list = new ArrayList<GameInfo>();
        Iterator<GameInfo> it = list.iterator();
        while(it.hasNext()){
            GameInfo game = it.next();
            Log.d("GameSelect", "Others:" + game.game_title);
            others_list.add(game);
        }
        mStringsHeaderAdapter = new GameIndexAdapter("OTHERS");
        mMergeRecyclerAdapter.addAdapter(mStringsHeaderAdapter);
        GameItemAdapter others_adapter = new GameItemAdapter(others_list);
        others_adapter.setOnItemClickListener(this);
        mMergeRecyclerAdapter.addAdapter(others_adapter);

        recyclerView.setAdapter(mMergeRecyclerAdapter);
    }

    @Override
    public void onItemClick(int position, GameInfo item, View v) {
        Calendar c = Calendar.getInstance();
        item.lastplay_date = c.getTime();
        item.save();

        if( mTracker != null ) {
            mTracker.send(new HitBuilders.EventBuilder()
                    .setCategory("Action")
                    .setAction(item.game_title)
                    .build());
        }

        Bundle bundle = new Bundle();
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, item.game_title);
        mFirebaseAnalytics.logEvent(
                FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        );
        Intent intent = new Intent(getActivity(), Yabause.class);
        intent.putExtra("org.uoyabause.android.FileNameEx", item.file_path );
        intent.putExtra("org.uoyabause.android.gamecode", item.product_number);
        startActivityForResult(intent, YABAUSE_ACTIVITY);

    }
    private void requestNewInterstitial() {
        AdRequest adRequest = new AdRequest.Builder()
                //.addTestDevice("YOUR_DEVICE_HASH")
                .build();
        mInterstitialAd.loadAd(adRequest);
    }


    @Override
    public void onResume(){
        super.onResume();
        if( mTracker != null ) {
            //mTracker.setScreenName(TAG);
            mTracker.send(new HitBuilders.ScreenViewBuilder().build());
        }
    }
    @Override
    public void onPause(){
        dismissDialog();
        super.onPause();
    }
    @Override
    public void onDestroy() {
        System.gc();
        super.onDestroy();
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

    @Override
    public void onShowMessage( int string_id ){showSnackbar( string_id );}

}
