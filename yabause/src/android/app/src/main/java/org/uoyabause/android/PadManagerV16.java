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

import static org.uoyabause.android.PadManager.MODE_ANALOG;


//class InputInfo{
//	public float _oldRightTrigger = 0.0f;
//	public float _oldLeftTrigger = 0.0f;
//	public int _selected_device_id = -1;
//}

class BasicInputDevice {
    public int _selected_device_id = -1;
    public int _playerindex = 0;
    boolean _testmode = false;
    Integer currentButtonState = 0;
    final int showMenuCode = 0x1c40;
    HashMap<Integer, Integer> Keymap;

    PadManagerV16 _pdm;

    BasicInputDevice(PadManagerV16 pdm) {
        Keymap = new HashMap<Integer, Integer>();
        _pdm = pdm;
    }

    void loadDefault( ){
        Keymap.clear();
        Keymap.put((MotionEvent.AXIS_HAT_Y | 0x8000), PadEvent.BUTTON_UP);
        Keymap.put(MotionEvent.AXIS_HAT_Y, PadEvent.BUTTON_DOWN);
        Keymap.put((MotionEvent.AXIS_HAT_X | 0x8000), PadEvent.BUTTON_LEFT);
        Keymap.put(MotionEvent.AXIS_HAT_X, PadEvent.BUTTON_RIGHT);
        Keymap.put(MotionEvent.AXIS_LTRIGGER, PadEvent.BUTTON_LEFT_TRIGGER);
        Keymap.put(MotionEvent.AXIS_RTRIGGER, PadEvent.BUTTON_RIGHT_TRIGGER);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_START, PadEvent.BUTTON_START);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_A, PadEvent.BUTTON_A);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_B, PadEvent.BUTTON_B);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_R1, PadEvent.BUTTON_C);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_X, PadEvent.BUTTON_X);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_Y, PadEvent.BUTTON_Y);
        Keymap.put(KeyEvent.KEYCODE_BUTTON_L1, PadEvent.BUTTON_Z);
        Keymap.put(MotionEvent.AXIS_X, PadEvent.PERANALOG_AXIS_X);
        Keymap.put(MotionEvent.AXIS_Y, PadEvent.PERANALOG_AXIS_Y);
        Keymap.put(MotionEvent.AXIS_LTRIGGER, PadEvent.PERANALOG_AXIS_LTRIGGER);
        Keymap.put(MotionEvent.AXIS_RTRIGGER, PadEvent.PERANALOG_AXIS_RTRIGGER);
    }

    public void loadSettings( String setting_filename ) {
        try {

            File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");
            if (!yabroot.exists()) yabroot.mkdir();

            InputStream inputStream = new FileInputStream(Environment.getExternalStorageDirectory() + "/" + "yabause/" + setting_filename);
            int size = inputStream.available();
            byte[] buffer = new byte[size];
            inputStream.read(buffer);
            inputStream.close();

            String json = new String(buffer);
            JSONObject jsonObject = new JSONObject(json);


            Keymap.clear();
            Keymap.put(jsonObject.getInt("BUTTON_UP"), PadEvent.BUTTON_UP);
            Keymap.put(jsonObject.getInt("BUTTON_DOWN"), PadEvent.BUTTON_DOWN);
            Keymap.put(jsonObject.getInt("BUTTON_LEFT"), PadEvent.BUTTON_LEFT);
            Keymap.put(jsonObject.getInt("BUTTON_RIGHT"), PadEvent.BUTTON_RIGHT);
            Keymap.put(jsonObject.getInt("BUTTON_LEFT_TRIGGER"), PadEvent.BUTTON_LEFT_TRIGGER);
            Keymap.put(jsonObject.getInt("BUTTON_RIGHT_TRIGGER"), PadEvent.BUTTON_RIGHT_TRIGGER);
            Keymap.put(jsonObject.getInt("BUTTON_START"), PadEvent.BUTTON_START);
            Keymap.put(jsonObject.getInt("BUTTON_A"), PadEvent.BUTTON_A);
            Keymap.put(jsonObject.getInt("BUTTON_B"), PadEvent.BUTTON_B);
            Keymap.put(jsonObject.getInt("BUTTON_C"), PadEvent.BUTTON_C);
            Keymap.put(jsonObject.getInt("BUTTON_X"), PadEvent.BUTTON_X);
            Keymap.put(jsonObject.getInt("BUTTON_Y"), PadEvent.BUTTON_Y);
            Keymap.put(jsonObject.getInt("BUTTON_Z"), PadEvent.BUTTON_Z);

            Keymap.put(jsonObject.getInt("PERANALOG_AXIS_X"), PadEvent.PERANALOG_AXIS_X);
            Keymap.put(jsonObject.getInt("PERANALOG_AXIS_Y"), PadEvent.PERANALOG_AXIS_Y);
            Keymap.put(jsonObject.getInt("PERANALOG_AXIS_LTRIGGER"), PadEvent.PERANALOG_AXIS_LTRIGGER);
            Keymap.put(jsonObject.getInt("PERANALOG_AXIS_RTRIGGER"), PadEvent.PERANALOG_AXIS_RTRIGGER);

        } catch (IOException e) {
            e.printStackTrace();
            loadDefault();
        } catch (JSONException e) {
            e.printStackTrace();
            loadDefault();
        }
    }

    // Limitaions
    // SS Controller adapter ... Left trigger and Right Trigger is not recognizeed as analog button. you need to skip them
    // Moga 000353 ... OK
    // SMACON ... Left trigger and Right Trigger is not recognizeed as analog button. you need to skip them
    // ipega ... OK, but too sensitive.
    public int onGenericMotionEvent(MotionEvent event ) {
        int rtn = 0;
        if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {

            MotionEvent motionEvent = (MotionEvent) event;

            for(HashMap.Entry<Integer, Integer> e : Keymap.entrySet()) {
                //System.out.println(e.getKey() + " : " + e.getValue());

                // AnalogDevices
                Integer  sat_btn = e.getValue();
                int btn = e.getKey();

                if( (_pdm.getAnalogMode() == MODE_ANALOG && _playerindex == 0) ||
                    (_pdm.getAnalogMode2() == MODE_ANALOG && _playerindex == 1)  ) {
                    float motion_value = motionEvent.getAxisValue((btn & 0x00007FFF));

                    if (sat_btn == PadEvent.PERANALOG_AXIS_X || sat_btn == PadEvent.PERANALOG_AXIS_Y ) {

                        float nomalize_value = (int) ((motion_value * 128.0) + 128);
                        if( nomalize_value > 255 ) nomalize_value = 255;
                        if( nomalize_value < 0 ) nomalize_value = 0;

                        YabauseRunnable.axis(sat_btn, _playerindex, (int)nomalize_value );

                        continue;
                    }else if( sat_btn == PadEvent.PERANALOG_AXIS_LTRIGGER || sat_btn == PadEvent.PERANALOG_AXIS_RTRIGGER ){

                        float nomalize_value = (int) ((motion_value * 255.0));
                        if( nomalize_value > 255 ) nomalize_value = 255;
                        if( nomalize_value < 0 ) nomalize_value = 0;

                        YabauseRunnable.axis(sat_btn, _playerindex, (int)nomalize_value );

                        // ToDo: Need to trigger event
                        //Keymap.put(MotionEvent.AXIS_LTRIGGER, PadEvent.BUTTON_LEFT_TRIGGER);
                        //Keymap.put(MotionEvent.AXIS_RTRIGGER, PadEvent.BUTTON_RIGHT_TRIGGER);

                        continue;

                    }

                }else{
                    if (sat_btn == PadEvent.PERANALOG_AXIS_X || sat_btn == PadEvent.PERANALOG_AXIS_Y || sat_btn == PadEvent.PERANALOG_AXIS_LTRIGGER || sat_btn == PadEvent.PERANALOG_AXIS_RTRIGGER){
                        continue;
                    }
                }


                if( (btn&0x80000000) != 0 ) {
                    float motion_value = motionEvent.getAxisValue((btn&0x00007FFF));
                    if( (btn&0x8000) != 0 ){  // Dir

                        if (Float.compare(motion_value, -0.8f) < 0) { // ON
                            if( _testmode)
                                _pdm.addDebugString("onGenericMotionEvent: On  " + btn + " Satpad: " + e.getValue().toString());
                            else
                                YabauseRunnable.press(e.getValue(), _playerindex);

                            rtn = 1;
                        }
                        else if( Float.compare(motion_value, -0.5f) > 0 ){ // OFF
                            if( _testmode)
                                _pdm.addDebugString("onGenericMotionEvent: Off  " + btn + " Satpad: " + e.getValue().toString());
                            else
                                YabauseRunnable.release(e.getValue(), _playerindex);

                            rtn = 1;
                        }
                    }else{
                        if (Float.compare(motion_value, 0.8f) > 0) {  // ON
                            if( _testmode)
                                _pdm.addDebugString("onGenericMotionEvent: On  " + btn + " Satpad: " + e.getValue().toString());
                            else
                                YabauseRunnable.press(e.getValue(), _playerindex);

                            rtn = 1;
                        }
                        else if( Float.compare(motion_value, 0.5f) < 0 ){ // OFF
                            if( _testmode)
                                _pdm.addDebugString("onGenericMotionEvent: Off  " + btn + " Satpad: " + e.getValue().toString());
                            else
                                YabauseRunnable.release(e.getValue(), _playerindex);

                            rtn = 1;
                        }
                    }
                }

                // AnalogDvice

            }
        }
        return rtn;
    }

    public int onKeyDown(int keyCode, KeyEvent event) {
        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {

            if( keyCode == KeyEvent.KEYCODE_BACK ){
                return 0;
            }

            if (keyCode == 0) {
                keyCode = event.getScanCode();
            }

            Integer PadKey = Keymap.get(keyCode);

            if (PadKey != null) {

                currentButtonState |= (1<<PadKey);
                if( showMenuCode == currentButtonState ){
                    _pdm.showMenu();
                    currentButtonState = 0; // clear
                }

                Log.d(this.getClass().getSimpleName(),"currentButtonState = " + Integer.toHexString(currentButtonState) );

                event.startTracking();
                if (_testmode)
                    _pdm.addDebugString("onKeyDown: " + keyCode + " Satpad: " + PadKey.toString());
                else
                    YabauseRunnable.press(PadKey, _playerindex);


                return 1;  // ignore this input
            } else {
                if (_testmode)
                    _pdm.addDebugString("onKeyDown: " + keyCode + " Satpad: none");
                return 0;
            }
        }
        return 0;
    }

    public int onKeyUp(int keyCode, KeyEvent event) {
        if (((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)) {

            if( keyCode == KeyEvent.KEYCODE_BACK ){
                return 0;
            }

            if( keyCode == 0 ){
                keyCode = event.getScanCode();
            }

            if( !event.isCanceled() ){
                Integer PadKey = Keymap.get(keyCode);
                if( PadKey != null ) {

                    currentButtonState &= ~(1<<PadKey);
                    Log.d(this.getClass().getSimpleName(),"currentButtonState = " + Integer.toHexString(currentButtonState) );

                    if( _testmode)
                        _pdm.addDebugString("onKeyUp: " + keyCode + " Satpad: " + PadKey.toString());
                    else
                        YabauseRunnable.release(PadKey, _playerindex);

                    return 1; // ignore this input
                }else{
                    if( _testmode) _pdm.addDebugString("onKeyUp: " + keyCode + " Satpad: none");
                    return 0;
                }
            }
        }

        return 0;
    }

}

class SSController extends BasicInputDevice {

    SSController( PadManagerV16 pdm){
       super(pdm);
    }

    public int onGenericMotionEvent(MotionEvent event ) {
        return super.onGenericMotionEvent(event);
    }
    public int onKeyDown(int keyCode, KeyEvent event ) {
        if( event.getScanCode() == 0 ){
            return 1;
        }
        if( keyCode == KeyEvent.KEYCODE_BACK ){
            Integer PadKey = Keymap.get(keyCode);
            if (PadKey != null) {
                YabauseRunnable.press(PadKey, _playerindex);
                return 1;
            }else{
                return 0;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
    public int onKeyUp(int keyCode, KeyEvent event ) {
        if( event.getScanCode() == 0 ){
            return 1;
        }
        if( keyCode == KeyEvent.KEYCODE_BACK ){
            Integer PadKey = Keymap.get(keyCode);
            if (PadKey != null) {
                YabauseRunnable.release(PadKey, _playerindex);
                return 1;
            }else{
                return 0;
            }
        }
        return super.onKeyUp(keyCode, event);
    }
}

class PadManagerV16 extends PadManager {
	
    private HashMap<String,Integer> deviceIds;
     final String TAG = "PadManagerV16";
    String DebugMesage = new String();
    boolean _testmode = false;

    int current_msg_index = 0;
    final int max_msg_index = 24;
    String [] DebugMesageArray;

    final int player_count = 2; 
    List<HashMap<Integer,Integer>> Keymap;

    BasicInputDevice pads[];

    public void loadSettings(  ){
        if( pads[0] != null ){
            pads[0].loadSettings("keymap.json");
        }
        if( pads[1] != null ){
            pads[1].loadSettings("keymap_player2.json");
        }
    }

    public void setTestMode( boolean mode ){
        _testmode = mode;
        if( pads[0] != null ) pads[0]._testmode = _testmode;
        if( pads[1] != null ) pads[1]._testmode = _testmode;
     }

    void addDebugString( String msg){
        DebugMesageArray[current_msg_index]= msg;
        current_msg_index++;
        if( current_msg_index >= max_msg_index) current_msg_index=0;
    }

    public String getStatusString(){
        DebugMesage = "";
        int start = current_msg_index;
        for( int i=0; i<max_msg_index; i++ ){
            if( !DebugMesageArray[start].equals("") ){
                DebugMesage = DebugMesageArray[start] + "\n" + DebugMesage;
                start--;
                if( start < 0 ){
                    start = (max_msg_index-1);
                }
            }
        }
        return DebugMesage;
    }
    
    PadManagerV16() {

        deviceIds = new HashMap<String,Integer>();

        pads = new BasicInputDevice[player_count];
        Keymap = new ArrayList<HashMap<Integer, Integer>>(); 
        for( int i=0; i<player_count; i++ ){
            pads[i] = null;
            //pads[i] = new BasicInputDevice(this);
            //pads[i]._selected_device_id = invalid_device_id;
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

        // Setting moe
        current_msg_index =0;
        DebugMesageArray = new String[max_msg_index];
        for( int i=0; i<max_msg_index; i++ ) {
            DebugMesageArray[i] = "";
        }
        _testmode = false;
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
                if( dev != null)
    	    	    return dev.getName();
                else
                    return null;
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
            pads[0] = new BasicInputDevice(this);
            pads[0]._selected_device_id = -1;
    		return;
    	}

    	Integer id = deviceIds.get(deviceid);
    	if( id == null ){
            pads[0] = new BasicInputDevice(this);
            pads[0]._selected_device_id = -1;
    	}else{
            InputDevice dev = InputDevice.getDevice(id);
            if( dev.getName().contains("HuiJia")){
                pads[0] = new SSController(this);
            }else{
                pads[0] = new BasicInputDevice(this);
            }
            pads[0]._selected_device_id = id;
    	}

    	pads[0]._playerindex = 0;
      pads[0].loadSettings("keymap.json");
      pads[0]._testmode = _testmode;
    	return;
    }
    
    public int getPlayer1InputDevice(){
        if( pads[0] == null ) return -1;
    	return pads[0]._selected_device_id;
    }
    
    public void setPlayer2InputDevice( String deviceid ){


        if( deviceid == null ){
            pads[1] = new BasicInputDevice(this);
            pads[1]._selected_device_id = -1;
            return;
        }

        Integer id = deviceIds.get(deviceid);
        if( id == null ){
            pads[1] = new BasicInputDevice(this);
            pads[1]._selected_device_id = -1;
        }else{
            InputDevice dev = InputDevice.getDevice(id);
            if( dev.getName().contains("HuiJia")){
                pads[1] = new SSController(this);
            }else{
                pads[1] = new BasicInputDevice(this);
            }
            pads[1]._selected_device_id = id;
        }

        pads[1]._playerindex = 1;
        pads[1].loadSettings("keymap_player2.json");
        pads[1]._testmode = _testmode;
        return;
    }
    
    public int getPlayer2InputDevice(){
        if( pads[1] == null ) return -1;
    	return pads[1]._selected_device_id;
    }


    BasicInputDevice findPlayerPad( int deviceid ){
        for( int i=0; i< player_count; i++ ){
            if( pads[i] != null && deviceid == pads[i]._selected_device_id){
                return pads[i];
            }
        }
        return null;
    }
    
    public int onGenericMotionEvent(MotionEvent event){

        if( pads[0] != null && pads[0]._selected_device_id == event.getDeviceId() ){
            pads[0].onGenericMotionEvent(event);
        }
        if( pads[1] != null && pads[1]._selected_device_id == event.getDeviceId() ){
            pads[1].onGenericMotionEvent(event);
        }

    	return 0;
    }

    public int onKeyDown(int keyCode, KeyEvent event) {

        int rtn = 0;
        if( pads[0] != null && pads[0]._selected_device_id == event.getDeviceId() ){
           rtn |=  pads[0].onKeyDown(keyCode, event);
        }
        if( pads[1] != null && pads[1]._selected_device_id == event.getDeviceId() ){
            rtn |= pads[1].onKeyDown(keyCode, event);
        }

        return rtn;
    }

    public int onKeyUp(int keyCode, KeyEvent event) {
        int rtn = 0;
        if( pads[0] != null && pads[0]._selected_device_id == event.getDeviceId() ){
            rtn |= pads[0].onKeyUp(keyCode, event);
        }
        if( pads[1] != null && pads[1]._selected_device_id == event.getDeviceId() ){
            rtn |= pads[1].onKeyUp(keyCode, event);
        }

        return rtn;
    }

}

