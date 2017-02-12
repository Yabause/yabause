/*  Copyright 2011-2013 Guillaume Duhamel
	Copyright 2015 devMiyax(Shinya Miyamoto)

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
import java.net.URI;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.io.File;
import java.io.FileOutputStream;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.app.DialogFragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Base64;
import android.util.Log;
import android.view.InputDevice;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.app.ActionBar.OnMenuVisibilityListener;
import android.app.Dialog;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ContentValues;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.view.View;
import android.graphics.Bitmap;
import android.os.Environment;
import android.provider.MediaStore;
import android.net.Uri;
import android.view.Surface;
import android.app.ActivityManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.Switch;
import android.widget.Toast;

import com.bumptech.glide.load.engine.Resource;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.json.JSONObject;

import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Logger;
import com.google.android.gms.analytics.Tracker;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;


class YabauseRunnable implements Runnable
{  
    public static native int init(Yabause yabause);
    public static native void deinit();
    public static native void exec();
    public static native void press(int key, int player);
    public static native void axis(int key, int player, int val);
    public static native void release(int key, int player);
    public static native int initViewport( Surface sf, int width, int hieght);
    public static native int drawScreen();
    public static native int lockGL();
    public static native int unlockGL();
    public static native void enableFPS(int enable);
    public static native void enableFrameskip(int enable);
    public static native void setCpu( int cpu );
    public static native void setFilter( int filter );
    public static native void setVolume(int volume);
    public static native int screenshot( String filename );
    public static native String getCurrentGameCode();
    public static native String getGameinfo();
    public static native void savestate( String path );
    public static native void loadstate( String path );
    public static native void pause();
    public static native void resume();
    public static native void setPolygonGenerationMode( int pg );
    public static native void setSoundEngine( int engine );
    public static native void setResolutionMode( int resoution_mode );
    public static native void openTray();
    public static native void closeTray();

    public static native void switch_padmode( int mode );


    private boolean inited;
    private boolean paused;

    public YabauseRunnable(Yabause yabause)
    {
        int ok = init(yabause);
        Log.v("Yabause", "init = " + ok);
        inited = (ok == 0);
    }  

    public void destroy() 
    {
        Log.v("Yabause", "destroying yabause...");
        inited = false;
        deinit();
    }

    public void run()
    {
        if (inited && (! paused))
        {
            exec();
        }
    }

}

class YabauseHandler extends Handler {
    private Yabause yabause;

    public YabauseHandler(Yabause yabause) {
        this.yabause = yabause;
    }

    public void handleMessage(Message msg) {
        yabause.showDialog(msg.what, msg.getData());
    }
}


public class Yabause extends Activity implements  FileDialog.FileSelectedListener
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
    private AdView mAdView = null;

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

        switch( _report_status){
            case   REPORT_STATE_INIT:
                Toast.makeText(this, "Fail to send your report. internal error", Toast.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_SUCCESS:
                Toast.makeText(this, "Success to send your report. Thank you for your collaboration.", Toast.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_DUPE:
                Toast.makeText(this, "Fail to send your report. You've sent a report for same game, same device and same vesion.", Toast.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_CONNECTION:
                Toast.makeText(this, "Fail to send your report. Server is down.", Toast.LENGTH_SHORT).show();
                break;
            case   REPORT_STATE_FAIL_AUTH:
                Toast.makeText(this, "Fail to send your report. Authorizing is failed.", Toast.LENGTH_SHORT).show();
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
        View v = findViewById(R.id.menu_view);
        v.setVisibility(View.GONE);
        View.OnClickListener ScreenshotClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
                Date date = new Date();

                String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
                        + YabauseRunnable.getCurrentGameCode() +
                        dateFormat.format(date) + ".png";

                if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                    return;
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
                startActivityForResult(Intent.createChooser(shareIntent, "share screenshot to"), R.id.save_screen);
            }
        };
        findViewById(R.id.button_screen_shot).setOnClickListener(ScreenshotClickListener);

        View.OnClickListener SaveStateClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String save_path = YabauseStorage.getStorage().getStateSavePath();
                YabauseRunnable.savestate(save_path);
                Yabause.this.showBottomMenu();
            }
        };
        findViewById(R.id.button_save_sate).setOnClickListener(SaveStateClickListener);

        View.OnClickListener LoadStateClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String save_path = YabauseStorage.getStorage().getStateSavePath();
                YabauseRunnable.loadstate(save_path);
                Yabause.this.showBottomMenu();
            }
        };
        findViewById(R.id.button_load_state).setOnClickListener(LoadStateClickListener);

        View.OnClickListener OpenTrayListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                Button btn = (Button)findViewById(R.id.button_open_cd);
                if( tray_state == 0 ) {
                    YabauseRunnable.openTray();
                    btn.setText("Close CD tray");
                    tray_state = 1;
                }else{
                    btn.setText("Open CD tray");
                    tray_state = 0;
                    File file = new File(gamepath);
                    String path = file.getParent();
                    FileDialog fd = new FileDialog(Yabause.this,path);
                    fd.addFileListener(Yabause.this);
                    fd.showDialog();
                }
                Yabause.this.showBottomMenu();
            }
        };
        findViewById(R.id.button_open_cd).setOnClickListener(OpenTrayListener);


        View.OnClickListener ExitClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                YabauseRunnable.deinit();
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {

                }
                //android.os.Process.killProcess(android.os.Process.myPid());
                finish();
                android.os.Process.killProcess(android.os.Process.myPid());
            }
        };
        findViewById(R.id.button_exit).setOnClickListener(ExitClickListener);

        View.OnClickListener ReportClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startReport();
            }
        };
        findViewById(R.id.button_report).setOnClickListener(ReportClickListener);

        ((Switch)findViewById(R.id.pad_mode_switch)).setOnCheckedChangeListener( new CompoundButton.OnCheckedChangeListener(){
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

                YabausePad padv = (YabausePad)findViewById(R.id.yabause_pad);

               if( isChecked ) {
                   Yabause.this.padm.setAnalogMode(PadManager.MODE_ANALOG);
                   YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG);
                   padv.setPadMode(PadManager.MODE_ANALOG);
               }else{
                   Yabause.this.padm.setAnalogMode(PadManager.MODE_HAT);
                   YabauseRunnable.switch_padmode(PadManager.MODE_HAT);
                   padv.setPadMode(PadManager.MODE_HAT);
               }

                SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean("pref_analog_pad", isChecked);
                editor.apply();

                Yabause.this.showBottomMenu();

            }
        });


        handler = new YabauseHandler(this);
        yabauseThread = new YabauseRunnable(this);

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
        audio.unmute(audio.SYSTEM);
        YabauseRunnable.resume();
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
    	    case R.id.save_screen:
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
                showBottomMenu();
            }
            return true;
        }

        if( menu_showing ){
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
        View v = findViewById(R.id.menu_view);

        if(menu_showing == false) {
            YabauseRunnable.pause();
            audio.mute(audio.SYSTEM);

            v.setVisibility(View.VISIBLE);
            v.setTranslationY(0);
            v.invalidate();

            //ObjectAnimator objectAnimator = ObjectAnimator.ofFloat(v, "translationY", v.getHeight(), 0);
            //objectAnimator.setDuration(1000);
            //objectAnimator.start();

            View button = findViewById(R.id.button_screen_shot);
            button.requestFocus();
            menu_showing = true;

         }else{
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);

            if( mAdView != null )
                mAdView.setVisibility(View.GONE);

            v.setVisibility(View.GONE);
            v.setTranslationY(v.getHeight());
            v.invalidate();

            //ObjectAnimator objectAnimator = ObjectAnimator.ofFloat(v, "translationY", 0, v.getHeight());
            //objectAnimator.setDuration(1000);
            //objectAnimator.start();

            View button = findViewById(R.id.yabause_view);
            button.requestFocus();
            menu_showing = false;
        }
    }

    private String errmsg;
    public void errorMsg(String msg) {
        errmsg = msg;
        Log.d(TAG, "errorMsg " + msg);
        runOnUiThread(new Runnable() {
            public void run() {
                Toast.makeText(Yabause.this, Yabause.this.errmsg, Toast.LENGTH_LONG).show();
            }
        });
/*
        Message message = handler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putString("message", msg);
        message.setData(bundle);
        handler.sendMessage(message);
*/
    }

    private void readPreferences() {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);

        boolean fps = sharedPref.getBoolean("pref_fps", false);
        YabauseRunnable.enableFPS(fps ? 1 : 0);
        Log.d(TAG,"enable FPS " + fps);

        boolean frameskip = sharedPref.getBoolean("pref_frameskip", false);
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
        ((Switch)findViewById(R.id.pad_mode_switch)).setChecked(analog);
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
            View decorView = findViewById(R.id.yabause_view);
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);}
    }

}
