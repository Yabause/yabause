package org.uoyabause.android;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.os.Bundle;
import android.os.Environment;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnGenericMotionListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.LinearLayout;

@SuppressLint("NewApi") public class InputSettingPrefernce extends DialogPreference implements OnKeyListener, OnGenericMotionListener{

	private TextView key_message;
	private HashMap<Integer,Integer> Keymap;
    private ArrayList<Integer> map;
	private int index = 0;
    private PadManager pad_m;
    Context context_m;
    private int _selected_device_id = 0;

	public InputSettingPrefernce(Context context) {
		super(context);
		InitObjects(context);
		
	}
	public InputSettingPrefernce(Context context,AttributeSet attrs) {
		super(context,attrs);
		InitObjects(context);
		}
	public InputSettingPrefernce(Context context,AttributeSet attrs,int defStyle) {
		super(context,attrs,defStyle);
		InitObjects(context);
	}
	
	void InitObjects( Context context){
		index = 0;
		context_m = context;
		Keymap = new HashMap<Integer,Integer>();
		map = new ArrayList<Integer>();
    	map.add(PadEvent.BUTTON_UP);
    	map.add(PadEvent.BUTTON_DOWN);
    	map.add(PadEvent.BUTTON_LEFT);
    	map.add(PadEvent.BUTTON_RIGHT);
    	map.add(PadEvent.BUTTON_LEFT_TRIGGER);
    	map.add(PadEvent.BUTTON_RIGHT_TRIGGER); 
    	map.add(PadEvent.BUTTON_START);
    	map.add(PadEvent.BUTTON_A);
    	map.add(PadEvent.BUTTON_B);
    	map.add(PadEvent.BUTTON_C);    	
    	map.add(PadEvent.BUTTON_X);
    	map.add(PadEvent.BUTTON_Y);
    	map.add(PadEvent.BUTTON_Z);
		setDialogTitle("Input the Key");
    	setPositiveButtonText(null);  // OKボタンを非表示にする		
    	
	}
	
	void setMessage( String str ){
		key_message.setText(str);
	}
	
	@Override
	protected void	showDialog(Bundle state){
		super.showDialog(state);
		Dialog dlg = this.getDialog();
    	dlg.setOnKeyListener(this);		

    	index = 0;
    	Keymap.clear();
    	
		pad_m = PadManager.getPadManager();
    	if( pad_m.hasPad() == false ){
    		Toast.makeText(context_m, "Joy Stick is not connected", Toast.LENGTH_LONG).show();
    		dlg.dismiss();
    		return;
    	}    
    	
    	_selected_device_id = pad_m.getPlayer1InputDevice();
	}

	 
	@Override
	protected View onCreateDialogView() {
		this.key_message = new TextView(this.getContext());
		this.key_message.setText("UP"); 
		this.key_message.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 32);
		this.key_message.setClickable(false);
		this.key_message.setLayoutParams(new LinearLayout.LayoutParams(
				LinearLayout.LayoutParams.MATCH_PARENT,
				LinearLayout.LayoutParams.MATCH_PARENT));
		this.key_message.setGravity(Gravity.CENTER);
		
		key_message.setOnGenericMotionListener(this);
		key_message.setFocusableInTouchMode(true);
		key_message.requestFocus();
	        
		return this.key_message;
	}
	
	@Override
	protected void onBindDialogView(View view){
		view.setOnGenericMotionListener(this);	
	}
	
	@Override
	protected void onDialogClosed(boolean positiveResult) {
		if(positiveResult){
			persistString("yabause/keymap.json");
		}
		super.onDialogClosed(positiveResult);
	}

	
    public void saveSettings(){
    	
    	JSONObject jsonObject = new JSONObject();
    	try {
    	
	    	for (HashMap.Entry<Integer,Integer> entry : Keymap.entrySet()) {
	    	    // キーを取得
	    		Integer key = entry.getKey();
	    	    // 値を取得
	    		Integer val = entry.getValue();
	    		switch(val){
	    		case PadEvent.BUTTON_UP: jsonObject.put("BUTTON_UP", key); break;
	    		case PadEvent.BUTTON_DOWN: jsonObject.put("BUTTON_DOWN", key); break;
	    		case PadEvent.BUTTON_LEFT: jsonObject.put("BUTTON_LEFT", key); break;
	    		case PadEvent.BUTTON_RIGHT: jsonObject.put("BUTTON_RIGHT", key); break;
	    		case PadEvent.BUTTON_LEFT_TRIGGER: jsonObject.put("BUTTON_LEFT_TRIGGER", key); break;
	    		case PadEvent.BUTTON_RIGHT_TRIGGER: jsonObject.put("BUTTON_RIGHT_TRIGGER", key); break;
	    		case PadEvent.BUTTON_START: jsonObject.put("BUTTON_START", key); break;
	    		case PadEvent.BUTTON_A: jsonObject.put("BUTTON_A", key); break;
	    		case PadEvent.BUTTON_B: jsonObject.put("BUTTON_B", key); break;
	    		case PadEvent.BUTTON_C: jsonObject.put("BUTTON_C", key); break;
	    		case PadEvent.BUTTON_X: jsonObject.put("BUTTON_X", key); break;
	    		case PadEvent.BUTTON_Y: jsonObject.put("BUTTON_Y", key); break;
	    		case PadEvent.BUTTON_Z: jsonObject.put("BUTTON_Z", key); break;
	    		}
	    	}
	    	
	        // jsonファイル出力
	        File file = new File(Environment.getExternalStorageDirectory() + "/" +  "yabause/keymap.json");
	        FileWriter filewriter;
	     
	        filewriter = new FileWriter(file);
	        BufferedWriter bw = new BufferedWriter(filewriter);
	        PrintWriter pw = new PrintWriter(bw);
	        pw.write(jsonObject.toString());
	        pw.close();	
    	
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        }

    }	
    
    boolean setKeymap(Integer padkey){
    	Keymap.put(padkey,map.get(index));
    	Log.d("setKeymap","index =" + index +": pad = " + padkey );
    	index++;
    	
    	if( index >= map.size() ){
    		saveSettings();
    		Dialog dlg = this.getDialog();
    		dlg.dismiss();
    		return true;
    	}
    	
		switch(map.get(index)){
		case PadEvent.BUTTON_UP: setMessage("Up"); break;
		case PadEvent.BUTTON_DOWN: setMessage("Down"); break;
		case PadEvent.BUTTON_LEFT:setMessage("Left"); break;
		case PadEvent.BUTTON_RIGHT: setMessage("Right"); break;
		case PadEvent.BUTTON_LEFT_TRIGGER: setMessage("L Trigger"); break;
		case PadEvent.BUTTON_RIGHT_TRIGGER: setMessage("R Trigger"); break;
		case PadEvent.BUTTON_START:setMessage("Start"); break;
		case PadEvent.BUTTON_A: setMessage("A"); break;
		case PadEvent.BUTTON_B: setMessage("B"); break;
		case PadEvent.BUTTON_C: setMessage("C"); break;
		case PadEvent.BUTTON_X:setMessage("X"); break;
		case PadEvent.BUTTON_Y: setMessage("Y"); break;
		case PadEvent.BUTTON_Z: setMessage("Z"); break;
		}	
		
		return true;
		
    }

    private final int KEYCODE_L2 = 104;
    private final int KEYCODE_R2 = 105;

	@Override
	public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
		
		if( event.getDeviceId() != _selected_device_id ) return false;
		
        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
                if (event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_DOWN) {
                	
                	
                	// for PS3 Controller needs to ignore L2,R2. this event is duped at onGenericMotion.
                	InputDevice dev = InputDevice.getDevice(event.getDeviceId());
                	if( dev.getProductId() == 616 & (keyCode == KEYCODE_L2 || keyCode == KEYCODE_R2) ){
                		return false;
                	}
                	                	
                	Integer PadKey = Keymap.get(keyCode);
                	if( PadKey != null ) {
                		Toast.makeText(context_m, "This Key has already been set.", Toast.LENGTH_SHORT).show();
                		return true;
                	}
    	    		return setKeymap(keyCode);        	
                }
            }
        	return false;
	}
	
	protected float _oldLeftTrigger = 0.0f;
	protected float _oldRightTrigger = 0.0f;
	
	@Override
	public boolean onGenericMotion(View v, MotionEvent event) {
		
		if( event.getDeviceId() != _selected_device_id ) return false;
		
        if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {
        	
      	  float newLeftTrigger = event.getAxisValue( MotionEvent.AXIS_LTRIGGER );
      	  if( newLeftTrigger != _oldLeftTrigger ){
     		  
      		  // On
      		  if( _oldLeftTrigger < newLeftTrigger && _oldLeftTrigger < 0.001 ){
      			
      			    _oldLeftTrigger = newLeftTrigger;
      	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_LTRIGGER);
                	if( PadKey != null ) {
                		Toast.makeText(context_m, "This Key has already been set.", Toast.LENGTH_SHORT).show();
                		_oldLeftTrigger = newLeftTrigger;
                		return true;
                	}
                	_oldLeftTrigger = newLeftTrigger;
                	return setKeymap(MotionEvent.AXIS_LTRIGGER); 
                	
      		  }
      		  _oldLeftTrigger = newLeftTrigger;
      	  }
      	  
      	  float newRightTrigger = event.getAxisValue( MotionEvent.AXIS_RTRIGGER );
      	  if( newRightTrigger != _oldRightTrigger ){

      		  // On
      		  if( _oldRightTrigger < newRightTrigger && _oldRightTrigger < 0.001 ){
      			
          		  	_oldRightTrigger = newRightTrigger;
      	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_RTRIGGER);
                	if( PadKey != null ) {
                		Toast.makeText(context_m, "This Key has already been set.", Toast.LENGTH_SHORT).show();
                		_oldRightTrigger = newRightTrigger;
                		return true;
                	}	
                	_oldRightTrigger = newRightTrigger;
                	return setKeymap(MotionEvent.AXIS_RTRIGGER); 
      		  }
      		  _oldRightTrigger = newRightTrigger;
      	  }
      }

		return false;
	}

}
