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

import android.os.Build
import android.view.KeyEvent
import android.view.MotionEvent

internal abstract class PadManager {
  var analogMode = MODE_HAT
  var analogMode2 = MODE_HAT
  abstract fun hasPad(): Boolean
  abstract fun onKeyDown(keyCode: Int, event: KeyEvent): Int
  abstract fun onKeyUp(keyCode: Int, event: KeyEvent): Int
  abstract fun onGenericMotionEvent(event: MotionEvent): Int
  abstract fun getName(index: Int): String?
  abstract fun getId(index: Int): String?
  abstract fun setPlayer1InputDevice(id: String?)
  abstract fun getPlayer1InputDevice(): Int
  abstract fun setPlayer2InputDevice(id: String?)
  abstract fun getPlayer2InputDevice(): Int
  abstract fun loadSettings()
  abstract fun getStatusString(): String?
  abstract fun setTestMode(mode: Boolean)
  abstract fun getDeviceList(): String?
  abstract fun getDeviceCount(): Int

  interface ShowMenuListener {
    fun show()
  }

  fun showMenu() {
    if (showMenulistener != null) {
      showMenulistener!!.show()
    }
  }

  var showMenulistener: ShowMenuListener? = null
    set(value) { field = value }
  /*
  fun setShowMenulistener(listener: ShowMenuListener?) {
    showMenulistener = listener
  }
  */

  companion object {
    var NO_ACTION_MAPPED = 0
    var ACTION_MAPPED = 1
    var TOGGLE_MENU = 2
    private var _instance: PadManager? = null
    const val invalid_device_id = 65535
    const val MODE_HAT = 0
    const val MODE_ANALOG = 1
    @JvmStatic
    val padManager: PadManager?
      get() {
        if (_instance == null) {
            _instance = PadManagerV16()
        }
        return _instance
      }

    @JvmStatic
    fun updatePadManager(): PadManager? {
      _instance = PadManagerV16()
      return _instance
    }
  }
}