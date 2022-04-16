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
package org.uoyabause.android.cheat

import android.app.AlertDialog
import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.inputmethod.EditorInfo
import android.widget.Button
import android.widget.EditText
import android.widget.TextView.OnEditorActionListener
import androidx.fragment.app.DialogFragment
import org.devmiyax.yabasanshiro.R

class LocalCheatEditDialog : DialogFragment() {
    var mContent: View? = null
    var target_: CheatItem? = null
    fun setEditTarget(cheat: CheatItem?) {
        target_ = cheat
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val builder = AlertDialog.Builder(requireActivity())
        val inflater =
            requireActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
        mContent = inflater.inflate(R.layout.edit_cheat, null)
        if (mContent == null) {
            return builder.create()
        }

        builder.setView(mContent)
        val desc = mContent!!.findViewById<View>(R.id.editText_cheat_desc) as EditText
        val code = mContent!!.findViewById<View>(R.id.editText_code) as EditText
        if (target_ != null) {
            desc.setText(target_!!.description)
            code.setText(target_!!.cheat_code)
        }

        // Apply Button
        val button_apply = mContent!!.findViewById<View>(R.id.button_cheat_edit_apply) as Button
        button_apply.setOnClickListener {
            val cheatDesc = mContent!!.findViewById<View>(R.id.editText_cheat_desc) as EditText
            val cheatCode = mContent!!.findViewById<View>(R.id.editText_code) as EditText
            val intent = Intent()
            val sdesc = cheatDesc.text.toString()
            val scode = cheatCode.text.toString()
            intent.putExtra(DESC, sdesc)
            intent.putExtra(CODE, scode)
            if (target_ != null) {
                intent.putExtra(KEY, target_!!.key)
            }
            targetFragment!!.onActivityResult(targetRequestCode, APPLY, intent)
            dialog!!.dismiss()
        }
        val button_cancel = mContent!!.findViewById<View>(R.id.button_edit_cheat_cancel) as Button
        button_cancel.setOnClickListener {
            targetFragment!!.onActivityResult(targetRequestCode, CANCEL, null)
            dismiss()
        }
        (mContent!!.findViewById<View>(R.id.editText_code) as EditText).setOnEditorActionListener(
            OnEditorActionListener { _, actionId, _ -> // For ShieldTV
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    val edit_view = mContent!!.findViewById<View>(R.id.editText_code) as EditText
                    var currentstr: String? = edit_view.text.toString()
                    currentstr += System.lineSeparator()
                    edit_view.setText(currentstr)
                    return@OnEditorActionListener true
                }
                false
            })
        return builder.create()
    }

    companion object {
        const val APPLY = 0
        const val CANCEL = 1
        const val KEY = "key"
        const val DESC = "desc"
        const val CODE = "code"
    }
}
