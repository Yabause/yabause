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

import java.lang.Runnable;
import java.io.File;
import java.io.FileOutputStream;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.InputDevice;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.app.Dialog;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
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
import android.content.pm.ConfigurationInfo;

class InputHandler extends Handler {
    private YabauseRunnable yr;

    public InputHandler(YabauseRunnable yr) {
        this.yr = yr;
    }

    public void handleMessage(Message msg) {
        //Log.v("Yabause", "received message: " + msg.arg1 + " " + msg.arg2);
        if (msg.arg1 == 0) {
        	YabauseRunnable.press(msg.arg2);
        } else if (msg.arg1 == 1) {
        	YabauseRunnable.release(msg.arg2);
        }
    }
}  

class YabauseRunnable implements Runnable
{  
    public static native int init(Yabause yabause);
    public static native void deinit();
    public static native void exec();
    public static native void press(int key);
    public static native void release(int key);
    public static native int initViewport( Surface sf, int width, int hieght);
    public static native int drawScreen();
    public static native int lockGL();
    public static native int unlockGL();
    public static native void enableFPS(int enable);
    public static native void enableFrameskip(int enable);
    public static native void setVolume(int volume);
    public static native void screenshot(Bitmap bitmap);
    public static native void savestate( String path );
    public static native void loadstate( String path );
    public static native void pause();
    public static native void resume();

    private boolean inited;
    private boolean paused;
    public InputHandler handler;

    public YabauseRunnable(Yabause yabause)
    {
        handler = new InputHandler(this);
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

            handler.post(this);
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

public class Yabause extends Activity implements OnPadListener
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

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);
        
        // Immersive mode 
        View decor = this.getWindow().getDecorView();
        decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION  | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE);
        

        audio = new YabauseAudio(this);

        PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
        readPreferences();
 
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
        
        handler = new YabauseHandler(this);
        yabauseThread = new YabauseRunnable(this);

        padm = PadManager.getPadManager();

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

        readPreferences();
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
            .setNegativeButton("Exit", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    Yabause.this.finish();
                }
            })
            .setPositiveButton("Ignore", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    dialog.cancel();
                }
            });
        AlertDialog alert = builder.create(); 
        return alert; 
    }

    @Override public boolean onPad(PadEvent event) {
        //Message message = handler.obtainMessage();
        //message.arg1 = event.getAction();
        //message.arg2 = event.getKey();
        //yabauseThread.handler.sendMessage(message);
        return true;
    }

    @Override public boolean onGenericMotionEvent(MotionEvent event) {

    	int rtn = padm.onGenericMotionEvent(event);
        if (rtn != 0) {
            return false;
        }
        return super.onGenericMotionEvent(event);
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.game_menu, menu);
        return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.exit:
            {
            	YabauseRunnable.deinit();
            	try{
            		Thread.sleep(1000);
            	}catch(InterruptedException e){
            		
            	}
            	//moveTaskToBack(true);
            	//finish();
            	android.os.Process.killProcess(android.os.Process.myPid());
                return true;
            }
            case R.id.save_state:   
            {
            	String save_path = YabauseStorage.getStorage().getStateSavePath();
            	YabauseRunnable.savestate(save_path);
            	return true;
            }
            case R.id.load_state:
            {
            	String save_path = YabauseStorage.getStorage().getStateSavePath();
            	YabauseRunnable.loadstate(save_path);
            	return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
        
        
    }    
    
    @Override public boolean onKeyDown(int keyCode, KeyEvent event) {
       
        int rtn =  padm.onKeyDown(keyCode, event);
        if (rtn != 0) {
            return true;
        }
        if ( keyCode == KeyEvent.KEYCODE_BACK) {
            openOptionsMenu();
            return false;
        } 
        return super.onKeyDown(keyCode, event);
    }

    @Override public boolean onKeyUp(int keyCode, KeyEvent event) {
        int rtn = padm.onKeyUp(keyCode, event);
        if (rtn != 0) {
            return true;
        }

        return super.onKeyUp(keyCode, event);
    }

    private void errorMsg(String msg) {
        Message message = handler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putString("message", msg);
        message.setData(bundle);
        handler.sendMessage(message);
    }

    private void readPreferences() {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        boolean fps = sharedPref.getBoolean("pref_fps", false);
        YabauseRunnable.enableFPS(fps ? 1 : 0);

        boolean frameskip = sharedPref.getBoolean("pref_frameskip", false);
        YabauseRunnable.enableFrameskip(frameskip ? 1 : 0);

        boolean audioout = sharedPref.getBoolean("pref_audio", true);
        if (audioout) {
            audio.unmute(audio.USER);
        } else {
            audio.mute(audio.USER);
        }

        String bios = sharedPref.getString("pref_bios", "");
        if (bios.length() > 0) {
            YabauseStorage storage = YabauseStorage.getStorage();
            biospath = storage.getBiosPath(bios);
        } else
            biospath = ""; 

        String cart = sharedPref.getString("pref_cart", "");
        if (cart.length() > 0) {
            Integer i = new Integer(cart);
            carttype = i.intValue();
        } else
            carttype = -1;

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

        // InputDevice
        YabausePad pad = (YabausePad) findViewById(R.id.yabause_pad);
        pad.setOnPadListener(this);
    	String selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
    	padm = PadManager.getPadManager();
    	
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
    	
        if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
            pad.setVisibility(View.INVISIBLE);
        	padm.setPlayer1InputDevice( selInputdevice );
        }else{
            padm.setPlayer1InputDevice( null );
        }
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
        	View decor = this.getWindow().getDecorView();
        	decor.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);}
    }
}
