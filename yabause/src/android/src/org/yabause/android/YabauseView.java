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

public class YabauseView extends SurfaceView implements Callback {
    private static String TAG = "YabauseView";
    private static final boolean DEBUG = false; 

    private EGLContext mEglContext;
    private EGLDisplay mEglDisplay;
    private EGLSurface mEglSurface;
    private EGLConfig mEglConfig;

    public YabauseView(Context context, AttributeSet attrs) {
        super(context,attrs);
        init();
    }   
     
    public YabauseView(Context context) {
        super(context); 
        init();
    } 

    private void init() {
       getHolder().addCallback(this);
    }
    
    private boolean initGLES(){

        EGL10 egl = (EGL10)EGLContext.getEGL();
        
        mEglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if( mEglDisplay == EGL10.EGL_NO_DISPLAY ){
            Log.e(TAG, "Fail to get Display");
            return false;
        }
            
        int[] version = new int[2];
        if( !egl.eglInitialize(mEglDisplay, version) ){
            Log.e(TAG, "Fail to eglInitialize");
            return false;
        }
        
        int[] configSpec = {
             EGL10.EGL_NONE
         };
            EGLConfig[] configs = new EGLConfig[1];
        
        int[] numConfigs = new int[1];
        if( !egl.eglChooseConfig(mEglDisplay, configSpec, configs, 1, numConfigs) ){
            Log.e(TAG, "Fail to Choose Config");
            return false;
        }
        mEglConfig = configs[0];

        mEglContext = egl.eglCreateContext(mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT, null);
        if( mEglContext == EGL10.EGL_NO_CONTEXT ){
            Log.e(TAG, "Fail to Create OpenGL Context");
            return false;
        }
        return true;
    }
    
    
    private boolean createSurface(){
        EGL10 egl = (EGL10)EGLContext.getEGL();
        mEglSurface = egl.eglCreateWindowSurface(mEglDisplay, mEglConfig, getHolder(), null);
        if( mEglSurface == EGL10.EGL_NO_SURFACE ){
            return false;
        }
        return true;
    }   
    
    private void endGLES(){
        EGL10 egl = (EGL10)EGLContext.getEGL();
        if( mEglSurface != null){
            egl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
            
            egl.eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = null;
        }
        
        if( mEglContext != null ){
            egl.eglDestroyContext(mEglDisplay, mEglContext);
            mEglContext = null;
        }
        
        if( mEglDisplay != null){
            egl.eglTerminate(mEglDisplay);
            mEglDisplay = null;
        }
    }   
   
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        EGL10 egl = (EGL10)EGLContext.getEGL();
          
        YabauseRunnable.lockGL();
        egl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);     
        YabauseRunnable.initViewport();
        YabauseRunnable.unlockGL();
        
    }
 
    @Override
    public void surfaceCreated(SurfaceHolder holder) {

        initGLES();
        if( !createSurface() ){
             Log.e(TAG, "Fail to Create Surface");
             return ;
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

        endGLES();

        YabauseRunnable.cleanup();
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
