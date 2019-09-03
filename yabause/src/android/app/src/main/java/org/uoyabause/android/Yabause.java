/*  Copyright 2011-2013 Guillaume Duhamel
	Copyright 2015-2017 devMiyax(devmiyax@gmail.com)

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

package org.uoyabause.android;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.Runnable;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.io.File;

import android.app.DialogFragment;
import android.app.ProgressDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.design.widget.Snackbar;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.AppCompatActivity;
import android.util.Base64;
import android.util.Log;
import android.view.MenuItem;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.app.Dialog;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.Surface;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONObject;
import org.uoyabause.android.backup.TabBackupFragment;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.analytics.Tracker;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import android.support.design.widget.NavigationView;


class YabauseHandler extends Handler {
    private Yabause yabause;

    public YabauseHandler(Yabause yabause) {
        this.yabause = yabause;
    }

    public void handleMessage(Message msg) {
        yabause.showDialog(msg.what, msg.getData());
    }
}


public class Yabause extends AppCompatActivity implements  FileDialog.FileSelectedListener, NavigationView.OnNavigationItemSelectedListener
{
    private static final String TAG = "Yabause";
    private YabauseRunnable yabauseThread;
    private YabauseHandler handler;
    private YabauseAudio audio;
    private String biospath;
    private String gamepath;
    private int carttype;
    private PadManager padm;
    private int video_interface;
    private boolean waiting_reault = false;
    private Tracker mTracker;
    private int tray_state = 0;
    DrawerLayout mDrawerLayout;
    NavigationView mNavigationView;
    AdView adView = null;

    private ProgressDialog mProgressDialog;
    private Boolean isShowProgress;
    public void showDialog() {
        mProgressDialog = new ProgressDialog(this);
        mProgressDialog.setMessage("Sending...");
        mProgressDialog.show();
        waiting_reault = true;
        isShowProgress = true;
    }
    public void dismissDialog() {
        mProgressDialog.dismiss();
        mProgressDialog = null;
        isShowProgress = false;
        waiting_reault = false;
        final DrawerLayout layout = (DrawerLayout) findViewById(R.id.drawer_layout);
        switch( _report_status){
            case   REPORT_STATE_INIT:
                Snackbar.make(layout, "Fail to send your report. internal error", Snackbar.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_SUCCESS:
                Snackbar.make(layout, "Success to send your report. Thank you for your collaboration.", Snackbar.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_DUPE:
                Snackbar.make(layout, "Fail to send your report. You've sent a report for same game, same device and same vesion.", Snackbar.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_CONNECTION:
                Snackbar.make(layout, "Fail to send your report. Server is down.", Snackbar.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_AUTH:
                Snackbar.make(layout, "Fail to send your report. Authorizing is failed.", Snackbar.LENGTH_SHORT).show();
                break;

        }
        showBottomMenu();
    }


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)    
    {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
        boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
        if( lock_landscape == true ){
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }else{
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        }

        super.onCreate(savedInstanceState);
        System.gc();

        YabauseApplication application = (YabauseApplication) getApplication();
        mTracker = application.getDefaultTracker();

        setContentView(R.layout.main);
        getWindow().setSustainedPerformanceMode(true);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON|WindowManager.LayoutParams.FLAG_FULLSCREEN);
        mDrawerLayout = (DrawerLayout)findViewById(R.id.drawer_layout);
        mDrawerLayout.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        mNavigationView = (NavigationView)findViewById(R.id.nav_view);
        mNavigationView.setNavigationItemSelectedListener(this);
        if( sharedPref.getBoolean("pref_analog_pad", false) == true) {
            mNavigationView.setCheckedItem(R.id.pad_mode);
        }

        DrawerLayout.DrawerListener drawerListener = new DrawerLayout.DrawerListener() {
            @Override
            public void onDrawerSlide(View view, float v) {

            }

            @Override
            public void onDrawerOpened(View view) {
                if( menu_showing == false ) {
                      menu_showing = true;
                    YabauseRunnable.pause();
                    audio.mute(audio.SYSTEM);
                    String name = YabauseRunnable.getGameTitle();
                    TextView tx = (TextView) findViewById(R.id.menu_title);
                    if (tx != null) {
                        tx.setText(name);
                    }

                    if( adView != null ) {
                        LinearLayout lp = (LinearLayout) findViewById(R.id.navilayer);
                        if( lp != null ) {
                            final int mCount = lp.getChildCount();
                            boolean find = false;
                            for (int i = 0; i < mCount; ++i) {
                                final View mChild = lp.getChildAt(i);
                                if (mChild == adView) {
                                    find = true;
                                }
                            }
                            if (find == false) {
                                lp.addView(adView);
                            }
                            AdRequest adRequest = new AdRequest.Builder().addTestDevice("303A789B146C169D4BDB5652D928FF8E").build();
                            adView.loadAd(adRequest);
                        }
                    }


                }
            }

            @Override
            public void onDrawerClosed(View view) {

                if( waiting_reault == false && menu_showing == true ) {
                    menu_showing = false;
                    YabauseRunnable.resume();
                    audio.unmute(audio.SYSTEM);
                }

            }

            @Override
            public void onDrawerStateChanged(int i) {
            }
        };
        this.mDrawerLayout.setDrawerListener(drawerListener);


        audio = new YabauseAudio(this);

        Intent intent = getIntent();
        String game = intent.getStringExtra("org.uoyabause.android.FileName");
        if (game != null && game.length() > 0) {
            YabauseStorage storage = YabauseStorage.getStorage();
            gamepath = storage.getGamePath(game);
        } else
            gamepath = "";

        String exgame = intent.getStringExtra("org.uoyabause.android.FileNameEx");
        if( exgame != null ){
        	gamepath = exgame;  
        }


        PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
        readPreferences();

        padm = PadManager.getPadManager();
        padm.loadSettings();
        waiting_reault = false;

        handler = new YabauseHandler(this);
        yabauseThread = new YabauseRunnable(this);

        UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
    if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION && !BuildConfig.BUILD_TYPE.equals("pro") ) {
            SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
            Boolean hasDonated = prefs.getBoolean("donated", false);
            if( hasDonated ) {
                adView = null;
            }else {
                adView = new AdView(this);
                adView.setAdUnitId( getString(R.string.banner_ad_unit_id2) );
                adView.setAdSize(AdSize.BANNER);
                AdRequest adRequest = new AdRequest.Builder().build();
                adView.loadAd(adRequest);
                adView.setAdListener(new AdListener() {
                    @Override
                    public void onAdOpened() {
                        // Save app state before going to the ad overlay.
                    }
                });
            }
        }else{
            adView = null;
        }


    }

    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        switch(id){
            /*
            case R.id.save_screen:{
                DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
                Date date = new Date();

                String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
                        + YabauseRunnable.getCurrentGameCode() +
                        dateFormat.format(date) + ".png";

                if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                    break;
                }

                waiting_reault = true;
                Intent shareIntent = new Intent();
                shareIntent.setAction(Intent.ACTION_SEND);
                ContentResolver cr = getContentResolver();
                ContentValues cv = new ContentValues();
                cv.put(MediaStore.Images.Media.TITLE, YabauseRunnable.getCurrentGameCode());
                cv.put(MediaStore.Images.Media.DISPLAY_NAME, YabauseRunnable.getCurrentGameCode());
                cv.put(MediaStore.Images.Media.DATE_TAKEN, System.currentTimeMillis());
                cv.put(MediaStore.Images.Media.MIME_TYPE, "image/png");
                cv.put(MediaStore.Images.Media.DATA, screen_shot_save_path);
                Uri uri = cr.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, cv);
                shareIntent.putExtra(Intent.EXTRA_STREAM, uri);
                shareIntent.setType("image/png");
                startActivityForResult(Intent.createChooser(shareIntent, "share screenshot to"), 0x01);
            }
            break;
            */
            case R.id.reset:
                YabauseRunnable.reset();
                break;
            case R.id.report:
                startReport();
                break;
            case R.id.save_state: {
                String save_path = YabauseStorage.getStorage().getStateSavePath();
                String current_gamecode = YabauseRunnable.getCurrentGameCode();
                File save_root = new File(YabauseStorage.getStorage().getStateSavePath(),current_gamecode);
                if (! save_root.exists()) save_root.mkdir();

                String save_filename = YabauseRunnable.savestate(save_path+current_gamecode);
                if( save_filename != "" ){
                    int point = save_filename.lastIndexOf(".");
                    if (point != -1) {
                        save_filename = save_filename.substring(0, point);
                    }
                    String screen_shot_save_path = save_filename + ".png";
                    if (YabauseRunnable.screenshot(screen_shot_save_path) != 0){
                        Snackbar.make(this.mDrawerLayout, "Failed to save the current state", Snackbar.LENGTH_SHORT).show();
                    }else {
                        Snackbar.make(this.mDrawerLayout, "Current state is saved as " + save_filename, Snackbar.LENGTH_LONG).show();
                    }
                }else{
                    Snackbar.make(this.mDrawerLayout, "Failed to save the current state", Snackbar.LENGTH_SHORT).show();
                }

                StateListFragment.checkMaxFileCount(save_path + current_gamecode);

            }
            break;
            case R.id.load_state: {
                //String save_path = YabauseStorage.getStorage().getStateSavePath();
                //YabauseRunnable.loadstate(save_path);
                String basepath;
                String save_path = YabauseStorage.getStorage().getStateSavePath();
                String current_gamecode = YabauseRunnable.getCurrentGameCode();
                basepath = save_path + current_gamecode;
                waiting_reault = true;
                FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                StateListFragment fragment = new StateListFragment();
                fragment.setBasePath(basepath);
                transaction.replace(R.id.ext_fragment, fragment, StateListFragment.TAG );
                transaction.show(fragment);
                transaction.commit();
            }
            break;
            case R.id.menu_item_backup: {
                waiting_reault = true;
                FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                TabBackupFragment fragment = TabBackupFragment.newInstance("hoge","hoge");
                //fragment.setBasePath(basepath);
                transaction.replace(R.id.ext_fragment, fragment, TabBackupFragment.TAG );
                transaction.show(fragment);
                transaction.commit();
            }
            break;
            case R.id.button_open_cd: {
                if (tray_state == 0) {
                    YabauseRunnable.openTray();
                    item.setTitle(getString(R.string.close_cd_tray));
                    tray_state = 1;
                } else {
                    item.setTitle(getString(R.string.open_cd_tray));
                    tray_state = 0;
                    File file = new File(gamepath);
                    String path = file.getParent();
                    FileDialog fd = new FileDialog(Yabause.this, path);
                    fd.addFileListener(Yabause.this);
                    fd.showDialog();
                }
            }
            break;
            case R.id.pad_mode:{
                YabausePad padv = (YabausePad)findViewById(R.id.yabause_pad);
                boolean mode = false;
                if (item.isChecked()){
                    item.setChecked(false);
                    Yabause.this.padm.setAnalogMode(PadManager.MODE_HAT);
                    YabauseRunnable.switch_padmode(PadManager.MODE_HAT);
                    padv.setPadMode(PadManager.MODE_HAT);
                    mode = false;
                }else{
                    item.setChecked(true);
                    Yabause.this.padm.setAnalogMode(PadManager.MODE_ANALOG);
                    YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG);
                    padv.setPadMode(PadManager.MODE_ANALOG);
                    mode = true;
                }
                SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean("pref_analog_pad", mode);
                editor.apply();
                Yabause.this.showBottomMenu();
            }
            break;
            case R.id.menu_item_cheat: {
                waiting_reault = true;
                CheatEditDialogStandalone newFragment = new CheatEditDialogStandalone();
                newFragment.setGameCode(YabauseRunnable.getCurrentGameCode(),this.cheat_codes);
                newFragment.show(getFragmentManager(), "Cheat");
            }
            break;
            case R.id.exit: {
                YabauseRunnable.deinit();
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {

                }
                //android.os.Process.killProcess(android.os.Process.myPid());
                finish();
                android.os.Process.killProcess(android.os.Process.myPid());
            }
            break;
        }
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    @Override
    // after disc change event
    public void fileSelected(File file) {

        if( file != null ) {
            gamepath = file.getAbsolutePath();
        }
        Button btn = (Button)findViewById(R.id.button_open_cd);
        YabauseRunnable.closeTray();
    }

    @Override
    public void onPause()
    {
        YabauseRunnable.pause();
        audio.mute(audio.SYSTEM);
        super.onPause();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        if( mTracker != null ) {
            mTracker.setScreenName(TAG);
            mTracker.send(new HitBuilders.ScreenViewBuilder().build());
        }

        if( waiting_reault == false  ) {
            audio.unmute(audio.SYSTEM);
            YabauseRunnable.resume();
        }
    }

    @Override
    public void onDestroy()
    {

    	Log.v(TAG, "this is the end...");
        yabauseThread.destroy();
        super.onDestroy();
    } 

    @Override
    public Dialog onCreateDialog(int id, Bundle args) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(args.getString("message"))
            .setCancelable(false)
            .setNegativeButton(R.string.exit, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    Yabause.this.finish();
                }
            })
            .setPositiveButton(R.string.ignore, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    dialog.cancel();
                }
            });
        AlertDialog alert = builder.create(); 
        return alert; 
    }

    void startReport(){
        waiting_reault = true;
        DialogFragment newFragment = new ReportDialog();
        newFragment.show(getFragmentManager(), "Report");
    }

    public void onFinishReport(){

    }

    class ReportContents{
        public int _rating;
        public String _message;
        public boolean _screenshot;
        public String _screenshot_base64;
        public String _screenshot_save_path;
    }

    public ReportContents current_report;
    public JSONObject current_game_info;

    static final int REPORT_STATE_INIT= 0;
    static final int REPORT_STATE_SUCCESS = 1;
    static final int REPORT_STATE_FAIL_DUPE = -1;
    static final int REPORT_STATE_FAIL_CONNECTION = -2;
    static final int REPORT_STATE_FAIL_AUTH = -3;
    public int _report_status = REPORT_STATE_INIT;

    String[] cheat_codes = null;
    void updateCheatCode( String[] cheat_codes ){
        this.cheat_codes = cheat_codes;
        int index = 0;
        ArrayList<String> send_codes = new ArrayList<String>();
        for( int i=0; i<cheat_codes.length; i++  ){
            String[] tmp = cheat_codes[i].split("\n", -1);
            for( int j =0; j<tmp.length; j++ ){
                send_codes.add(tmp[j]);
            }
        }
        String[] cheat_codes_array = (String[])send_codes.toArray(new String[0]);
        YabauseRunnable.updateCheat(cheat_codes_array);

        if( waiting_reault ) {
            waiting_reault = false;
            menu_showing = false;
            View mainview = (View)findViewById(R.id.yabause_view);
            mainview.requestFocus();
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
        }
    }

    public void cancelStateLoad(){
        if( waiting_reault ) {
            waiting_reault = false;
            menu_showing = false;
            View mainview = (View)findViewById(R.id.yabause_view);
            mainview.requestFocus();
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
        }
    }

    public void loadState( String filename ){

        YabauseRunnable.loadstate(filename);

        Fragment fg = getSupportFragmentManager().findFragmentByTag(StateListFragment.TAG);
        if( fg != null ){
            FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
            transaction.remove(fg);
            transaction.commit();
        }

        if( waiting_reault ) {
            waiting_reault = false;
            menu_showing = false;
            View mainview = (View)findViewById(R.id.yabause_view);
            mainview.requestFocus();
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
        }
    }

    void doReportCurrentGame( int rating, String message, boolean screenshot ){
        current_report = new ReportContents();
        current_report._rating = rating;
        current_report._message = message;
        current_report._screenshot = screenshot;
        _report_status = REPORT_STATE_INIT;

        String gameinfo = YabauseRunnable.getGameinfo();
        if (gameinfo != null){

            try {

                showDialog();
                AsyncReport asyncTask = new AsyncReport(this);
                current_game_info = new JSONObject(gameinfo);

                DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
                Date date = new Date();

                String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
                        + YabauseRunnable.getCurrentGameCode() +
                        dateFormat.format(date) + ".png";

                if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                    dismissDialog();
                    return;
                }

                InputStream inputStream = new FileInputStream(screen_shot_save_path);//You can get an inputStream using any IO API
                byte[] bytes;
                byte[] buffer = new byte[8192];
                int bytesRead;
                ByteArrayOutputStream output = new ByteArrayOutputStream();
                try {
                    while ((bytesRead = inputStream.read(buffer)) != -1) {
                        output.write(buffer, 0, bytesRead);
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    dismissDialog();
                    return;
                }
                bytes = output.toByteArray();
                String encodedString = Base64.encodeToString(bytes, Base64.DEFAULT);


                JSONObject jsonObjimg = new JSONObject();
                jsonObjimg.put("data", encodedString);
                jsonObjimg.put("filename", screen_shot_save_path);
                jsonObjimg.put("content_type", "image/png");
                JSONObject jsonObjgame = current_game_info.getJSONObject("game");
                jsonObjgame.put("title_image", jsonObjimg);

                if( screenshot ){
                    current_report._screenshot_base64 = encodedString;
                    current_report._screenshot_save_path = screen_shot_save_path;
                }
                //asyncTask.execute("http://192.168.0.7:3000/api/", YabauseRunnable.getCurrentGameCode());
                asyncTask.execute("http://www.uoyabause.org/api/", YabauseRunnable.getCurrentGameCode() );

                return;

            }catch( Exception e){
                e.printStackTrace();
            }
        }
    }
    void cancelReportCurrentGame(){
        waiting_reault = false;
        YabauseRunnable.resume();
        audio.unmute(audio.SYSTEM);
    }

    @Override
    protected void onActivityResult( int requestCode, int resultCode, Intent data) {
    	switch(requestCode){
    	    case 0x01:
                 waiting_reault = false;
                 showBottomMenu();
                break;
    	}
    }
     
    @Override public boolean onGenericMotionEvent(MotionEvent event) {

        if( menu_showing ){
            return super.onGenericMotionEvent(event);
        }

    	int rtn = padm.onGenericMotionEvent(event);
        if (rtn != 0) {
            return true;
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent (KeyEvent event){

        int action =event.getAction();
        int keyCode = event.getKeyCode();

        if ( keyCode == KeyEvent.KEYCODE_BACK) {

            if( event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0 ){

                Fragment fg = getSupportFragmentManager().findFragmentByTag(StateListFragment.TAG);
                if( fg != null ){
                    this.cancelStateLoad();
                    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                    transaction.remove(fg);
                    transaction.commit();
                    View mainv = findViewById(R.id.yabause_view);
                    mainv.setActivated(true);
                    mainv.requestFocus();
                    waiting_reault = false;
                    YabauseRunnable.resume();
                    audio.unmute(audio.SYSTEM);
                    return true;
                }

                fg = getSupportFragmentManager().findFragmentByTag(TabBackupFragment.TAG);
                if( fg != null ){
                     FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                     transaction.remove(fg);
                     transaction.commit();
                    View mainv = findViewById(R.id.yabause_view);
                    mainv.setActivated(true);
                    mainv.requestFocus();
                    waiting_reault = false;
                    YabauseRunnable.resume();
                    audio.unmute(audio.SYSTEM);
                    return true;
                }
                showBottomMenu();
            }
            return true;
        }

        if( menu_showing ){
            return super.dispatchKeyEvent(event);
        }

        if( this.waiting_reault ){
            return super.dispatchKeyEvent(event);
        }

    	//Log.d("dispatchKeyEvent","device:" + event.getDeviceId() + ",action:" + action +",keyCoe:" + keyCode );
    	if( action == KeyEvent.ACTION_UP){
            int rtn = padm.onKeyUp(keyCode, event);
            if (rtn != 0) {
                return true;
            }   		
    	}else if( action == KeyEvent.ACTION_MULTIPLE ){
    		
    	}else if( action == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0 ){
            int rtn =  padm.onKeyDown(keyCode, event);
            if (rtn != 0) {
                return true;
            }  
    	}
    	return super.dispatchKeyEvent(event);
    }

    @Override
    public void onTrimMemory (int level){
        super.onTrimMemory(level);
        switch(level){
            case TRIM_MEMORY_RUNNING_MODERATE:
                break;
            case TRIM_MEMORY_RUNNING_LOW :
                break;
            case TRIM_MEMORY_RUNNING_CRITICAL :
                break;
            case TRIM_MEMORY_UI_HIDDEN :
                break;
            case TRIM_MEMORY_BACKGROUND :
                break;
            case TRIM_MEMORY_COMPLETE :
                break;
        }

    }

    private boolean menu_showing = false;
    private void showBottomMenu(){
        if ( menu_showing == true ) {
            menu_showing = false;
            View mainview = (View)findViewById(R.id.yabause_view);
            mainview.requestFocus();
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
             this.mDrawerLayout.closeDrawer(GravityCompat.START);
        } else {
            menu_showing = true;
            YabauseRunnable.pause();
            audio.mute(audio.SYSTEM);
            String name = YabauseRunnable.getGameTitle();
            TextView tx = (TextView)findViewById(R.id.menu_title);
            if( tx != null ) {
                tx.setText(name);
            }
            if( adView != null ) {
                LinearLayout lp = (LinearLayout) findViewById(R.id.navilayer);
                if( lp != null ) {
                    final int mCount = lp.getChildCount();
                    boolean find = false;
                    for (int i = 0; i < mCount; ++i) {
                        final View mChild = lp.getChildAt(i);
                        if (mChild == adView) {
                            find = true;
                        }
                    }
                    if (find == false) {
                        lp.addView(adView);
                    }
                    AdRequest adRequest = new AdRequest.Builder().addTestDevice("303A789B146C169D4BDB5652D928FF8E").build();
                    adView.loadAd(adRequest);
                }
            }
            this.mDrawerLayout.openDrawer(GravityCompat.START);
        }
    }

    private String errmsg;
    public void errorMsg(String msg) {
        errmsg = msg;
        Log.d(TAG, "errorMsg " + msg);
        runOnUiThread(new Runnable() {
            public void run() {
                final DrawerLayout layout = (DrawerLayout) findViewById(R.id.drawer_layout);
                Snackbar.make(layout, Yabause.this.errmsg, Snackbar.LENGTH_SHORT) .show();
            }
        });

    }

    private void readPreferences() {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);

        boolean extmemory = sharedPref.getBoolean("pref_extend_internal_memory", true);
        YabauseRunnable.enableExtendedMemory(extmemory ? 1 : 0);
        Log.d(TAG,"enable Extended Memory " + extmemory);


    YabauseRunnable.enableComputeShader(sharedPref.getBoolean("pref_use_compute_shader", false) ? 1 : 0);

        YabauseRunnable.enableRotateScreen(sharedPref.getBoolean("pref_rotate_screen", false) ? 1 : 0);

        boolean fps = sharedPref.getBoolean("pref_fps", false);
        YabauseRunnable.enableFPS(fps ? 1 : 0);
        Log.d(TAG,"enable FPS " + fps);

        boolean frameskip = sharedPref.getBoolean("pref_frameskip", true);
        YabauseRunnable.enableFrameskip(frameskip ? 1 : 0);
        Log.d(TAG, "enable enableFrameskip " + frameskip);

        String cpu = sharedPref.getString("pref_cpu", "2");
        Integer icpu = new Integer(cpu);
        YabauseRunnable.setCpu(icpu.intValue());
        Log.d(TAG, "cpu " + icpu.toString());

        String sfilter = sharedPref.getString("pref_filter", "0");
        Integer ifilter = new Integer(sfilter);
        YabauseRunnable.setFilter(ifilter);
        Log.d(TAG, "setFilter " + ifilter.toString());

        String sPg = sharedPref.getString("pref_polygon_generation", "0");
        Integer iPg = new Integer(sPg);
        YabauseRunnable.setPolygonGenerationMode(iPg);
        Log.d(TAG, "setPolygonGenerationMode " + iPg.toString());


    boolean keep_aspectrate = sharedPref.getBoolean("pref_keepaspectrate", true);

    if(keep_aspectrate) {
      YabauseRunnable.setKeepAspect(1);
    }else {
      YabauseRunnable.setKeepAspect(0);
    }

        boolean audioout = sharedPref.getBoolean("pref_audio", true);
        if (audioout) {
            audio.unmute(audio.USER);
        } else {
            audio.mute(audio.USER);
        }
        Log.d(TAG, "Audio " + audioout);

        String bios = sharedPref.getString("pref_bios", "");
        if (bios.length() > 0) {
            YabauseStorage storage = YabauseStorage.getStorage();
            biospath = storage.getBiosPath(bios);
        } else
            biospath = "";

        Log.d(TAG, "bios " + bios);

        String cart = sharedPref.getString("pref_cart", "");
        if (cart.length() > 0) {
            Integer i = new Integer(cart);
            carttype = i.intValue();
        } else
            carttype = -1;

        Log.d(TAG, "cart " + cart);

        final ActivityManager activityManager = (ActivityManager) getSystemService(this.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000;

        String video;

        if( supportsEs3 ) {
          video = sharedPref.getString("pref_video", "1");
        }else{
          video = sharedPref.getString("pref_video", "2");
        }
        if (video.length() > 0) {
            Integer i = new Integer(video);
            video_interface = i.intValue();
        } else {
            video_interface = -1;
        }

        Log.d(TAG, "video " + video);

    Integer rbg_resolution_setting = new Integer(sharedPref.getString("pref_rbg_resolution", "0"));
    YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting);


        // InputDevice
    	String selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
    	padm = PadManager.getPadManager();
        Log.d(TAG, "input " + selInputdevice);
    	// First time
    	if( selInputdevice.equals("65535") ){
    		// if game pad is connected use it.
    		if( padm.getDeviceCount() > 0 ){
    			 padm.setPlayer1InputDevice(null);  
    			 Editor editor = sharedPref.edit();
    			 editor.putString("pref_player1_inputdevice", padm.getId(0) );
    			 editor.commit();
    		// if no game pad is detected use on-screen game pad. 
    		}else{
   			 	Editor editor = sharedPref.edit();
   			 	editor.putString("pref_player1_inputdevice", Integer.toString(-1) );
   			 	editor.commit();
    		}
    	}

        YabausePad pad = (YabausePad)findViewById(R.id.yabause_pad);

        if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
            pad.show(false);
            Log.d(TAG, "ScreenPad Disable");
        	padm.setPlayer1InputDevice(selInputdevice);
        }else{
            pad.show(true);
            Log.d(TAG, "ScreenPad Enable");
            padm.setPlayer1InputDevice( null );
        }


        String selInputdevice2 = sharedPref.getString("pref_player2_inputdevice", "65535");
        if( !selInputdevice.equals("65535") ){
        	padm.setPlayer2InputDevice( selInputdevice2 );
        }else{
        	padm.setPlayer2InputDevice( null );
        }
        Log.d(TAG, "input " + selInputdevice2);
        Log.d(TAG, "getGamePath " + getGamePath());
        Log.d(TAG, "getMemoryPath " + getMemoryPath());
        Log.d(TAG, "getCartridgePath " + getCartridgePath());

        String ssound = sharedPref.getString("pref_sound_engine", "1");
        Integer isound = new Integer(ssound);
        YabauseRunnable.setSoundEngine(isound);
        Log.d(TAG, "setSoundEngine " + isound.toString());

        boolean analog = sharedPref.getBoolean("pref_analog_pad", false);
        //((Switch)findViewById(R.id.pad_mode_switch)).setChecked(analog);
        Log.d(TAG, "analog pad? " + analog);
        YabausePad padv = (YabausePad)findViewById(R.id.yabause_pad);

        if( analog ) {
            Yabause.this.padm.setAnalogMode(PadManager.MODE_ANALOG);
            YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG);
            padv.setPadMode(PadManager.MODE_ANALOG);
        }else{
            Yabause.this.padm.setAnalogMode(PadManager.MODE_HAT);
            YabauseRunnable.switch_padmode(PadManager.MODE_HAT);
            padv.setPadMode(PadManager.MODE_HAT);
        }

        Integer resolution_setting =  new Integer(sharedPref.getString("pref_resolution","0"));
        YabauseRunnable.setResolutionMode(resolution_setting);

        Integer scsp_sync =  new Integer(sharedPref.getString("pref_scsp_sync_per_frame","1"));
        YabauseRunnable.setScspSyncPerFrame(scsp_sync);

        Integer cpu_sync =  new Integer(sharedPref.getString("pref_cpu_sync_per_line","1"));
        YabauseRunnable.setCpuSyncPerLine(cpu_sync);

        Integer scsp_time_sync =  new Integer(sharedPref.getString("scsp_time_sync_mode","1"));
        YabauseRunnable.setScspSyncTimeMode(scsp_time_sync);

    }

    public String getBiosPath() {
        return biospath;
    }

    public String getGamePath() {  
        return gamepath;
    }

    public String getMemoryPath() {
        return YabauseStorage.getStorage().getMemoryPath("memory.ram");
    }

    public int getCartridgeType() {
        return carttype;
    }

    public int getVideoInterface() {
      return video_interface;
    }
    
    public int getPlayer2InputDevice(){
    	return padm.getPlayer2InputDevice();
    }

    public String getCartridgePath() {
        return YabauseStorage.getStorage().getCartridgePath(Cartridge.getDefaultFilename(carttype));
    }
 
    static {
        System.loadLibrary("yabause_native");   
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            View decorView = findViewById(R.id.drawer_layout);
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);}
    }

}
