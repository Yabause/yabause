package org.uoyabause.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
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
	
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
       
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        
        // Immersive mode 
        View decor = this.getWindow().getDecorView();
        decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION  | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE);

        setContentView(R.layout.padtest);
        
        mPadView = (YabausePad)findViewById(R.id.yabause_pad);
        mPadView.setTestmode(true);
        mPadView.setOnPadListener(this);
        mSlide   = (SeekBar)findViewById(R.id.button_scale);
        
        mSlide.setProgress( (int)(mPadView.getScale()*100.0f) );
        
        mSlide.setOnSeekBarChangeListener(
                new OnSeekBarChangeListener() {
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    	mPadView.setScale( (float)progress / 100.0f );
                    	mPadView.requestLayout();
                    	mPadView.invalidate();
                    }
  
                    public void onStartTrackingTouch(SeekBar seekBar) {
                    }
  
                    public void onStopTrackingTouch(SeekBar seekBar) {
                    }
                }
        );        
               
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
   			 	PadTestActivity.super.onBackPressed();
            }});  
        alert.setNegativeButton(R.string.no, new DialogInterface.OnClickListener(){  
            public void onClick(DialogInterface dialog, int which) {  
            	PadTestActivity.super.onBackPressed();
            }});  
        alert.show(); 
		
		
	}	
    

	@Override
	public boolean onPad(PadEvent event) {
		
		TextView tv = (TextView)findViewById(R.id.text_status);
		tv.setText(mPadView.getStatusString());
		tv.invalidate();
		
		return false;
	}	
	
}
