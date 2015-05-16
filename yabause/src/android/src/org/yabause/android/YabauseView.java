/*  Copyright 2011-2013 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

package org.yabause.android;

import java.lang.Runnable;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;

class YabauseView extends SurfaceView implements Callback {
    private static String TAG = "YabauseView";
    private static final boolean DEBUG = false;

    private int axisX = 0;
    private int axisY = 0;

    public boolean[] pointers = new boolean[256];
    public int[] pointerX = new int[256];
    public int[] pointerY = new int[256];


    public YabauseView(Context context, AttributeSet attrs) {
        super(context,attrs);
        init(false, 0, 0);
    }

    public YabauseView(Context context) {
        super(context);
        init(false, 0, 0);
    }

    public YabauseView(Context context, boolean translucent, int depth, int stencil) {
        super(context);
        init(translucent, depth, stencil);
    }

    private void init(boolean translucent, int depth, int stencil) {
       getHolder().addCallback(this);
       getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
      YabauseRunnable.lockGL();
      YabauseRunnable.initViewport(holder.getSurface(),width, height);
      YabauseRunnable.unlockGL();

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int specw = MeasureSpec.getSize(widthMeasureSpec);
        int spech = MeasureSpec.getSize(heightMeasureSpec);
        float specratio = (float) specw / spech;
        int saturnw = 320;
        int saturnh = 224;
        float saturnratio = (float) saturnw / saturnh;
        float revratio = (float) saturnh / saturnw;

        if (specratio > saturnratio) {
            setMeasuredDimension((int) (spech * saturnratio), spech);
        } else {
            setMeasuredDimension(specw, (int) (specw * revratio));
        }
    }
}
