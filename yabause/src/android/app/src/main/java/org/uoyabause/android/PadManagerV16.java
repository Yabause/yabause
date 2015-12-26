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
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.android.PadEvent;
import org.uoyabause.android.PadManager;


class InputInfo{
	public float _oldRightTrigger = 0.0f;
	public float _oldLeftTrigger = 0.0f;
	public int _selected_device_id = -1;
}

class PadManagerV16 extends PadManager {
	
    private HashMap<String,Integer> deviceIds;
     final String TAG = "PadManagerV16";
    String DebugMesage = new String();
    
    final int player_count = 2; 
    List<HashMap<Integer,Integer>> Keymap;
    
    InputInfo input[];
    
    PadManagerV16() {
        deviceIds = new HashMap<String,Integer>();
        
        input = new InputInfo[player_count];
        Keymap = new ArrayList<HashMap<Integer, Integer>>(); 
        for( int i=0; i<player_count; i++ ){
        	input[i] = new InputInfo();
        	input[i]._selected_device_id = invalid_device_id;
        	HashMap<Integer,Integer> akymap = new HashMap<Integer,Integer>();
        	Keymap.add(akymap);
        }
        

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
    		input[0]._selected_device_id = -1;
    		return;
    	} 
 
    	Integer id = deviceIds.get(deviceid);
    	if( id == null ){
    		input[0]._selected_device_id = -1;
    	}else{
    		input[0]._selected_device_id = id;
    	}  
 
    	return;
    }
    
    public int getPlayer1InputDevice(){ 
    	return input[0]._selected_device_id; 
    }
    
    public void setPlayer2InputDevice( String deviceid ){
   	 
    	if( deviceid == null ){
    		input[1]._selected_device_id = -1;
    		return;
    	} 
 
    	Integer id = deviceIds.get(deviceid);
    	if( id == null ){
    		input[1]._selected_device_id = -1;
    	}else{
    		input[1]._selected_device_id = id;
    	}  
 
    	return;
    }
    
    public int getPlayer2InputDevice(){ 
    	return input[1]._selected_device_id; 
    }
    
    int findPlayerIndex( int deviceid ){
    	for( int i=0; i< player_count; i++ ){
    		if( deviceid == input[i]._selected_device_id){
    			return i;
    		}
    	}
    	return -1;
    }
    
    public int onGenericMotionEvent(MotionEvent event){
    	int rtn = 0;
    	int playerindex = findPlayerIndex(event.getDeviceId());
    	if( playerindex == -1 ) return 0;
    	
        if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {

			MotionEvent motionEvent = (MotionEvent) event;
			for(HashMap.Entry<Integer, Integer> e : Keymap.get(playerindex).entrySet()) {
				//System.out.println(e.getKey() + " : " + e.getValue());
				int btn = e.getKey();
				if( (btn&0x80000000) != 0 ) {
					float motion_value = motionEvent.getAxisValue((btn&0x00007FFF));
					if( (btn&0x8000) != 0 ){

						if (Float.compare(motion_value, -1.0f) == 0) { // ON
							YabauseRunnable.press(e.getValue(),playerindex);
						}
						else if( Float.compare(motion_value, -0.5f) > 0 ){ // OFF
							YabauseRunnable.release(e.getValue(),playerindex);
						}
					}else{
						if (Float.compare(motion_value, 1.0f) == 0) {  // ON
							YabauseRunnable.press(e.getValue(),playerindex);
						}
						else if( Float.compare(motion_value, 0.5f) < 0 ){ // OFF
							YabauseRunnable.release(e.getValue(),playerindex);
						}
					}
				}

			}
        }
    	return rtn;
    }

    public int onKeyDown(int keyCode, KeyEvent event) {
        PadEvent pe = null;
        
        if( keyCode == KeyEvent.KEYCODE_BACK ){
        	return 0;
        }
        
    	int playerindex = findPlayerIndex(event.getDeviceId());
    	if( playerindex == -1 ) return 0; 

        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
            ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
        	
            	Integer PadKey = Keymap.get(playerindex).get(keyCode);
            	if( PadKey != null ) {
            		event.startTracking();
            		YabauseRunnable.press(PadKey,playerindex);
             		return 1;  // ignore this input
            	}else{
            		return 0;
            	}
        }
        return 0;
    }

    public int onKeyUp(int keyCode, KeyEvent event) {
        PadEvent pe = null;
       
        if( keyCode == KeyEvent.KEYCODE_BACK ){
        	return 0;
        }
        
    	int playerindex = findPlayerIndex(event.getDeviceId());
    	if( playerindex == -1 ) return 0;

        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
            ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {
                 	
        	if( !event.isCanceled() ){
            	Integer PadKey = Keymap.get(playerindex).get(keyCode);
            	if( PadKey != null ) {
            	   	YabauseRunnable.release(PadKey,playerindex);
          		    return 1; // ignore this input
            	}else{
            		return 0;
            	}            	
        	}
          }

        return 0;
    }
    
    void loadDefault( int player ){
	    Keymap.get(player).clear();
	    Keymap.get(player).put((MotionEvent.AXIS_HAT_Y | 0x8000 ),PadEvent.BUTTON_UP);
	    Keymap.get(player).put(MotionEvent.AXIS_HAT_Y, PadEvent.BUTTON_DOWN);
	    Keymap.get(player).put((MotionEvent.AXIS_HAT_X | 0x8000 ), PadEvent.BUTTON_LEFT);
	    Keymap.get(player).put(MotionEvent.AXIS_HAT_X , PadEvent.BUTTON_RIGHT);
	    Keymap.get(player).put(MotionEvent.AXIS_LTRIGGER,PadEvent.BUTTON_LEFT_TRIGGER);
	    Keymap.get(player).put(MotionEvent.AXIS_RTRIGGER, PadEvent.BUTTON_RIGHT_TRIGGER);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_START, PadEvent.BUTTON_START);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_A, PadEvent.BUTTON_A);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_B, PadEvent.BUTTON_B);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_R1, PadEvent.BUTTON_C);        
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_X, PadEvent.BUTTON_X);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_Y, PadEvent.BUTTON_Y);
	    Keymap.get(player).put(KeyEvent.KEYCODE_BUTTON_L1, PadEvent.BUTTON_Z);   
    }
    
    public void loadSettings(){
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
            
            Keymap.get(0).clear();
            Keymap.get(0).put(jsonObject.getInt("BUTTON_UP"),PadEvent.BUTTON_UP);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_DOWN"), PadEvent.BUTTON_DOWN);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_LEFT"), PadEvent.BUTTON_LEFT);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_RIGHT"), PadEvent.BUTTON_RIGHT);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_LEFT_TRIGGER"),PadEvent.BUTTON_LEFT_TRIGGER);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_RIGHT_TRIGGER"), PadEvent.BUTTON_RIGHT_TRIGGER);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_START"), PadEvent.BUTTON_START);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_A"), PadEvent.BUTTON_A);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_B"), PadEvent.BUTTON_B);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_C"), PadEvent.BUTTON_C);        
            Keymap.get(0).put(jsonObject.getInt("BUTTON_X"), PadEvent.BUTTON_X);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_Y"), PadEvent.BUTTON_Y);
            Keymap.get(0).put(jsonObject.getInt("BUTTON_Z"), PadEvent.BUTTON_Z);             
            
            
             
        } catch (IOException e) {
            e.printStackTrace();
            loadDefault(0);
        } catch (JSONException e) {
            e.printStackTrace();
            loadDefault(0);
        }  
        
        try{
        	InputStream inputStream = new FileInputStream(Environment.getExternalStorageDirectory() + "/" +  "yabause/keymap_player2.json");
            int size = inputStream.available();
            byte[] buffer_p2 = new byte[size];
            inputStream.read(buffer_p2);
            inputStream.close();
             
            String json = new String(buffer_p2);
            JSONObject jsonObject = new JSONObject(json);
            
            Keymap.get(1).clear();
            Keymap.get(1).put(jsonObject.getInt("BUTTON_UP"),PadEvent.BUTTON_UP);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_DOWN"), PadEvent.BUTTON_DOWN);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_LEFT"), PadEvent.BUTTON_LEFT);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_RIGHT"), PadEvent.BUTTON_RIGHT);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_LEFT_TRIGGER"),PadEvent.BUTTON_LEFT_TRIGGER);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_RIGHT_TRIGGER"), PadEvent.BUTTON_RIGHT_TRIGGER);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_START"), PadEvent.BUTTON_START);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_A"), PadEvent.BUTTON_A);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_B"), PadEvent.BUTTON_B);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_C"), PadEvent.BUTTON_C);        
            Keymap.get(1).put(jsonObject.getInt("BUTTON_X"), PadEvent.BUTTON_X);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_Y"), PadEvent.BUTTON_Y);
            Keymap.get(1).put(jsonObject.getInt("BUTTON_Z"), PadEvent.BUTTON_Z);         	
        }catch (IOException e) {
            e.printStackTrace();
            loadDefault(1);
        } catch (JSONException e) {
            e.printStackTrace();
            loadDefault(1);
        }  
    }

    
}

