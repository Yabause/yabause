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

    public PadEvent onKeyDown(int keyCode, KeyEvent event) {
        return null;
    }

    public PadEvent onKeyUp(int keyCode, KeyEvent event) {
        return null;
    }
    
    public PadEvent onGenericMotionEvent(MotionEvent event){
    	return null;
    }    
 }
