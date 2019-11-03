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

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.util.TypedValue;
import android.view.*;
import android.view.InputDevice;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.uranus.R;

import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;

import static android.content.Context.LAYOUT_INFLATER_SERVICE;

public class InputSettingFragment extends DialogFragment
    implements DialogInterface.OnKeyListener, View.OnGenericMotionListener, View.OnClickListener
{
  private TextView key_message;
  private Button skip;
  private HashMap<Integer, Integer> Keymap;
  private ArrayList<Integer> map;
  private int index = 0;
  private PadManager pad_m;
  Context context_m;
  private int _selected_device_id = 0;
  private String save_filename = "keymap";
  private int playerid = 0;

  public interface InputSettingListener{
    public void onFinishInputSetting(  );
    public void onCancel( );
  }
  InputSettingListener listener_ = null;

  public void setListener( InputSettingListener listener ){
    listener_ = listener;
  }

  class MotionMap {
    MotionMap(int id) {
      this.id = id;
    }
    int id = 0;
    float oldval = 0.0f;
  }
  ArrayList<MotionMap> motions;

  public void setPlayerAndFilename(int playerid, String fname) {
    this.playerid = playerid;
    this.save_filename = fname;
  }

  void InitObjects(Context context) {
    index = 0;
    context_m = context;
    Keymap = new HashMap<Integer, Integer>();
    map = new ArrayList<Integer>();
    map.add(PadEvent.BUTTON_UP);
    map.add(PadEvent.BUTTON_DOWN);
    map.add(PadEvent.BUTTON_LEFT);
    map.add(PadEvent.BUTTON_RIGHT);
    map.add(PadEvent.BUTTON_LEFT_TRIGGER);
    map.add(PadEvent.BUTTON_RIGHT_TRIGGER);
    map.add(PadEvent.BUTTON_START);
    map.add(PadEvent.BUTTON_A);
    map.add(PadEvent.BUTTON_B);
    map.add(PadEvent.BUTTON_C);
    map.add(PadEvent.BUTTON_X);
    map.add(PadEvent.BUTTON_Y);
    map.add(PadEvent.BUTTON_Z);
    map.add(PadEvent.PERANALOG_AXIS_X); // left to right
    map.add(PadEvent.PERANALOG_AXIS_Y); // up to down
    map.add(PadEvent.PERANALOG_AXIS_LTRIGGER); // left trigger
    map.add(PadEvent.PERANALOG_AXIS_RTRIGGER); // right trigger

    //setDialogTitle(R.string.input_the_key);
    //setPositiveButtonText(null);  // OKボタンを非表示にする
    //setDialogLayoutResource(R.layout.keymap);

    motions = new ArrayList<MotionMap>();
    motions.add(new MotionMap(MotionEvent.AXIS_X));
    motions.add(new MotionMap(MotionEvent.AXIS_Y));
    motions.add(new MotionMap(MotionEvent.AXIS_PRESSURE));
    motions.add(new MotionMap(MotionEvent.AXIS_SIZE));
    motions.add(new MotionMap(MotionEvent.AXIS_TOUCH_MAJOR));
    motions.add(new MotionMap(MotionEvent.AXIS_TOUCH_MINOR));
    motions.add(new MotionMap(MotionEvent.AXIS_TOOL_MAJOR));
    motions.add(new MotionMap(MotionEvent.AXIS_TOOL_MINOR));
    motions.add(new MotionMap(MotionEvent.AXIS_ORIENTATION));
    motions.add(new MotionMap(MotionEvent.AXIS_VSCROLL));
    motions.add(new MotionMap(MotionEvent.AXIS_HSCROLL));
    motions.add(new MotionMap(MotionEvent.AXIS_Z));
    motions.add(new MotionMap(MotionEvent.AXIS_RX));
    motions.add(new MotionMap(MotionEvent.AXIS_RY));
    motions.add(new MotionMap(MotionEvent.AXIS_RZ));
    motions.add(new MotionMap(MotionEvent.AXIS_HAT_X));
    motions.add(new MotionMap(MotionEvent.AXIS_HAT_Y));
    motions.add(new MotionMap(MotionEvent.AXIS_LTRIGGER));
    motions.add(new MotionMap(MotionEvent.AXIS_RTRIGGER));
    motions.add(new MotionMap(MotionEvent.AXIS_THROTTLE));
    motions.add(new MotionMap(MotionEvent.AXIS_RUDDER));
    motions.add(new MotionMap(MotionEvent.AXIS_WHEEL));
    motions.add(new MotionMap(MotionEvent.AXIS_GAS));
    motions.add(new MotionMap(MotionEvent.AXIS_BRAKE));
    motions.add(new MotionMap(MotionEvent.AXIS_DISTANCE));
    motions.add(new MotionMap(MotionEvent.AXIS_TILT));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_1));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_2));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_3));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_4));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_5));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_6));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_7));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_8));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_9));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_10));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_11));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_12));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_13));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_14));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_15));
    motions.add(new MotionMap(MotionEvent.AXIS_GENERIC_16));
  }

  public Dialog onCreateDialog(Bundle savedInstanceState){
    InitObjects(getActivity());

    AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(getActivity());
    Resources res = getResources();

    LayoutInflater inflater = (LayoutInflater)getActivity().getLayoutInflater();
    View view = inflater.inflate(R.layout.keymap, null, false);
    dialogBuilder.setView(view);

    dialogBuilder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int whichButton) {
        if( listener_ != null ){
          listener_.onCancel();
        }
      }
    });

    Dialog dlg = dialogBuilder.create();
    dlg.setOnKeyListener(this);

    index = 0;
    Keymap.clear();

    pad_m = PadManager.getPadManager();
    if (pad_m.hasPad() == false) {
      Toast.makeText(context_m, R.string.joystick_is_not_connected, Toast.LENGTH_LONG).show();
      if( listener_ != null ){
        listener_.onCancel();
      }
      dlg.dismiss();
    }

    if (this.playerid == 1) {
      _selected_device_id = pad_m.getPlayer2InputDevice();
    } else {
      _selected_device_id = pad_m.getPlayer1InputDevice();
    }

    view.setOnGenericMotionListener(this);
    key_message = (TextView) view.findViewById(R.id.text_key);
    key_message.setText(res.getString(R.string.up));
    key_message.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 32);
    key_message.setClickable(false);
    key_message.setOnGenericMotionListener(this);
    key_message.setFocusableInTouchMode(true);
    key_message.requestFocus();
    View button = (Button) view.findViewById(R.id.button_skip);
    button.setOnClickListener(this);

    return dlg;
  }

  public void saveSettings() {
    JSONObject jsonObject = new JSONObject();
    try {
      // Inisiazlie
      Integer dmykey = 65535;
      jsonObject.put("BUTTON_UP", dmykey);
      jsonObject.put("BUTTON_DOWN", dmykey);
      jsonObject.put("BUTTON_LEFT", dmykey);
      jsonObject.put("BUTTON_RIGHT", dmykey);
      jsonObject.put("BUTTON_LEFT_TRIGGER", dmykey);
      jsonObject.put("BUTTON_RIGHT_TRIGGER", dmykey);
      jsonObject.put("BUTTON_START", dmykey);
      jsonObject.put("BUTTON_A", dmykey);
      jsonObject.put("BUTTON_B", dmykey);
      jsonObject.put("BUTTON_C", dmykey);
      jsonObject.put("BUTTON_X", dmykey);
      jsonObject.put("BUTTON_Y", dmykey);
      jsonObject.put("BUTTON_Z", dmykey);

      jsonObject.put("PERANALOG_AXIS_X", dmykey);
      jsonObject.put("PERANALOG_AXIS_Y", dmykey);
      jsonObject.put("PERANALOG_AXIS_LTRIGGER", dmykey);
      jsonObject.put("PERANALOG_AXIS_RTRIGGER", dmykey);

      for (HashMap.Entry<Integer, Integer> entry : Keymap.entrySet()) {
        // キーを取得
        Integer key = entry.getKey();
        // 値を取得
        Integer val = entry.getValue();
        switch (val) {
          case PadEvent.BUTTON_UP:
            jsonObject.put("BUTTON_UP", key);
            break;
          case PadEvent.BUTTON_DOWN:
            jsonObject.put("BUTTON_DOWN", key);
            break;
          case PadEvent.BUTTON_LEFT:
            jsonObject.put("BUTTON_LEFT", key);
            break;
          case PadEvent.BUTTON_RIGHT:
            jsonObject.put("BUTTON_RIGHT", key);
            break;
          case PadEvent.BUTTON_LEFT_TRIGGER:
            jsonObject.put("BUTTON_LEFT_TRIGGER", key);
            break;
          case PadEvent.BUTTON_RIGHT_TRIGGER:
            jsonObject.put("BUTTON_RIGHT_TRIGGER", key);
            break;
          case PadEvent.BUTTON_START:
            jsonObject.put("BUTTON_START", key);
            break;
          case PadEvent.BUTTON_A:
            jsonObject.put("BUTTON_A", key);
            break;
          case PadEvent.BUTTON_B:
            jsonObject.put("BUTTON_B", key);
            break;
          case PadEvent.BUTTON_C:
            jsonObject.put("BUTTON_C", key);
            break;
          case PadEvent.BUTTON_X:
            jsonObject.put("BUTTON_X", key);
            break;
          case PadEvent.BUTTON_Y:
            jsonObject.put("BUTTON_Y", key);
            break;
          case PadEvent.BUTTON_Z:
            jsonObject.put("BUTTON_Z", key);
            break;
          case PadEvent.PERANALOG_AXIS_X:
            jsonObject.put("PERANALOG_AXIS_X", key);
            break;
          case PadEvent.PERANALOG_AXIS_Y:
            jsonObject.put("PERANALOG_AXIS_Y", key);
            break;
          case PadEvent.PERANALOG_AXIS_LTRIGGER:
            jsonObject.put("PERANALOG_AXIS_LTRIGGER", key);
            break;
          case PadEvent.PERANALOG_AXIS_RTRIGGER:
            jsonObject.put("PERANALOG_AXIS_RTRIGGER", key);
            break;
        }
      }

      // jsonファイル出力
      File file = new File(Environment.getExternalStorageDirectory() + "/" + "yabause/" + save_filename + ".json");
      FileWriter filewriter;

      filewriter = new FileWriter(file);
      BufferedWriter bw = new BufferedWriter(filewriter);
      PrintWriter pw = new PrintWriter(bw);
      pw.write(jsonObject.toString());
      pw.close();

    } catch (IOException e) {
      e.printStackTrace();
    } catch (JSONException e) {
      e.printStackTrace();
    }

  }

  int keystate_ = 0;
  Handler h;

  void setMessage(String str) {
    key_message.setText(str);
  }
  boolean setKeymap(Integer padkey) {

    if (keystate_ != 0) return true;
    keystate_ = 1;
    h = new Handler();
    new Thread(new Runnable() {
      public void run() {
        h.postDelayed(new Runnable() {
          public void run() {
            keystate_ = 0;
          }
        }, 300);
      }
    }).start();
    Keymap.put(padkey, map.get(index));
    Log.d("setKeymap", "index =" + index + ": pad = " + padkey);
    index++;


    if (index >= map.size()) {
      saveSettings();
      if( listener_ != null ){ listener_.onFinishInputSetting(); }
      Dialog dlg = this.getDialog();
      dlg.dismiss();
      return true;
    }

    Resources res = context_m.getResources();

    switch (map.get(index)) {
      case PadEvent.BUTTON_UP:
        setMessage(res.getString(R.string.up));
        break;
      case PadEvent.BUTTON_DOWN:
        setMessage(res.getString(R.string.down));
        break;
      case PadEvent.BUTTON_LEFT:
        setMessage(res.getString(R.string.left));
        break;
      case PadEvent.BUTTON_RIGHT:
        setMessage(res.getString(R.string.right));
        break;
      case PadEvent.BUTTON_LEFT_TRIGGER:
        setMessage(res.getString(R.string.l_trigger));
        break;
      case PadEvent.BUTTON_RIGHT_TRIGGER:
        setMessage(res.getString(R.string.r_trigger));
        break;
      case PadEvent.BUTTON_START:
        setMessage(res.getString(R.string.start));
        break;
      case PadEvent.BUTTON_A:
        setMessage(res.getString(R.string.a_button));
        break;
      case PadEvent.BUTTON_B:
        setMessage(res.getString(R.string.b_button));
        break;
      case PadEvent.BUTTON_C:
        setMessage(res.getString(R.string.c_button));
        break;
      case PadEvent.BUTTON_X:
        setMessage(res.getString(R.string.x_button));
        break;
      case PadEvent.BUTTON_Y:
        setMessage(res.getString(R.string.y_button));
        break;
      case PadEvent.BUTTON_Z:
        setMessage(res.getString(R.string.z_button));
        break;
      case PadEvent.PERANALOG_AXIS_X:
        setMessage(res.getString(R.string.axis_x));
        break;
      case PadEvent.PERANALOG_AXIS_Y:
        setMessage(res.getString(R.string.axis_y));
        break;
      case PadEvent.PERANALOG_AXIS_LTRIGGER:
        setMessage(res.getString(R.string.axis_l));
        break;
      case PadEvent.PERANALOG_AXIS_RTRIGGER:
        setMessage(res.getString(R.string.axis_r));
        break;
    }

    return true;

  }

  private final int KEYCODE_L2 = 104;
  private final int KEYCODE_R2 = 105;
  boolean onkey = false;

  @Override
  public boolean onKey(DialogInterface dialogInterface, int keyCode, KeyEvent event) {
    if (event.getDeviceId() != _selected_device_id) return false;

    if (((event.getSource() & android.view.InputDevice.SOURCE_GAMEPAD) == android.view.InputDevice.SOURCE_GAMEPAD) ||
        ((event.getSource() & android.view.InputDevice.SOURCE_JOYSTICK) == android.view.InputDevice.SOURCE_JOYSTICK)) {

      android.view.InputDevice dev = InputDevice.getDevice(_selected_device_id);
      if (dev.getName().contains("HuiJia")) {
        if (event.getScanCode() == 0) {
          return false;
        }
      }

      Log.d("Yabause", "key:" + event.getScanCode() + " value: " + event.getAction());

      if (map.get(index) == PadEvent.PERANALOG_AXIS_X || map.get(index) == PadEvent.PERANALOG_AXIS_Y ||
          map.get(index) == PadEvent.PERANALOG_AXIS_LTRIGGER || map.get(index) == PadEvent.PERANALOG_AXIS_RTRIGGER) {
        onkey = false;
        return true;
      }

      if (event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_UP) {
        onkey = false;
      }

      if (event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_DOWN) {
        onkey = true;
        if (keyCode == 0) {
          keyCode = event.getScanCode();
        }
        Integer PadKey = Keymap.get(keyCode);
        return setKeymap(keyCode);
      }
    }
    return false;
  }

  @Override
  public void onClick(View view) {
    if (view.getId() == R.id.button_skip) {
      setKeymap(-1);
    }
  }

  @Override
  public boolean onGenericMotion(View view, MotionEvent event) {
    if (event.getDeviceId() != _selected_device_id) return false;
    if (onkey) return false;


    if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {
      for (int i = 0; i < motions.size(); i++) {
        float value = event.getAxisValue(motions.get(i).id);

        if (value != 0.0) {
          Log.d("Yabause", "key:" + motions.get(i).id + " value:" + value);
        }

        InputDevice dev = InputDevice.getDevice(_selected_device_id);
        if (dev.getName().contains("Moga") && motions.get(i).id == 32) {
          continue;
        }

        if (map.get(index) == PadEvent.PERANALOG_AXIS_X || map.get(index) == PadEvent.PERANALOG_AXIS_Y ||
            map.get(index) == PadEvent.PERANALOG_AXIS_LTRIGGER || map.get(index) == PadEvent.PERANALOG_AXIS_RTRIGGER) {
          if (Float.compare(value, -1.0f) <= 0 || Float.compare(value, 1.0f) >= 0) {
            motions.get(i).oldval = value;
            Integer PadKey = Keymap.get(motions.get(i).id);
            if (PadKey == null) {
              return setKeymap(motions.get(i).id);
            }
          }
        } else {

          if (Float.compare(value, -1.0f) <= 0) {
            motions.get(i).oldval = value;
            Integer PadKey = Keymap.get(motions.get(i).id | 0x8000 | 0x80000000);
            return setKeymap(motions.get(i).id | 0x8000 | 0x80000000);
          }
          if (Float.compare(value, 1.0f) >= 0) {
            motions.get(i).oldval = value;
            Integer PadKey = Keymap.get(motions.get(i).id | 0x80000000);
            return setKeymap(motions.get(i).id | 0x80000000);
          }
        }
      }
    }
    return false;
  }

}
