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
import android.os.Bundle
import android.view.*
import androidx.appcompat.app.AlertDialog
import androidx.preference.PreferenceDialogFragmentCompat
import java.io.*
import java.util.*
import org.devmiyax.yabasanshiro.R

class InputSettingPreferenceFragment() : PreferenceDialogFragmentCompat() {

  val presenter: InputSettingPresenter = InputSettingPresenter()

  lateinit var mainView: View
  override fun onPrepareDialogBuilder(builder: AlertDialog.Builder) {
    super.onPrepareDialogBuilder(builder)
    builder.setTitle(R.string.input_the_key)
    builder.setPositiveButton(null, null)
    val view = requireActivity().layoutInflater.inflate(R.layout.keymap, null, false)
    onBindDialogView(view)
    builder.setView(view)
    mainView = view
  }

  override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
    val dlg = super.onCreateDialog(savedInstanceState)
    presenter.onCreateDialog(requireActivity(), dlg, mainView)
    return dlg
  }

  override fun onDialogClosed(positiveResult: Boolean) {
    if (positiveResult) {
      val `is` = preference as InputSettingPreference
      `is`.save()
    }
  }

  companion object {
    fun newInstance(key: String?): InputSettingPreferenceFragment {
      val fragment = InputSettingPreferenceFragment()
      val b = Bundle(1)
      b.putString(ARG_KEY, key)
      fragment.arguments = b
      return fragment
    }
  }
}
