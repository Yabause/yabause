package org.uoyabause.android;

import android.content.Context;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.uoyabause.android.PadEvent;
import org.uoyabause.android.PadManager;

class PadManagerV8 extends PadManager {

    public boolean hasPad() {
        return false;
    }

    public int onKeyDown(int keyCode, KeyEvent event) {
        return 0;
    }

    public int onKeyUp(int keyCode, KeyEvent event) {
        return 0;
    }
    
    public int onGenericMotionEvent(MotionEvent event){
    	return 0;
    }
    
    public String getDeviceList(){
    	return new String("Nothing");
    }
    
    public int getId( int index ){ return -1; }    
    
    public int getDeviceCount(){ return 0; }
    public String getName( int index ){ return "Nothing"; }
    public void setPlayer1InputDevice( int index ){}
    public int getPlayer1InputDevice(){ return -1; }
    
 }
