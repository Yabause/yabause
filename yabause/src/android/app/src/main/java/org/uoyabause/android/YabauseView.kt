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
package org.uoyabause.android

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceHolder
import android.view.SurfaceView

class YabauseView : SurfaceView, SurfaceHolder.Callback {
    /*
    private int axisX = 0;
    private int axisY = 0;

    public boolean[] pointers = new boolean[256];
    public int[] pointerX = new int[256];
    public int[] pointerY = new int[256];
*/
    var _context: Context

    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        _context = context
        init()
    }

    constructor(context: Context) : super(context) {
        _context = context
        init()
    }
/*
    constructor(context: Context, translucent: Boolean, depth: Int, stencil: Int) : super(context) {
        _context = context
        init()
    }
*/
    private fun init() {
        holder.addCallback(this)
        // getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        YabauseRunnable.lockGL()
        YabauseRunnable.initViewport(holder.surface, width, height)
        YabauseRunnable.unlockGL()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
    }
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        YabauseRunnable.lockGL()
        YabauseRunnable.initViewport(null, 0, 0)
        YabauseRunnable.unlockGL()
    } /*
    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    	/ *
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(_context);
        boolean keep_aspectrate = sharedPref.getBoolean("pref_keepaspectrate", false);

        if( keep_aspectrate ) {
	        int specw = MeasureSpec.getSize(widthMeasureSpec);
	        int spech = MeasureSpec.getSize(heightMeasureSpec);
	        float specratio = (float) specw / spech;
            float saturnw = 4.0f;
            float saturnh = 3.0f;

            if( sharedPref.getBoolean("pref_rotate_screen", false) ){
                saturnw = 3.0f;
                saturnh = 4.0f;
            }
            float saturnratio = (float) saturnw / saturnh;
	        float revratio = (float) saturnh / saturnw;

	        if (specratio > saturnratio) {
	            setMeasuredDimension((int) (spech * saturnratio), spech);
	        } else {
	            setMeasuredDimension(specw, (int) (specw * revratio));
	        }
        }else{
        	super.onMeasure(widthMeasureSpec,heightMeasureSpec);
        }
    }
*/

    companion object {
        private const val TAG = "YabauseView"
        private const val DEBUG = false
    }
}
