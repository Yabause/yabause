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

import android.util.Log;
import android.view.Surface;

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
    public static native String savestate_compress( String path );
    public static native void loadstate_compress( String path );
    public static native int record( String path );
    public static native int play( String path );
    public static native int getRecordingStatus();
    public static native void pause();
    public static native void resume();
    public static native void setPolygonGenerationMode( int pg );
    public static native void setAspectRateMode( int ka);
    public static native void setSoundEngine( int engine );
    public static native void setResolutionMode( int resoution_mode );
    public static native void setRbgResolutionMode( int resoution_mode );

    public static native void openTray();
    public static native void closeTray();
    public static native void switch_padmode( int mode );
    public static native void switch_padmode2( int mode );
    public static native void updateCheat( String[] cheat_code );
    public static native void setScspSyncPerFrame( int scsp_sync_count );
    public static native void setCpuSyncPerLine( int cpu_sync_count );
    public static native void setScspSyncTimeMode( int mode );

    public static native String getDevicelist( );
    public static native String getFilelist( int deviceid  );
    public static native int deletefile( int index );
    public static native String getFile( int index  );
    public static native String putFile( String path);
    public static native int copy( int target_device, int file_index );
    public static native byte[] getGameinfoFromChd( String path );

    public static native int enableBackupWriteHook();

    public static native void setFrameLimitMode( int mode );

    private boolean inited;
    private boolean paused;

    static final int IDLE = -1;
    static final int RECORDING = 0;
    static final int PLAYING = 1;

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
