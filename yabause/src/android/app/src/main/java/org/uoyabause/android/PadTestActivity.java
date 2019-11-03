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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.os.Bundle;
import android.preference.PreferenceManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import org.uoyabause.uranus.R;

public class PadTestActivity extends AppCompatActivity implements PadTestFragment.PadTestListener {

  YabausePad mPadView;
  SeekBar mSlide;
  SeekBar mTransSlide;
  private PadManager padm;
  TextView tv;

  private static final int CONTENT_VIEW_ID = 10101010;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
    boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
    if (lock_landscape == true) {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    } else {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
    }

    super.onCreate(savedInstanceState);

    requestWindowFeature(Window.FEATURE_NO_TITLE);

    // Immersive mode
    View decor = this.getWindow().getDecorView();
    decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE);

    FrameLayout frame = new FrameLayout(this);
    frame.setId(CONTENT_VIEW_ID);
    setContentView(frame, new FrameLayout.LayoutParams(
        FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

    if (savedInstanceState == null) {
      Fragment newFragment = new PadTestFragment();
      ((PadTestFragment) newFragment).setListener(this);
      FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
      ft.add(CONTENT_VIEW_ID,newFragment,PadTestFragment.TAG).commit();
    }
  }

  @Override
  public void onBackPressed() {
    PadTestFragment fg2 = (PadTestFragment)getSupportFragmentManager().findFragmentByTag(PadTestFragment.TAG);
    if( fg2 != null ){
      fg2.onBackPressed();
      return;
    }
    super.onBackPressed();
  }
/*
  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {


    int action = event.getAction();
    int keyCode = event.getKeyCode();
    //Log.d("dispatchKeyEvent","device:" + event.getDeviceId() + ",action:" + action +",keyCoe:" + keyCode );
    if (action == KeyEvent.ACTION_UP) {
      int rtn = padm.onKeyUp(keyCode, event);
      if (rtn != 0) {
        tv.setText(padm.getStatusString());
        tv.invalidate();
        return true;
      }
      tv.setText(padm.getStatusString());
      tv.invalidate();

    } else if (action == KeyEvent.ACTION_MULTIPLE) {

    } else if (action == KeyEvent.ACTION_DOWN) {

      if (keyCode == KeyEvent.KEYCODE_BACK) {
        return super.dispatchKeyEvent(event);
      }

      int rtn = padm.onKeyDown(keyCode, event);
      if (rtn != 0) {
        tv.setText(padm.getStatusString());
        tv.invalidate();
        return true;
      }
      tv.setText(padm.getStatusString());
      tv.invalidate();

    }

    return super.dispatchKeyEvent(event);
  }

  @Override
  public boolean onPad(PadEvent event) {
    TextView tv = (TextView) findViewById(R.id.text_status);
    tv.setText(mPadView.getStatusString());
    tv.invalidate();
    return true;
  }
*/

  @Override
  public void onFinish() {
    this.finish();
  }

  @Override
  public void onCancel() {
    this.finish();
  }
}
