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
import android.hardware.input.InputManager
import android.os.Bundle
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.DialogFragment
import androidx.preference.PreferenceManager
import java.util.ArrayList
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.PadManager.Companion.padManager
import org.uoyabause.android.PadManager.Companion.updatePadManager

class InputDevice(var Inputlabel_: CharSequence?, var Inputvalue_: CharSequence?)
class SelInputDeviceFragment : DialogFragment(), InputManager.InputDeviceListener {
    interface InputDeviceListener {
        fun onDeviceUpdated(target: Int)
        fun onSelected(target: Int, name: String?, id: String?)
        fun onCancel(target: Int)
    }

    var mInputManager: InputManager? = null
    var listener_: InputDeviceListener? = null
    private var padm: PadManager? = null
    var InputDevices: MutableList<InputDevice> = ArrayList()
    var target_ = PLAYER1
    fun setTarget(target: Int) {
        target_ = target
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        mInputManager = context.getSystemService(Context.INPUT_SERVICE) as InputManager
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    fun setListener(l: InputDeviceListener?) {
        listener_ = l
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialogBuilder = AlertDialog.Builder(
            requireActivity())
        val res = resources
        updatePadManager()
        padm = padManager
        padm!!.loadSettings()
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(
            requireActivity())
        val selInputdevice: String?
        selInputdevice = when (target_) {
            PLAYER1 -> sharedPref.getString("pref_player1_inputdevice", "65535")
            PLAYER2 -> sharedPref.getString("pref_player2_inputdevice", "65535")
            else -> sharedPref.getString("pref_player1_inputdevice", "65535")
        }
        InputDevices.clear()
        val items: Array<String?>
        var insert_index = 0
        if (target_ == PLAYER1) {
            val pad = InputDevice(res.getString(R.string.onscreen_pad), "-1")
            InputDevices.add(pad)
            items = arrayOfNulls(padm!!.getDeviceCount() + 1)
            items[insert_index] = res.getString(R.string.onscreen_pad)
            insert_index++
        } else {
            val pad = InputDevice("Disconnect", "-1")
            InputDevices.add(pad)
            items = arrayOfNulls(padm!!.getDeviceCount() + 1)
            items[insert_index] = "Disconnect"
            insert_index++
        }
        for (inputType in 0 until padm!!.getDeviceCount()) {
            val tmp = InputDevice(padm!!.getName(inputType), padm!!.getId(inputType))
            InputDevices.add(tmp)
            items[insert_index] = padm!!.getName(inputType)
            insert_index++
        }
        var selid = 0
        for (i in InputDevices.indices) {
            if (InputDevices[i].Inputvalue_ == selInputdevice) {
                selid = i
            }
        }
        dialogBuilder.setTitle("Select Input Device")
        dialogBuilder.setSingleChoiceItems(items, selid, null)
        dialogBuilder.setNegativeButton("Cancel") { _, _ ->
            if (listener_ != null) {
                listener_!!.onCancel(target_)
            }
        }
        dialogBuilder.setPositiveButton(R.string.ok) { dialog, _ ->
            dialog.dismiss()
            val selectedPosition = (dialog as AlertDialog).listView.checkedItemPosition
            val lsharedPref = PreferenceManager.getDefaultSharedPreferences(
                requireActivity())
            val editor = lsharedPref.edit()
            val edit_target: String
            edit_target = when (target_) {
                PLAYER1 -> "pref_player1_inputdevice"
                PLAYER2 -> "pref_player2_inputdevice"
                else -> "pref_player1_inputdevice"
            }
            editor.putString(edit_target, InputDevices[selectedPosition].Inputvalue_ as String?)
            editor.apply()
            if (listener_ != null) {
                listener_!!.onSelected(target_,
                    InputDevices[selectedPosition].Inputlabel_ as String?,
                    InputDevices[selectedPosition].Inputvalue_ as String?)
            }
        }
        return dialogBuilder.create()
    }

    override fun onResume() {
        super.onResume()
        mInputManager!!.registerInputDeviceListener(this, null)
    }

    override fun onPause() {
        super.onPause()
        mInputManager!!.unregisterInputDeviceListener(this)
    }

    override fun onInputDeviceAdded(i: Int) {
        // PadManager.updatePadManager();
        if (listener_ != null) listener_!!.onDeviceUpdated(target_)
        dismiss()
    }

    override fun onInputDeviceRemoved(i: Int) {
        // PadManager.updatePadManager();
        if (listener_ != null) listener_!!.onDeviceUpdated(target_)
        dismiss()
    }

    override fun onInputDeviceChanged(i: Int) {
        // PadManager.updatePadManager();
        if (listener_ != null) listener_!!.onDeviceUpdated(target_)
        dismiss()
    }

    companion object {
        const val PLAYER1 = 0
        const val PLAYER2 = 1
    }
}
