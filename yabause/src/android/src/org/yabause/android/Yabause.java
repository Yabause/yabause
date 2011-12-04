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
