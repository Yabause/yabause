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

import android.util.Log
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.io.InputStream
import java.util.ArrayList
import java.util.HashMap
import org.json.JSONException
import org.json.JSONObject
import org.uoyabause.android.PadManager.Companion.ACTION_MAPPED
import org.uoyabause.android.PadManager.Companion.NO_ACTION_MAPPED
import org.uoyabause.android.PadManager.Companion.TOGGLE_MENU
import org.uoyabause.android.YabauseStorage.Companion.storage

// class InputInfo{
// 	public float _oldRightTrigger = 0.0f;
// 	public float _oldLeftTrigger = 0.0f;
// 	public int _selected_device_id = -1;
// }
internal open class BasicInputDevice(pdm: PadManagerV16) {
  var _selected_device_id = -1
  var _playerindex = 0
  var _testmode = false
  var currentButtonState = 0
  val showMenuCode = 0x1c40
  var Keymap: HashMap<Int, Int>
  var _pdm: PadManagerV16
  var isLTriggerAnalog = true
  var isRTriggerAnalog = true

  fun loadDefault() {
    Keymap.clear()
    Keymap[MotionEvent.AXIS_HAT_Y or 0x8000] = PadEvent.BUTTON_UP
    Keymap[MotionEvent.AXIS_HAT_Y] = PadEvent.BUTTON_DOWN
    Keymap[MotionEvent.AXIS_HAT_X or 0x8000] = PadEvent.BUTTON_LEFT
    Keymap[MotionEvent.AXIS_HAT_X] = PadEvent.BUTTON_RIGHT
    Keymap[MotionEvent.AXIS_LTRIGGER] = PadEvent.BUTTON_LEFT_TRIGGER
    Keymap[MotionEvent.AXIS_RTRIGGER] = PadEvent.BUTTON_RIGHT_TRIGGER
    Keymap[KeyEvent.KEYCODE_BUTTON_START] = PadEvent.BUTTON_START
    Keymap[KeyEvent.KEYCODE_BUTTON_A] = PadEvent.BUTTON_A
    Keymap[KeyEvent.KEYCODE_BUTTON_B] = PadEvent.BUTTON_B
    Keymap[KeyEvent.KEYCODE_BUTTON_R1] = PadEvent.BUTTON_C
    Keymap[KeyEvent.KEYCODE_BUTTON_X] = PadEvent.BUTTON_X
    Keymap[KeyEvent.KEYCODE_BUTTON_Y] = PadEvent.BUTTON_Y
    Keymap[KeyEvent.KEYCODE_BUTTON_L1] = PadEvent.BUTTON_Z
    Keymap[MotionEvent.AXIS_X] = PadEvent.PERANALOG_AXIS_X
    Keymap[MotionEvent.AXIS_Y] = PadEvent.PERANALOG_AXIS_Y
    Keymap[MotionEvent.AXIS_LTRIGGER] = PadEvent.PERANALOG_AXIS_LTRIGGER
    Keymap[MotionEvent.AXIS_RTRIGGER] = PadEvent.PERANALOG_AXIS_RTRIGGER
    Keymap[KeyEvent.KEYCODE_BUTTON_SELECT] = PadEvent.MENU
    isRTriggerAnalog = true
    isRTriggerAnalog = true
  }

  fun loadSettings(setting_filename: String) {
    try {
      val yabroot = File(storage.rootPath)
      if (!yabroot.exists()) yabroot.mkdir()
      val inputStream: InputStream = FileInputStream(storage.rootPath + setting_filename)
      val size = inputStream.available()
      val buffer = ByteArray(size)
      inputStream.read(buffer)
      inputStream.close()
      val json = String(buffer)

      Log.d("yabause", "keymap: $json")

      val jsonObject = JSONObject(json)
      Keymap.clear()
      Keymap[jsonObject.getInt("BUTTON_UP")] = PadEvent.BUTTON_UP
      Keymap[jsonObject.getInt("BUTTON_DOWN")] = PadEvent.BUTTON_DOWN
      Keymap[jsonObject.getInt("BUTTON_LEFT")] = PadEvent.BUTTON_LEFT
      Keymap[jsonObject.getInt("BUTTON_RIGHT")] = PadEvent.BUTTON_RIGHT
      Keymap[jsonObject.getInt("BUTTON_LEFT_TRIGGER")] = PadEvent.BUTTON_LEFT_TRIGGER
      Keymap[jsonObject.getInt("BUTTON_RIGHT_TRIGGER")] = PadEvent.BUTTON_RIGHT_TRIGGER
      Keymap[jsonObject.getInt("BUTTON_START")] = PadEvent.BUTTON_START
      Keymap[jsonObject.getInt("BUTTON_A")] = PadEvent.BUTTON_A
      Keymap[jsonObject.getInt("BUTTON_B")] = PadEvent.BUTTON_B
      Keymap[jsonObject.getInt("BUTTON_C")] = PadEvent.BUTTON_C
      Keymap[jsonObject.getInt("BUTTON_X")] = PadEvent.BUTTON_X
      Keymap[jsonObject.getInt("BUTTON_Y")] = PadEvent.BUTTON_Y
      Keymap[jsonObject.getInt("BUTTON_Z")] = PadEvent.BUTTON_Z
      Keymap[jsonObject.getInt("PERANALOG_AXIS_X")] = PadEvent.PERANALOG_AXIS_X
      Keymap[jsonObject.getInt("PERANALOG_AXIS_Y")] = PadEvent.PERANALOG_AXIS_Y
      Keymap[jsonObject.getInt("PERANALOG_AXIS_LTRIGGER")] = PadEvent.PERANALOG_AXIS_LTRIGGER
      Keymap[jsonObject.getInt("PERANALOG_AXIS_RTRIGGER")] = PadEvent.PERANALOG_AXIS_RTRIGGER
      Keymap[jsonObject.getInt("MENU")] = PadEvent.MENU

      try {
        isLTriggerAnalog = jsonObject.getBoolean("IS_LTRIGGER_ANALOG")
      } catch (e: JSONException) {
        isLTriggerAnalog = true
      }

      try {
        isRTriggerAnalog = jsonObject.getBoolean("IS_RTRIGGER_ANALOG")
      } catch (e: JSONException) {
        isRTriggerAnalog = true
      }
    } catch (e: IOException) {
      e.printStackTrace()
      loadDefault()
    } catch (e: JSONException) {
      e.printStackTrace()
      loadDefault()
    }
  }

  // Limitaions
  // SS Controller adapter ... Left trigger and Right Trigger is not recognizeed as analog button. you need to skip them
  // Moga 000353 ... OK
  // SMACON ... Left trigger and Right Trigger is not recognizeed as analog button. you need to skip them
  // ipega ... OK, but too sensitive.
  open fun onGenericMotionEvent(event: MotionEvent): Int {
    var rtn = 0
    if (event.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) {
      val motionEvent = event
      for ((btn, sat_btn) in Keymap) {
        // System.out.println(e.getKey() + " : " + e.getValue());

        // AnalogDevices
        if (_pdm.analogMode == PadManager.MODE_ANALOG && _playerindex == 0 ||
          _pdm.analogMode2 == PadManager.MODE_ANALOG && _playerindex == 1
        ) {
          val motion_value = motionEvent.getAxisValue(btn and 0x00007FFF)
          if (sat_btn == PadEvent.PERANALOG_AXIS_X || sat_btn == PadEvent.PERANALOG_AXIS_Y) {
            var nomalize_value: Double = motion_value * 128.0 + 128.0
            if (nomalize_value > 255.0) nomalize_value = 255.0
            if (nomalize_value < 0.0) nomalize_value = 0.0
            Log.d("Yabause", " $sat_btn: $motion_value/$nomalize_value")
            YabauseRunnable.axis(sat_btn, _playerindex, nomalize_value.toInt())
            continue
          } else if (sat_btn == PadEvent.PERANALOG_AXIS_LTRIGGER && isLTriggerAnalog) {
            var nomalize_value: Double = motion_value * 255.0
            if (nomalize_value > 255.0) nomalize_value = 255.0
            if (nomalize_value < 0.0) nomalize_value = 0.0
            YabauseRunnable.axis(sat_btn, _playerindex, nomalize_value.toInt())
            if (nomalize_value > 145.0) {
              YabauseRunnable.press(PadEvent.BUTTON_LEFT_TRIGGER, _playerindex)
            } else {
              YabauseRunnable.release(PadEvent.BUTTON_LEFT_TRIGGER, _playerindex)
            }
            continue
          } else if (sat_btn == PadEvent.PERANALOG_AXIS_RTRIGGER && isRTriggerAnalog) {
            var nomalize_value: Double = motion_value * 255.0
            if (nomalize_value > 255.0) nomalize_value = 255.0
            if (nomalize_value < 0.0) nomalize_value = 0.0
            YabauseRunnable.axis(sat_btn, _playerindex, nomalize_value.toInt())
            if (nomalize_value > 145.0) {
              YabauseRunnable.press(PadEvent.BUTTON_RIGHT_TRIGGER, _playerindex)
            } else {
              YabauseRunnable.release(PadEvent.BUTTON_RIGHT_TRIGGER, _playerindex)
            }
            continue
          }
        } else {
          if (sat_btn == PadEvent.PERANALOG_AXIS_X || sat_btn == PadEvent.PERANALOG_AXIS_Y || sat_btn == PadEvent.PERANALOG_AXIS_LTRIGGER || sat_btn == PadEvent.PERANALOG_AXIS_RTRIGGER) {
            continue
          }
        }
        if (btn and -0x80000000 != 0) {
          val motion_value = motionEvent.getAxisValue(btn and 0x00007FFF)
          if (btn and 0x8000 != 0) { // Dir
            if (java.lang.Float.compare(motion_value, -0.8f) < 0) { // ON
              if (_testmode) _pdm.addDebugString("onGenericMotionEvent: On  $btn Satpad: $sat_btn") else YabauseRunnable.press(
                sat_btn, _playerindex
              )
              rtn = 1
            } else if (java.lang.Float.compare(motion_value, -0.5f) > 0) { // OFF
              if (_testmode) _pdm.addDebugString("onGenericMotionEvent: Off  $btn Satpad: $sat_btn") else YabauseRunnable.release(
                sat_btn, _playerindex
              )
              rtn = 1
            }
          } else {
            if (java.lang.Float.compare(motion_value, 0.8f) > 0) { // ON
              if (_testmode) _pdm.addDebugString("onGenericMotionEvent: On  $btn Satpad: $sat_btn") else YabauseRunnable.press(
                sat_btn, _playerindex
              )
              rtn = 1
            } else if (java.lang.Float.compare(motion_value, 0.5f) < 0) { // OFF
              if (_testmode) _pdm.addDebugString("onGenericMotionEvent: Off  $btn Satpad: $sat_btn") else YabauseRunnable.release(
                sat_btn, _playerindex
              )
              rtn = 1
            }
          }
        }

        // AnalogDvice
      }
    }
    return rtn
  }

  open fun onKeyDown(keyCode: Int, event: KeyEvent): Int {
    var lkeyCode = keyCode
    if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
      event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
    ) {
      if (lkeyCode == KeyEvent.KEYCODE_BACK) {
        return PadManager.NO_ACTION_MAPPED
      }
      if (lkeyCode == 0) {
        lkeyCode = event.scanCode
      }
      var PadKey = Keymap[lkeyCode]
      return if (PadKey != null) {
        currentButtonState = currentButtonState or (1 shl PadKey)
        if (showMenuCode == currentButtonState) {
          for (i in 0..31) {
            if (currentButtonState and (0x1 shl i) != 0) {
              YabauseRunnable.release(i, _playerindex)
            }
          }
          currentButtonState = 0 // clear
          _pdm.showMenu()
          return PadManager.ACTION_MAPPED
        }

        // Log.d(this.getClass().getSimpleName(),"currentButtonState = " + Integer.toHexString(currentButtonState) );
        event.startTracking()
        if (_testmode) {
          _pdm.addDebugString("onKeyDown: $lkeyCode Satpad: $PadKey")
        } else {

          if (((_pdm.analogMode == PadManager.MODE_ANALOG && _playerindex == 0) ||
              (_pdm.analogMode2 == PadManager.MODE_ANALOG && _playerindex == 1))) {

                if (PadKey == PadEvent.PERANALOG_AXIS_LTRIGGER && isLTriggerAnalog == false) {
                  YabauseRunnable.axis(PadKey, _playerindex, 255)
                  YabauseRunnable.press(PadEvent.BUTTON_LEFT_TRIGGER, _playerindex)
                } else if (PadKey == PadEvent.PERANALOG_AXIS_RTRIGGER && isRTriggerAnalog == false) {
                  YabauseRunnable.axis(PadKey, _playerindex, 255)
                  YabauseRunnable.press(PadEvent.BUTTON_RIGHT_TRIGGER, _playerindex)
                } else if (PadKey == PadEvent.PERANALOG_AXIS_X || PadKey == PadEvent.PERANALOG_AXIS_Y) {
                } else {
                  YabauseRunnable.press(PadKey, _playerindex)
                }
          } else {
            if (PadKey == PadEvent.PERANALOG_AXIS_LTRIGGER) {
              PadKey = PadEvent.BUTTON_LEFT_TRIGGER
            } else if (PadKey == PadEvent.PERANALOG_AXIS_RTRIGGER) {
              PadKey = PadEvent.BUTTON_RIGHT_TRIGGER
            }
            YabauseRunnable.press(PadKey, _playerindex)
          }
        }
        PadManager.ACTION_MAPPED // ignore this input
      } else {
        if (_testmode) _pdm.addDebugString("onKeyDown: $lkeyCode Satpad: none")
        PadManager.NO_ACTION_MAPPED
      }
    }
    return 0
  }

  open fun onKeyUp(keyCode: Int, event: KeyEvent): Int {
    var lkeyCode = keyCode
    if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
      event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
    ) {
      if (lkeyCode == KeyEvent.KEYCODE_BACK) {
        return NO_ACTION_MAPPED
      }
      if (lkeyCode == 0) {
        lkeyCode = event.scanCode
      }
      if (!event.isCanceled) {
        var PadKey = Keymap[lkeyCode]
        return if (PadKey != null) {
          currentButtonState = currentButtonState and (1 shl PadKey).inv()
          // Log.d(this.getClass().getSimpleName(),"currentButtonState = " + Integer.toHexString(currentButtonState) );
          if (_testmode) _pdm.addDebugString("onKeyUp: $lkeyCode Satpad: $PadKey") else {
              if (PadKey == PadEvent.MENU) {
                return TOGGLE_MENU
              } else {
                if (((_pdm.analogMode == PadManager.MODE_ANALOG && _playerindex == 0) ||
                      (_pdm.analogMode2 == PadManager.MODE_ANALOG && _playerindex == 1))) {
                  if (PadKey == PadEvent.PERANALOG_AXIS_LTRIGGER && isLTriggerAnalog == false) {
                    YabauseRunnable.axis(PadKey, _playerindex, 0)
                    YabauseRunnable.release(PadEvent.BUTTON_LEFT_TRIGGER, _playerindex)
                  } else if (PadKey == PadEvent.PERANALOG_AXIS_RTRIGGER && isRTriggerAnalog == false) {
                    YabauseRunnable.axis(PadKey, _playerindex, 0)
                    YabauseRunnable.release(PadEvent.BUTTON_RIGHT_TRIGGER, _playerindex)
                  } else if (PadKey == PadEvent.PERANALOG_AXIS_X || PadKey == PadEvent.PERANALOG_AXIS_Y) {
                  } else {
                    YabauseRunnable.release(PadKey, _playerindex)
                  }
                } else {
                  if (PadKey == PadEvent.PERANALOG_AXIS_LTRIGGER) {
                    PadKey = PadEvent.BUTTON_LEFT_TRIGGER
                  } else if (PadKey == PadEvent.PERANALOG_AXIS_RTRIGGER) {
                    PadKey = PadEvent.BUTTON_RIGHT_TRIGGER
                  }
                  YabauseRunnable.release(PadKey, _playerindex)
                }
              }
          }
          ACTION_MAPPED // ignore this input
        } else {
          if (_testmode) _pdm.addDebugString("onKeyUp: $lkeyCode Satpad: none")
          NO_ACTION_MAPPED
        }
      }
    }
    return NO_ACTION_MAPPED
  }

  init {
    Keymap = HashMap()
    _pdm = pdm
  }
}

internal class SSController(pdm: PadManagerV16) : BasicInputDevice(pdm) {
  override fun onGenericMotionEvent(event: MotionEvent): Int {
    return super.onGenericMotionEvent(event)
  }

  override fun onKeyDown(keyCode: Int, event: KeyEvent): Int {
    if (event.scanCode == 0) {
      return ACTION_MAPPED
    }
    if (keyCode == KeyEvent.KEYCODE_BACK) {
      val PadKey = Keymap[keyCode]
      return if (PadKey != null) {
        YabauseRunnable.press(PadKey, _playerindex)
        ACTION_MAPPED
      } else {
        NO_ACTION_MAPPED
      }
    }
    return super.onKeyDown(keyCode, event)
  }

  override fun onKeyUp(keyCode: Int, event: KeyEvent): Int {
    if (event.scanCode == 0) {
      return ACTION_MAPPED
    }
    if (keyCode == KeyEvent.KEYCODE_BACK) {
      val PadKey = Keymap[keyCode]
      return if (PadKey != null) {
        YabauseRunnable.release(PadKey, _playerindex)
        ACTION_MAPPED
      } else {
        NO_ACTION_MAPPED
      }
    }
    return super.onKeyUp(keyCode, event)
  }
}

internal class PadManagerV16 : PadManager() {
  private val deviceIds: HashMap<String, Int?>
  val TAG = "PadManagerV16"
  var DebugMesage = String()
  var _testmode = false
  var current_msg_index = 0
  val max_msg_index = 24
  var DebugMesageArray: Array<String?>
  val player_count = 2
  var Keymap: List<HashMap<Int, Int>>
  var pads: Array<BasicInputDevice?>
  override fun loadSettings() {
    if (pads[0] != null) {
      pads[0]!!.loadSettings("keymap_v2.json")
    }
    if (pads[1] != null) {
      pads[1]!!.loadSettings("keymap_player2_v2.json")
    }
  }

  override fun setTestMode(mode: Boolean) {
    _testmode = mode
    if (pads[0] != null) pads[0]!!._testmode = _testmode
    if (pads[1] != null) pads[1]!!._testmode = _testmode
  }

  fun addDebugString(msg: String?) {
    DebugMesageArray[current_msg_index] = msg
    current_msg_index++
    if (current_msg_index >= max_msg_index) current_msg_index = 0
  }

  override fun getStatusString(): String? {
    DebugMesage = ""
    var start = current_msg_index
    for (i in 0 until max_msg_index) {
      if (DebugMesageArray[start] != "") {
        DebugMesage = """
                ${DebugMesageArray[start]}
                $DebugMesage
                """.trimIndent()
        start--
        if (start < 0) {
          start = max_msg_index - 1
        }
      }
    }
    return DebugMesage
  }

  override fun hasPad(): Boolean {
    return deviceIds.size > 0
  }

  override fun getDeviceList(): String {
    return DebugMesage
  }

  override fun getDeviceCount(): Int {
    return deviceIds.size
  }

  override fun getName(index: Int): String? {
    if (index < 0 && index >= deviceIds.size) {
      return null
    }
    var counter = 0
    for (`val` in deviceIds.values) {
      if (counter == index) {
        val dev = InputDevice.getDevice(`val`!!)
        return dev?.name
      }
      counter++
    }
    return null
  }

  override fun getId(index: Int): String? {
    if (index < 0 && index >= deviceIds.size) {
      return null
    }
    var counter = 0
    for (key in deviceIds.keys) {
      if (counter == index) {
        return key
      }
      counter++
    }
    return null
  }

  override fun setPlayer1InputDevice(id: String?) {
    if (id == null) {
      pads[0] = BasicInputDevice(this)
      pads[0]!!._selected_device_id = -1
      return
    }
    val did = deviceIds[id]
    if (did == null) {
      pads[0] = BasicInputDevice(this)
      pads[0]!!._selected_device_id = -1
    } else {
      val dev = InputDevice.getDevice(did)
      if (dev.name.contains("HuiJia")) {
        pads[0] = SSController(this)
      } else {
        pads[0] = BasicInputDevice(this)
      }
      pads[0]!!._selected_device_id = did
    }
    pads[0]!!._playerindex = 0
    pads[0]!!.loadSettings("keymap_v2.json")
    pads[0]!!._testmode = _testmode
    return
  }

  override fun getPlayer1InputDevice(): Int {
    return if (pads[0] == null) -1 else pads[0]!!._selected_device_id
  }

  override fun setPlayer2InputDevice(id: String?) {
    if (id == null) {
      pads[1] = BasicInputDevice(this)
      pads[1]!!._selected_device_id = -1
      return
    }
    val did = deviceIds[id]
    if (did == null) {
      pads[1] = BasicInputDevice(this)
      pads[1]!!._selected_device_id = -1
    } else {
      val dev = InputDevice.getDevice(did)
      if (dev.name.contains("HuiJia")) {
        pads[1] = SSController(this)
      } else {
        pads[1] = BasicInputDevice(this)
      }
      pads[1]!!._selected_device_id = did
    }
    pads[1]!!._playerindex = 1
    pads[1]!!.loadSettings("keymap_player2_v2.json")
    pads[1]!!._testmode = _testmode
    return
  }

  override fun getPlayer2InputDevice(): Int {
    return if (pads[1] == null) -1 else pads[1]!!._selected_device_id
  }

  fun findPlayerPad(deviceid: Int): BasicInputDevice? {
    for (i in 0 until player_count) {
      if (pads[i] != null && deviceid == pads[i]!!._selected_device_id) {
        return pads[i]
      }
    }
    return null
  }

  override fun onGenericMotionEvent(event: MotionEvent): Int {
    if (pads[0] != null && pads[0]!!._selected_device_id == event.deviceId) {
      pads[0]!!.onGenericMotionEvent(event)
    }
    if (pads[1] != null && pads[1]!!._selected_device_id == event.deviceId) {
      pads[1]!!.onGenericMotionEvent(event)
    }
    return NO_ACTION_MAPPED
  }

  override fun onKeyDown(keyCode: Int, event: KeyEvent): Int {
    var rtn = NO_ACTION_MAPPED
    if (pads[0] != null && pads[0]!!._selected_device_id == event.deviceId) {
      rtn = rtn or pads[0]!!.onKeyDown(keyCode, event)
    }
    if (pads[1] != null && pads[1]!!._selected_device_id == event.deviceId) {
      rtn = rtn or pads[1]!!.onKeyDown(keyCode, event)
    }
    return rtn
  }

  override fun onKeyUp(keyCode: Int, event: KeyEvent): Int {
    var rtn = NO_ACTION_MAPPED
    if (pads[0] != null && pads[0]!!._selected_device_id == event.deviceId) {
      rtn = rtn or pads[0]!!.onKeyUp(keyCode, event)
    }
    if (pads[1] != null && pads[1]!!._selected_device_id == event.deviceId) {
      rtn = rtn or pads[1]!!.onKeyUp(keyCode, event)
    }
    return rtn
  }

  init {
    deviceIds = HashMap()
    pads = arrayOfNulls(player_count)
    Keymap = ArrayList()
    for (i in 0 until player_count) {
      pads[i] = null
      // pads[i] = new BasicInputDevice(this);
      // pads[i]._selected_device_id = invalid_device_id;
    }
    val ids = InputDevice.getDeviceIds()
    for (deviceId in ids) {
      val dev = InputDevice.getDevice(deviceId)
      val sources = dev.sources
      if ((sources and InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
        (sources and InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
      ) {
        if (deviceIds[dev.descriptor] == null) {

          // Avoid crazy devices
          if (dev.name == "msm8974-taiko-mtp-snd-card Button Jack") {
            continue
          }

          if (dev.name == "uinput-fpc") {
            continue
          }

          deviceIds[dev.descriptor] = deviceId
        }
      }
      val isGamePad = sources and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD
      val isGameJoyStick = sources and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
      DebugMesage += "Input Device:${dev.name} ID:${dev.descriptor} Product ID:${dev.productId} isGamePad?:$isGamePad isJoyStick?:$isGameJoyStick"
    }

    // Setting moe
    current_msg_index = 0
    DebugMesageArray = arrayOfNulls(max_msg_index)
    for (i in 0 until max_msg_index) {
      DebugMesageArray[i] = ""
    }
    _testmode = false
  }
}
