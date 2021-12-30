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

import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.os.Bundle
import android.os.Handler
import android.util.Log
import android.util.TypedValue
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.View.OnGenericMotionListener
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.DialogFragment
import org.devmiyax.yabasanshiro.R
import org.json.JSONException
import org.json.JSONObject
import org.uoyabause.android.PadManager.Companion.padManager
import org.uoyabause.android.YabauseStorage.Companion.storage
import java.io.*
import java.util.*

class InputSettingPresenter : DialogInterface.OnKeyListener,
OnGenericMotionListener, View.OnClickListener {
  private var key_message: TextView? = null
  private val skip: Button? = null
  private var Keymap: HashMap<Int, Int>? = null
  private var map: ArrayList<Int>? = null
  private var index = 0
  private var pad_m: PadManager? = null
  var context_m: Context? = null
  private var _selected_device_id = 0
  private var save_filename = "keymap"
  private var playerid = 0
  private var isLTriggerAnalog = true
  private var isRTriggerAnalog = true
  lateinit var parentDialog:Dialog

  var listener_: InputSettingListener? = null
  fun setListener(listener: InputSettingListener?) {
    listener_ = listener
  }

  inner class MotionMap(id: Int) {
    var id = 0
    var oldval = 0.0f

    init {
      this.id = id
    }
  }

  var motions: ArrayList<MotionMap>? = null
  fun setPlayerAndFilename(playerid: Int, fname: String) {
    this.playerid = playerid
    save_filename = fname
  }

  fun InitObjects(context: Context?) {
    index = 0
    context_m = context
    Keymap = HashMap()
    map = ArrayList()
    map!!.add(PadEvent.BUTTON_UP)
    map!!.add(PadEvent.BUTTON_DOWN)
    map!!.add(PadEvent.BUTTON_LEFT)
    map!!.add(PadEvent.BUTTON_RIGHT)
    map!!.add(PadEvent.BUTTON_LEFT_TRIGGER)
    map!!.add(PadEvent.BUTTON_RIGHT_TRIGGER)
    map!!.add(PadEvent.BUTTON_START)
    map!!.add(PadEvent.BUTTON_A)
    map!!.add(PadEvent.BUTTON_B)
    map!!.add(PadEvent.BUTTON_C)
    map!!.add(PadEvent.BUTTON_X)
    map!!.add(PadEvent.BUTTON_Y)
    map!!.add(PadEvent.BUTTON_Z)
    map!!.add(PadEvent.PERANALOG_AXIS_X) // left to right
    map!!.add(PadEvent.PERANALOG_AXIS_Y) // up to down
    map!!.add(PadEvent.PERANALOG_AXIS_LTRIGGER) // left trigger
    map!!.add(PadEvent.PERANALOG_AXIS_RTRIGGER) // right trigger
    map!!.add(PadEvent.MENU) // right trigger

    //setDialogTitle(R.string.input_the_key);
    //setPositiveButtonText(null);  // OKボタンを非表示にする
    //setDialogLayoutResource(R.layout.keymap);
    motions = ArrayList()
    motions!!.add(MotionMap(MotionEvent.AXIS_X))
    motions!!.add(MotionMap(MotionEvent.AXIS_Y))
    motions!!.add(MotionMap(MotionEvent.AXIS_PRESSURE))
    motions!!.add(MotionMap(MotionEvent.AXIS_SIZE))
    motions!!.add(MotionMap(MotionEvent.AXIS_TOUCH_MAJOR))
    motions!!.add(MotionMap(MotionEvent.AXIS_TOUCH_MINOR))
    motions!!.add(MotionMap(MotionEvent.AXIS_TOOL_MAJOR))
    motions!!.add(MotionMap(MotionEvent.AXIS_TOOL_MINOR))
    motions!!.add(MotionMap(MotionEvent.AXIS_ORIENTATION))
    motions!!.add(MotionMap(MotionEvent.AXIS_VSCROLL))
    motions!!.add(MotionMap(MotionEvent.AXIS_HSCROLL))
    motions!!.add(MotionMap(MotionEvent.AXIS_Z))
    motions!!.add(MotionMap(MotionEvent.AXIS_RX))
    motions!!.add(MotionMap(MotionEvent.AXIS_RY))
    motions!!.add(MotionMap(MotionEvent.AXIS_RZ))
    motions!!.add(MotionMap(MotionEvent.AXIS_HAT_X))
    motions!!.add(MotionMap(MotionEvent.AXIS_HAT_Y))
    motions!!.add(MotionMap(MotionEvent.AXIS_LTRIGGER))
    motions!!.add(MotionMap(MotionEvent.AXIS_RTRIGGER))
    motions!!.add(MotionMap(MotionEvent.AXIS_THROTTLE))
    motions!!.add(MotionMap(MotionEvent.AXIS_RUDDER))
    motions!!.add(MotionMap(MotionEvent.AXIS_WHEEL))
    motions!!.add(MotionMap(MotionEvent.AXIS_GAS))
    motions!!.add(MotionMap(MotionEvent.AXIS_BRAKE))
    motions!!.add(MotionMap(MotionEvent.AXIS_DISTANCE))
    motions!!.add(MotionMap(MotionEvent.AXIS_TILT))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_1))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_2))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_3))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_4))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_5))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_6))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_7))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_8))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_9))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_10))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_11))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_12))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_13))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_14))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_15))
    motions!!.add(MotionMap(MotionEvent.AXIS_GENERIC_16))
  }

  fun onCreateDialog( context: Context, dlg:Dialog, view:View) {
    InitObjects(context)
    val res = context.resources
    dlg.setOnKeyListener(this)
    pad_m = padManager
    if (pad_m!!.hasPad() == false) {
      Toast.makeText(context_m, R.string.joystick_is_not_connected, Toast.LENGTH_LONG).show()
      listener_?.onCancel()
      dlg.dismiss()
    }
    if (playerid == 1) {
      _selected_device_id = pad_m!!.getPlayer2InputDevice()
    } else {
      _selected_device_id = pad_m!!.getPlayer1InputDevice()
    }

    key_message = view.findViewById<View>(R.id.text_key) as TextView
    key_message!!.setOnGenericMotionListener(this)
    key_message!!.text = res.getString(R.string.up)
    key_message!!.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 32f)
    key_message!!.isClickable = false
    key_message!!.setOnGenericMotionListener(this)
    key_message!!.isFocusableInTouchMode = true
    key_message!!.requestFocus()
    val button: View = view.findViewById<View>(R.id.button_skip) as Button
    button.setOnClickListener(this)

    parentDialog = dlg
  }

  fun saveSettings() {
    val jsonObject = JSONObject()
    try {
      // Inisiazlie
      val dmykey = 65535
      jsonObject.put("BUTTON_UP", dmykey)
      jsonObject.put("BUTTON_DOWN", dmykey)
      jsonObject.put("BUTTON_LEFT", dmykey)
      jsonObject.put("BUTTON_RIGHT", dmykey)
      jsonObject.put("BUTTON_LEFT_TRIGGER", dmykey)
      jsonObject.put("BUTTON_RIGHT_TRIGGER", dmykey)
      jsonObject.put("BUTTON_START", dmykey)
      jsonObject.put("BUTTON_A", dmykey)
      jsonObject.put("BUTTON_B", dmykey)
      jsonObject.put("BUTTON_C", dmykey)
      jsonObject.put("BUTTON_X", dmykey)
      jsonObject.put("BUTTON_Y", dmykey)
      jsonObject.put("BUTTON_Z", dmykey)
      jsonObject.put("PERANALOG_AXIS_X", dmykey)
      jsonObject.put("PERANALOG_AXIS_Y", dmykey)
      jsonObject.put("PERANALOG_AXIS_LTRIGGER", dmykey)
      jsonObject.put("PERANALOG_AXIS_RTRIGGER", dmykey)
      jsonObject.put("MENU", dmykey)
      jsonObject.put("IS_LTRIGGER_ANALOG", isLTriggerAnalog)
      jsonObject.put("IS_RTRIGGER_ANALOG", isRTriggerAnalog)
      for (entry: Map.Entry<Int, Int> in Keymap!!.entries) {
        when (entry.value) {
          PadEvent.BUTTON_UP -> jsonObject.put("BUTTON_UP", entry.key)
          PadEvent.BUTTON_DOWN -> jsonObject.put("BUTTON_DOWN", entry.key)
          PadEvent.BUTTON_LEFT -> jsonObject.put("BUTTON_LEFT", entry.key)
          PadEvent.BUTTON_RIGHT -> jsonObject.put("BUTTON_RIGHT", entry.key)
          PadEvent.BUTTON_LEFT_TRIGGER -> jsonObject.put("BUTTON_LEFT_TRIGGER", entry.key)
          PadEvent.BUTTON_RIGHT_TRIGGER -> jsonObject.put("BUTTON_RIGHT_TRIGGER", entry.key)
          PadEvent.BUTTON_START -> jsonObject.put("BUTTON_START", entry.key)
          PadEvent.BUTTON_A -> jsonObject.put("BUTTON_A", entry.key)
          PadEvent.BUTTON_B -> jsonObject.put("BUTTON_B", entry.key)
          PadEvent.BUTTON_C -> jsonObject.put("BUTTON_C", entry.key)
          PadEvent.BUTTON_X -> jsonObject.put("BUTTON_X", entry.key)
          PadEvent.BUTTON_Y -> jsonObject.put("BUTTON_Y", entry.key)
          PadEvent.BUTTON_Z -> jsonObject.put("BUTTON_Z", entry.key)
          PadEvent.PERANALOG_AXIS_X -> jsonObject.put("PERANALOG_AXIS_X", entry.key)
          PadEvent.PERANALOG_AXIS_Y -> jsonObject.put("PERANALOG_AXIS_Y", entry.key)
          PadEvent.PERANALOG_AXIS_LTRIGGER -> jsonObject.put("PERANALOG_AXIS_LTRIGGER", entry.key)
          PadEvent.PERANALOG_AXIS_RTRIGGER -> jsonObject.put("PERANALOG_AXIS_RTRIGGER", entry.key)
          PadEvent.MENU -> jsonObject.put("MENU", entry.key)
        }
      }

      // jsonファイル出力
      val file = File(storage.rootPath, "${save_filename}_v2.json")
      val filewriter: FileWriter
      filewriter = FileWriter(file)
      val bw = BufferedWriter(filewriter)
      val pw = PrintWriter(bw)
      pw.write(jsonObject.toString())
      pw.close()
    } catch (e: IOException) {
      e.printStackTrace()
    } catch (e: JSONException) {
      e.printStackTrace()
    }
  }

  var keystate_ = 0
  var h: Handler? = null
  fun setMessage(str: String?) {
    key_message!!.text = str
  }

  fun setKeymap(padkey: Int): Boolean {
    if (keystate_ != 0) return true
    keystate_ = 1
    h = Handler()
    Thread { h!!.postDelayed({ keystate_ = 0 }, 300) }.start()
    Keymap!![padkey] = map!!.get(index)
    Log.d("setKeymap", "index =" + map!![index] + ": pad = " + padkey)
    index++
    if (index >= map!!.size) {
      saveSettings()
      listener_?.onFinishInputSetting()
      parentDialog!!.dismiss()
      return true
    }
    val res = context_m!!.resources
    when (map!![index]) {
      PadEvent.BUTTON_UP -> setMessage(res.getString(R.string.up))
      PadEvent.BUTTON_DOWN -> setMessage(res.getString(R.string.down))
      PadEvent.BUTTON_LEFT -> setMessage(res.getString(R.string.left))
      PadEvent.BUTTON_RIGHT -> setMessage(res.getString(R.string.right))
      PadEvent.BUTTON_LEFT_TRIGGER -> setMessage(res.getString(R.string.l_trigger))
      PadEvent.BUTTON_RIGHT_TRIGGER -> setMessage(res.getString(R.string.r_trigger))
      PadEvent.BUTTON_START -> setMessage(res.getString(R.string.start))
      PadEvent.BUTTON_A -> setMessage(res.getString(R.string.a_button))
      PadEvent.BUTTON_B -> setMessage(res.getString(R.string.b_button))
      PadEvent.BUTTON_C -> setMessage(res.getString(R.string.c_button))
      PadEvent.BUTTON_X -> setMessage(res.getString(R.string.x_button))
      PadEvent.BUTTON_Y -> setMessage(res.getString(R.string.y_button))
      PadEvent.BUTTON_Z -> setMessage(res.getString(R.string.z_button))
      PadEvent.PERANALOG_AXIS_X -> setMessage(res.getString(R.string.axis_x))
      PadEvent.PERANALOG_AXIS_Y -> setMessage(res.getString(R.string.axis_y))
      PadEvent.PERANALOG_AXIS_LTRIGGER -> setMessage(res.getString(R.string.axis_l))
      PadEvent.PERANALOG_AXIS_RTRIGGER -> setMessage(res.getString(R.string.axis_r))
      PadEvent.MENU -> setMessage(res.getString(R.string.menu))
    }

    key_message?.requestFocus()
    return true
  }

  override fun onClick(view: View?) {
    if (view?.id == R.id.button_skip) {
      setKeymap(-1)
    }
  }

  private val KEYCODE_L2 = 104
  private val KEYCODE_R2 = 105
  var onkey = false

  override fun onKey(dialogInterface: DialogInterface?, keyCode: Int, event: KeyEvent?): Boolean {
    var keyCode = keyCode
    if (event?.deviceId != _selected_device_id) return false
    if (event?.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
      event?.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
    ) {
      val dev = InputDevice.getDevice(_selected_device_id)
      if (dev.name.contains("HuiJia")) {
        if (event.scanCode == 0) {
          return false
        }
      }
      Log.d("Yabause", "onKey: ${keyCode} ${event.action} value: ${event.action}")
      if (event.repeatCount == 0 && event.action == KeyEvent.ACTION_UP) {
        if (onkey) {
          onkey = false
          if (keyCode == 0) {
            keyCode = event.scanCode
          }
          if (map!![index] == PadEvent.PERANALOG_AXIS_LTRIGGER) {
            isLTriggerAnalog = false
          }
          if (map!![index] == PadEvent.PERANALOG_AXIS_RTRIGGER) {
            isRTriggerAnalog = false
          }

          if (map!!.get(index) == PadEvent.PERANALOG_AXIS_X || map!!.get(index) == PadEvent.PERANALOG_AXIS_Y){
            setKeymap(65535)
          }else {
            val PadKey = Keymap!![keyCode]
            setKeymap(keyCode)
          }
        }
        return true
      }
      if (event.repeatCount == 0 && event.action == KeyEvent.ACTION_DOWN) {
        onkey = true
        return true

        // Accept AnalogInput
/*
        if (map.get(index) == PadEvent.PERANALOG_AXIS_X || map.get(index) == PadEvent.PERANALOG_AXIS_Y ||
                map.get(index) == PadEvent.PERANALOG_AXIS_LTRIGGER || map.get(index) == PadEvent.PERANALOG_AXIS_RTRIGGER) {
          return true;
        }
*/
      }
    }
    return false
  }


  override fun onGenericMotion(view: View?, event: MotionEvent?): Boolean {

    //Log.d("Yabause", "onGenericMotion: ${event} ")

    if (event?.deviceId != _selected_device_id) return false
    if (onkey == false) return false
    if (event?.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {
      for (i in motions!!.indices) {
        val value = event.getAxisValue(motions!![i].id)
        if (value.toDouble() != 0.0) {
          Log.d("Yabause", "onGenericMotion:" + motions!![i].id + " value:" + value)
        }
        val dev = InputDevice.getDevice(_selected_device_id)
        if (dev.name.contains("Moga") && motions!![i].id == 32) {
          continue
        }
        if (map!![index] == PadEvent.PERANALOG_AXIS_X ||
          map!![index] == PadEvent.PERANALOG_AXIS_Y ||
          map!![index] == PadEvent.PERANALOG_AXIS_LTRIGGER ||
          map!![index] == PadEvent.PERANALOG_AXIS_RTRIGGER) {
          if ( value <= -0.9f  || value >= 0.9f) {
            motions!![i].oldval = value
            if (map!![index] == PadEvent.PERANALOG_AXIS_LTRIGGER) {
              isLTriggerAnalog = true
            }
            if (map!![index] == PadEvent.PERANALOG_AXIS_RTRIGGER) {
              isRTriggerAnalog = true
            }
            onkey = false
            val PadKey = Keymap!!.get(motions!!.get(i).id)
            setKeymap( motions!!.get(i).id or -0x80000000 )
            return true
          }
        } else {
/*
          if (java.lang.Float.compare(value, -1.0f) <= 0) {
            motions!![i].oldval = value
            onkey = false
            val PadKey = Keymap!![motions!![i].id or 0x8000 or -0x80000000]
            return setKeymap(motions!![i].id or 0x8000 or -0x80000000)
          }
          if (java.lang.Float.compare(value, 1.0f) >= 0) {
            onkey = false
            motions!![i].oldval = value
            val PadKey = Keymap!![motions!![i].id or -0x80000000]
            return setKeymap(motions!![i].id or -0x80000000)
          }
 */
        }
      }
    }
    return false
  }

}


interface InputSettingListener {
  fun onFinishInputSetting()
  fun onCancel()
}

class InputSettingFragment() : DialogFragment(){

  val presenter : InputSettingPresenter =  InputSettingPresenter()

  var listener_: InputSettingListener? = null
  fun setListener(listener: InputSettingListener?) {
    listener_ = listener
    presenter.setListener(listener_)
  }

  fun setPlayerAndFilename(playerid: Int, fname: String) {
    presenter.setPlayerAndFilename(playerid,fname)
  }

  override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
    val dialogBuilder = AlertDialog.Builder(requireActivity())
    val view = layoutInflater.inflate(R.layout.keymap, null, false)
    dialogBuilder.setView(view)
    dialogBuilder.setNegativeButton("Cancel") { dialog, whichButton ->
      listener_?.onCancel()
    }
    val dlg: Dialog = dialogBuilder.create()
    presenter.onCreateDialog(requireActivity(),dlg,view)
    return dlg
  }

}