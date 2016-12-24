package org.uoyabause.android;

import android.content.Context;
import android.os.Build;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.uoyabause.android.PadEvent;
import org.uoyabause.android.PadManagerV16;
import org.uoyabause.android.PadManagerV8;

abstract class PadManager {

	private static PadManager _instance = null; 
    public static final int invalid_device_id = 65535; 

    public static final int MODE_HAT=0;
    public static final int MODE_ANALOG=1;

    protected int _mode = MODE_HAT;

    public abstract boolean hasPad();
    public abstract int onKeyDown(int keyCode, KeyEvent event);
    public abstract int onKeyUp(int keyCode, KeyEvent event);
    public abstract int onGenericMotionEvent(MotionEvent event);
    public abstract String getDeviceList();
    
    public abstract int getDeviceCount();
    
    public abstract String getName( int index );
    public abstract String getId( int index );
    
    public abstract void setPlayer1InputDevice( String id );
    public abstract int getPlayer1InputDevice();
    public abstract void setPlayer2InputDevice( String id );
    public abstract int getPlayer2InputDevice();
    public abstract void loadSettings();

    public void setAnalogMode( int mode ){_mode = mode; };
    public int getAnalogMode(){ return _mode; };
    
    static PadManager getPadManager() {
    	if( _instance == null ){
	        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
	        	_instance = new PadManagerV16();
	        } else {
	        	_instance = new PadManagerV8();
	        }
    	}
    	
    	return _instance;
    }

    public abstract String getStatusString();
    public abstract void setTestMode( boolean mode );
   

}
