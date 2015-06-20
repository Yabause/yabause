package org.yabause.android;

import android.content.Context;
import android.os.Build;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.yabause.android.PadEvent;
import org.yabause.android.PadManagerV8;
import org.yabause.android.PadManagerV16;

abstract class PadManager {

    public abstract boolean hasPad();
    public abstract PadEvent onKeyDown(int keyCode, KeyEvent event);
    public abstract PadEvent onKeyUp(int keyCode, KeyEvent event);
    public abstract PadEvent onGenericMotionEvent(MotionEvent event);
    
    static PadManager getPadManager() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            return new PadManagerV16();
        } else {
            return new PadManagerV8();
        }
    }
   

}
