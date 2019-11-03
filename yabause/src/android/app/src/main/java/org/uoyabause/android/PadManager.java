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
    protected int _mode2 = MODE_HAT;

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

  public void setAnalogMode2( int mode ){_mode2 = mode; };
  public int getAnalogMode2(){ return _mode2; };

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

    static PadManager updatePadManager() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
             _instance = new PadManagerV16();
        } else {
            _instance = new PadManagerV8();
        }
        return _instance;
    }

    public abstract String getStatusString();
    public abstract void setTestMode( boolean mode );

  public interface ShowMenuListener {
    void show();
  }

  public void showMenu(){
    if( showMenulistener != null ){
      showMenulistener.show();
    }
  }

  protected ShowMenuListener showMenulistener = null;

  public void setShowMenulistener( ShowMenuListener listener ){
    showMenulistener = listener;
  }

   

}
