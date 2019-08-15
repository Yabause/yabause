package org.uoyabause.android;

import android.util.Log;
import android.view.Surface;

/**
 * Created by shinya on 2017/08/27.
 */

public class YabauseRunnable implements Runnable
{
    public static native int init(Yabause yabause);
    public static native void deinit();
    public static native void exec();
    public static native void reset();
    public static native void press(int key, int player);
    public static native void axis(int key, int player, int val);
    public static native void release(int key, int player);
    public static native int initViewport(Surface sf, int width, int hieght);
    public static native int drawScreen();
    public static native int lockGL();
    public static native int unlockGL();
    public static native void enableFPS(int enable);
    public static native void enableExtendedMemory(int enable);
    public static native void enableRotateScreen(int enable);
    public static native void enableComputeShader(int enable);
    public static native void enableFrameskip(int enable);
    public static native void setCpu( int cpu );
    public static native void setFilter( int filter );
    public static native void setVolume(int volume);
    public static native int screenshot( String filename );
    public static native String getCurrentGameCode();
    public static native String getGameTitle();
    public static native String getGameinfo();
    public static native String savestate( String path );
    public static native void loadstate( String path );
    public static native void pause();
    public static native void resume();
    public static native void setPolygonGenerationMode( int pg );
    public static native void setKeepAspect( int ka);
    public static native void setSoundEngine( int engine );
    public static native void setResolutionMode( int resoution_mode );
    public static native void setRbgResolutionMode( int resoution_mode );

    public static native void openTray();
    public static native void closeTray();
    public static native void switch_padmode( int mode );
    public static native void updateCheat( String[] cheat_code );
    public static native void setScspSyncPerFrame( int scsp_sync_count );
    public static native void setCpuSyncPerLine( int cpu_sync_count );
    public static native void setScspSyncTimeMode( int mode );

    public static native String getDevicelist( );
    public static native String getFilelist( int deviceid  );
    public static native int deletefile( int index );
    public static native String getFile( int index  );
    public static native String putFile( );
    public static native int copy( int target_device, int file_index );
    public static native String getGameinfoFromChd( String path );

    private boolean inited;
    private boolean paused;

    public YabauseRunnable(Yabause yabause)
    {
        int ok = init(yabause);
        Log.v("Yabause", "init = " + ok);
        inited = (ok == 0);
    }

    public void destroy()
    {
        Log.v("Yabause", "destroying yabause...");
        inited = false;
        deinit();
    }

    public void run()
    {
        if (inited && (! paused))
        {
            exec();
        }
    }

}
