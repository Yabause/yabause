/*  Copyright 2011 Guillaume Duhamel

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

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;

class YabauseRunnable implements Runnable
{
    public static native void init(Bitmap bitmap);
    public static native void exec();
    private boolean paused;
    private Handler handler;

    public YabauseRunnable(Bitmap bitmap)
    {
        handler = new Handler();
        init(bitmap);
    }

    public void pause()
    {
        Log.v("Yabause", "pause... should really pause emulation now...");
        paused = true;
    }

    public void resume()
    {
        Log.v("Yabause", "resuming emulation...");
        paused = false;
        handler.post(this);
    }

    public void run()
    {
        if (! paused)
        {
            exec();
            
            handler.post(this);
        }
    }

    public boolean paused()
    {
        return paused;
    }
}

public class Yabause extends Activity
{
    private static final String TAG = "Yabause";
    private YabauseRunnable yabauseThread;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        YabauseView view = new YabauseView(this);
        setContentView(view);

        yabauseThread = new YabauseRunnable(view.bitmap);
    }

    @Override
    public void onPause()
    {
        super.onPause();
        Log.v(TAG, "pause... should pause emulation...");
        yabauseThread.pause();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        Log.v(TAG, "resume... should resume emulation...");
        yabauseThread.resume();
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        Log.v(TAG, "this is the end...");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.emulation, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.pause:
            yabauseThread.pause();
            return true;
        case R.id.quit:
            this.finish();
            return true;
        case R.id.resume:
            yabauseThread.resume();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (yabauseThread.paused()) {
            menu.setGroupVisible(R.id.paused, true);
            menu.setGroupVisible(R.id.running, false);
        } else {
            menu.setGroupVisible(R.id.paused, false);
            menu.setGroupVisible(R.id.running, true);
        }
        return true;
    }

    static {
        System.loadLibrary("yabause");
    }
}

class YabauseView extends View {
    public Bitmap bitmap;

    public YabauseView(Context context) {
        super(context);

        final int W = 320;
        final int H = 240;

        bitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
    }

    @Override protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(bitmap, 0, 0, null);
        invalidate();
    }
}
