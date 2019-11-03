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
    
    public String getId( int index ){ return null; }    
    
    public int getDeviceCount(){ return 0; }
    public String getName( int index ){ return "Nothing"; }
    public int getPlayer1InputDevice(){ return -1; }

	@Override
	public void setPlayer1InputDevice(String id) {
		
	}
    
    public void setPlayer2InputDevice( String id ){}
    public int getPlayer2InputDevice(){ return 0; }
    public void loadSettings(){};
    public String getStatusString(){ return "none"; }
    public void setTestMode( boolean mode ){}
	
 }
