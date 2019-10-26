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

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SeekBar;
import android.widget.TextView;
import androidx.fragment.app.Fragment;
import org.uoyabause.android.backup.TabBackupFragment;
import org.uoyabause.uranus.R;

public class PadTestFragment extends Fragment implements org.uoyabause.android.YabausePad.OnPadListener{

  YabausePad mPadView;
  SeekBar mSlide;
  SeekBar mTransSlide;
  private PadManager padm;
  TextView tv;

  public static final String TAG = "PadTestFragment";

  public interface PadTestListener{
    public void onFinish();
    public void onCancel();
  }
  PadTestListener listener_ = null;

  public static PadTestFragment newInstance(String param1, String param2) {
    PadTestFragment fragment = new PadTestFragment();
    Bundle args = new Bundle();
    fragment.setArguments(args);
    return fragment;
  }

  public void setListener( PadTestListener listner ){
    listener_ = listner;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container,
                           Bundle savedInstanceState) {

    View rootView = inflater.inflate(R.layout.padtest, container, false);

    padm = PadManager.getPadManager();
    padm.setTestMode(true);
    padm.loadSettings();

    mPadView = (YabausePad)rootView.findViewById(R.id.yabause_pad);
    mPadView.setTestmode(true);
    mPadView.setOnPadListener(this);
    mPadView.show(true);

    mSlide   = (SeekBar)rootView.findViewById(R.id.button_scale);
    mSlide.setProgress( (int)(mPadView.getScale()*100.0f) );
    mSlide.setOnSeekBarChangeListener(
        new SeekBar.OnSeekBarChangeListener() {
          public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            mPadView.setScale((float) progress / 100.0f);
            mPadView.requestLayout();
            mPadView.invalidate();
          }

          public void onStartTrackingTouch(SeekBar seekBar) {
          }

          public void onStopTrackingTouch(SeekBar seekBar) {
          }
        }
    );

    mTransSlide   = (SeekBar)rootView.findViewById(R.id.button_transparent);
    mTransSlide.setProgress( (int)(mPadView.getTrans()*100.0f) );
    mTransSlide.setOnSeekBarChangeListener(
        new SeekBar.OnSeekBarChangeListener() {
          public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            mPadView.setTrans((float) progress / 100.0f);
            mPadView.invalidate();
          }

          public void onStartTrackingTouch(SeekBar seekBar) {
          }

          public void onStopTrackingTouch(SeekBar seekBar) {
          }
        }
    );


    tv = (TextView)rootView.findViewById(R.id.text_status);

    return rootView;
  }


  public void onBackPressed() {
    AlertDialog.Builder alert = new AlertDialog.Builder(getActivity());
    alert.setTitle("");
    alert.setMessage(R.string.do_you_want_to_save_this_setting);
    alert.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int which) {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
        SharedPreferences.Editor editor = sharedPref.edit();
        float value = (float) mSlide.getProgress() / 100.0f;
        editor.putFloat("pref_pad_scale", value);
        editor.commit();
        value = (float) mTransSlide.getProgress() / 100.0f;
        editor.putFloat("pref_pad_trans", value);
        editor.commit();
        if (listener_ != null) listener_.onFinish();
      }
    });
    alert.setNegativeButton(R.string.no, new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int which) {
        if (listener_ != null) listener_.onCancel();
      }
    });
    alert.show();
  }

  @Override
  public boolean onPad(PadEvent event) {
    //TextView tv = (TextView)findViewById(R.id.text_status);
    tv.setText(mPadView.getStatusString());
    tv.invalidate();
    return true;
  }
}
