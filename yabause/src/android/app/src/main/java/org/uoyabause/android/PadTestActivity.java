package org.uoyabause.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.os.Bundle;
import android.preference.PreferenceManager;

public class PadTestActivity extends Activity implements OnPadListener {

	YabausePad mPadView;
	SeekBar mSlide;
    SeekBar mTransSlide;
    private PadManager padm;
    TextView tv;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
        if( lock_landscape == true ){
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }else{
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        }

        super.onCreate(savedInstanceState);

        padm = PadManager.getPadManager();
        padm.setTestMode(true);
        padm.loadSettings();

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        
        // Immersive mode 
        View decor = this.getWindow().getDecorView();
        decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION  | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE);

        setContentView(R.layout.padtest);
        
        mPadView = (YabausePad)findViewById(R.id.yabause_pad);
        mPadView.setTestmode(true);
        mPadView.setOnPadListener(this);
        mPadView.show(true);

        mSlide   = (SeekBar)findViewById(R.id.button_scale);
        mSlide.setProgress( (int)(mPadView.getScale()*100.0f) );
        mSlide.setOnSeekBarChangeListener(
                new OnSeekBarChangeListener() {
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                        mPadView.setScale((float) progress / 100.0f);
                        mPadView.requestLayout();
                        mPadView.invalidate();
                    }

                    public void onStartTrackingTouch(SeekBar seekBar) {
                    }

                    public void onStopTrackingTouch(SeekBar seekBar) {
                    }
                }
        );

        mTransSlide   = (SeekBar)findViewById(R.id.button_transparent);
        mTransSlide.setProgress( (int)(mPadView.getTrans()*100.0f) );
        mTransSlide.setOnSeekBarChangeListener(
                new OnSeekBarChangeListener() {
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                        mPadView.setTrans((float) progress / 100.0f);
                        mPadView.invalidate();
                    }

                    public void onStartTrackingTouch(SeekBar seekBar) {
                    }

                    public void onStopTrackingTouch(SeekBar seekBar) {
                    }
                }
        );


        tv = (TextView)findViewById(R.id.text_status);
    }
    
	@Override
	public void onBackPressed(){
		
        AlertDialog.Builder alert = new AlertDialog.Builder(this);  
        alert.setTitle("");  
        alert.setMessage(R.string.do_you_want_to_save_this_setting);  
        alert.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener(){  
            public void onClick(DialogInterface dialog, int which) {  
            	SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(PadTestActivity.this);
   			 	Editor editor = sharedPref.edit();
   			 	float value = (float)mSlide.getProgress()/100.0f;
   			 	editor.putFloat("pref_pad_scale", value );
   			 	editor.commit();

                value = (float)mTransSlide.getProgress()/100.0f;
                editor.putFloat("pref_pad_trans", value );
                editor.commit();


   			 	PadTestActivity.super.onBackPressed();
            }});  
        alert.setNegativeButton(R.string.no, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                PadTestActivity.super.onBackPressed();
            }
        });
        alert.show();
		
		
	}


//    @Override public boolean onGenericMotionEvent(MotionEvent event) {
/*
        int rtn = padm.onGenericMotionEvent(event);
        if (rtn != 0) {
            tv.setText(padm.getStatusString());
            tv.invalidate();
            return true;
        }
        tv.setText(padm.getStatusString());
        tv.invalidate();
*/
//        return super.onGenericMotionEvent(event);
//    }

    @Override
    public boolean dispatchKeyEvent (KeyEvent event){


        int action =event.getAction();
        int keyCode = event.getKeyCode();
        //Log.d("dispatchKeyEvent","device:" + event.getDeviceId() + ",action:" + action +",keyCoe:" + keyCode );
        if( action == KeyEvent.ACTION_UP){
            int rtn = padm.onKeyUp(keyCode, event);
            if (rtn != 0) {
                tv.setText(padm.getStatusString());
                tv.invalidate();
                return true;
            }
            tv.setText(padm.getStatusString());
            tv.invalidate();

        }else if( action == KeyEvent.ACTION_MULTIPLE ){

        }else if( action == KeyEvent.ACTION_DOWN ){

            if ( keyCode == KeyEvent.KEYCODE_BACK) {
                return super.dispatchKeyEvent(event);
            }

            int rtn =  padm.onKeyDown(keyCode, event);
            if (rtn != 0) {
                tv.setText(padm.getStatusString());
                tv.invalidate();
                return true;
            }
            tv.setText(padm.getStatusString());
            tv.invalidate();

        }

        return super.dispatchKeyEvent(event);
    }

	@Override
	public boolean onPad(PadEvent event) {
		TextView tv = (TextView)findViewById(R.id.text_status);
		tv.setText(mPadView.getStatusString());
		tv.invalidate();
		return true;
	}	
	
}
