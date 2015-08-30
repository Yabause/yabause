package org.uoyabause.android;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.android.PadEvent;
import org.uoyabause.android.PadManager;


class PadManagerV16 extends PadManager {
    private HashMap<String,Integer> deviceIds;
    HashMap<Integer,Integer> Keymap;
    final String TAG = "PadManagerV16";
    float _oldRightTrigger = 0.0f;
    float _oldLeftTrigger = 0.0f;
    String DebugMesage = new String();
    int _selected_device_id = invalid_device_id;

    PadManagerV16() {
        deviceIds = new HashMap<String,Integer>();
        

        int[] ids = InputDevice.getDeviceIds();
        for (int deviceId : ids) {
            InputDevice dev = InputDevice.getDevice(deviceId);
            int sources = dev.getSources();

            if (((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
                    || ((sources & InputDevice.SOURCE_JOYSTICK)== InputDevice.SOURCE_JOYSTICK)) {
                if (deviceIds.get(dev.getDescriptor()) == null ) {
                	
                	// Avoid crazy devices
                	if( dev.getName().equals("msm8974-taiko-mtp-snd-card Button Jack") ){
                		continue;
                	}
                    deviceIds.put(dev.getDescriptor(),deviceId);
                }
            }
            
            boolean isGamePad = ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD);
            boolean isGameJoyStick = ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
            
           
            DebugMesage += "Inputdevice:" + dev.getName() +" ID:" + dev.getDescriptor() + " Product ID:" +  dev.getProductId() + 
            		" isGamePad?:" + isGamePad +
            		" isJoyStick?:" + isGameJoyStick + "\n";
            
        }
        
      
        Keymap = new HashMap<Integer,Integer>();
        loadSettings();
        
    }

    public boolean hasPad() {
        return deviceIds.size() > 0;
    }
    
    public String getDeviceList(){
    	return DebugMesage;
    }
    
    public int getDeviceCount(){
    	return deviceIds.size();
    }
    public String getName( int index ){
    	
    	if( index < 0 && index >= deviceIds.size()) {
    		return null;
    	}
    	
    	int counter = 0;
    	for (Integer val : deviceIds.values()) {
    	    if( counter == index){
    	    	InputDevice dev = InputDevice.getDevice(val);
    	    	return dev.getName();
    	    }
    	    counter++;
    	}
    	
    	return null;
    }
    
    public String getId( int index ){

    	if( index < 0 && index >= deviceIds.size()) {
    		return null;
    	}
    	
    	int counter = 0;
    	for (String key : deviceIds.keySet()) {
    	    if( counter == index){
    	    	return key;
    	    }
    	    counter++;
    	}
    	return null;
    }
    
    public void setPlayer1InputDevice( String deviceid ){
    	 
    	if( deviceid == null ){
    		_selected_device_id = -1;
    		return;
    	} 
 
    	Integer id = deviceIds.get(deviceid);
    	if( id == null ){
    		_selected_device_id = -1;
    	}else{
    		_selected_device_id = id;
    	}  
 
    	return;
    }
    
    public int getPlayer1InputDevice(){ 
    	return _selected_device_id; 
    }
    
    public int onGenericMotionEvent(MotionEvent event){
    	int rtn = 0;
    	if( event.getDeviceId() != _selected_device_id ) return 0;
    	
        if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {
        	
        	  float newLeftTrigger = event.getAxisValue( MotionEvent.AXIS_LTRIGGER );
        	  if( newLeftTrigger != _oldLeftTrigger ){
        		  //Log.d(TAG,"AXIS_LTRIGGER = " + newLeftTrigger);
        		  
        		  // On
        		  if( _oldLeftTrigger < newLeftTrigger /*&& _oldLeftTrigger < 0.001*/ ){
        			
        	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_LTRIGGER);
                    	if( PadKey != null ) {
                    	   	//pe = new PadEvent(0, PadKey);
                    		YabauseRunnable.press(PadKey);
                    		rtn = 1;
                    	}			  
        		  }
        		  
        		  // Off
        		  else if( _oldLeftTrigger > newLeftTrigger /*&& newLeftTrigger > 0.5*/ ){
	      	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_LTRIGGER);
	                  	if( PadKey != null ) {
	                  	   	//pe = new PadEvent(1, PadKey);
	                  		YabauseRunnable.release(PadKey);
	                  		rtn = 1;
	                  	}   			  
        		  }
        		  
        		  _oldLeftTrigger = newLeftTrigger;
        	  }
        	  
        	  float newRightTrigger = event.getAxisValue( MotionEvent.AXIS_RTRIGGER );
        	  if( newRightTrigger != _oldRightTrigger ){
        		  //Log.d(TAG,"AXIS_LTRIGGER = " + newRightTrigger);

        		  // On
        		  if( _oldRightTrigger < newRightTrigger /*&& _oldRightTrigger < 0.001*/ ){
        			
        	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_RTRIGGER);
                    	if( PadKey != null ) {
                    	   	//pe = new PadEvent(0, PadKey);
                    		YabauseRunnable.press(PadKey);
                    		rtn = 1;
                    	}			  
        		  }
        		  
        		  // Off
        		  else if( _oldRightTrigger > newRightTrigger /*&& newRightTrigger > 0.5*/ ){
	      	           	Integer PadKey = Keymap.get(MotionEvent.AXIS_RTRIGGER);
	                  	if( PadKey != null ) {
	                  	   	//pe = new PadEvent(1, PadKey);
	                  	  YabauseRunnable.release(PadKey);
	                  	  rtn = 1;
	                  	}   			  
        		  }
        		  _oldRightTrigger = newRightTrigger;
        	  }
        }
    	return rtn;
    }

    public int onKeyDown(int keyCode, KeyEvent event) {
        PadEvent pe = null;
        
        if( keyCode == KeyEvent.KEYCODE_BACK ){
        	return 0;
        }
        
        if( event.getDeviceId() != _selected_device_id ) return 0;

        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
            ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            if (event.getRepeatCount() == 0) {
            	
            	Integer PadKey = Keymap.get(keyCode);
            	if( PadKey != null ) {
            	   	//pe = new PadEvent(0, Keymap.get(keyCode));
            		YabauseRunnable.press(PadKey);
            		return 1;
            	}else{
            		return 0;
            	}
            }
        }

        return 0;
    }

    public int onKeyUp(int keyCode, KeyEvent event) {
        PadEvent pe = null;
       
        if( keyCode == KeyEvent.KEYCODE_BACK ){
        	return 0;
        }
        
        if( event.getDeviceId() != _selected_device_id ) return 0;

        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
            ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
            if (event.getRepeatCount() == 0) {
            	
            	Integer PadKey = Keymap.get(keyCode);
            	if( PadKey != null ) {
            	   	//pe = new PadEvent(1, Keymap.get(keyCode));
            		YabauseRunnable.release(PadKey);
            		return 1;
            	}else{
            		return 0;
            	}            	
            }
        }

        return 0;
    }
    
    void loadDefault(){
    	Keymap.clear();
        Keymap.put(KeyEvent.KEYCODE_DPAD_UP,PadEvent.BUTTON_UP);
        Keymap.put(KeyEvent.KEYCODE_DPAD_DOWN, PadEvent.BUTTON_DOWN);
        Keymap.put(KeyEvent.KEYCODE_DPAD_LEFT, PadEvent.BUTTON_LEFT);
        Keymap.put(KeyEvent.KEYCODE_DPAD_RIGHT, PadEvent.BUTTON_RIGHT);
        Keymap.put(MotionEvent.AXIS_LTRIGGER,PadEvent.BUTTON_LEFT_TRIGGER);
        Keymap.put(MotionEvent.AXIS_RTRIGGER, PadEvent.BUTTON_RIGHT_TRIGGER);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_START, PadEvent.BUTTON_START);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_A, PadEvent.BUTTON_A);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_B, PadEvent.BUTTON_B);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_R1, PadEvent.BUTTON_C);        
        Keymap.put(KeyEvent.KEYCODE_BUTTON_X, PadEvent.BUTTON_X);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_Y, PadEvent.BUTTON_Y);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_L1, PadEvent.BUTTON_Z);   	
    }
    
    void loadSettings(){
        try {
        	
            File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");
            if (! yabroot.exists()) yabroot.mkdir();
            
            InputStream inputStream = new FileInputStream(Environment.getExternalStorageDirectory() + "/" +  "yabause/keymap.json");
            int size = inputStream.available();
            byte[] buffer = new byte[size];
            inputStream.read(buffer);
            inputStream.close();
             
            String json = new String(buffer);
            JSONObject jsonObject = new JSONObject(json);
            
            Keymap.clear();
            
            Keymap.put(jsonObject.getInt("BUTTON_UP"),PadEvent.BUTTON_UP);
            Keymap.put(jsonObject.getInt("BUTTON_DOWN"), PadEvent.BUTTON_DOWN);
            Keymap.put(jsonObject.getInt("BUTTON_LEFT"), PadEvent.BUTTON_LEFT);
            Keymap.put(jsonObject.getInt("BUTTON_RIGHT"), PadEvent.BUTTON_RIGHT);
            Keymap.put(jsonObject.getInt("BUTTON_LEFT_TRIGGER"),PadEvent.BUTTON_LEFT_TRIGGER);
            Keymap.put(jsonObject.getInt("BUTTON_RIGHT_TRIGGER"), PadEvent.BUTTON_RIGHT_TRIGGER);
            Keymap.put(jsonObject.getInt("BUTTON_START"), PadEvent.BUTTON_START);
            Keymap.put(jsonObject.getInt("BUTTON_A"), PadEvent.BUTTON_A);
            Keymap.put(jsonObject.getInt("BUTTON_B"), PadEvent.BUTTON_B);
            Keymap.put(jsonObject.getInt("BUTTON_C"), PadEvent.BUTTON_C);        
            Keymap.put(jsonObject.getInt("BUTTON_X"), PadEvent.BUTTON_X);
            Keymap.put(jsonObject.getInt("BUTTON_Y"), PadEvent.BUTTON_Y);
            Keymap.put(jsonObject.getInt("BUTTON_Z"), PadEvent.BUTTON_Z);             
                
             
        } catch (IOException e) {
            e.printStackTrace();
            loadDefault();
        } catch (JSONException e) {
            e.printStackTrace();
            loadDefault();
        }    	
    }

    
}

