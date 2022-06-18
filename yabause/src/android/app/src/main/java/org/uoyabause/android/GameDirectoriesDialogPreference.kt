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

import android.app.Activity
import android.content.Context
import android.util.AttributeSet
import androidx.preference.DialogPreference
import org.uoyabause.android.GameDirectoriesDialogPreference.DirListChangeListener

class GameDirectoriesDialogPreference : DialogPreference {
    interface DirListChangeListener {
        fun onChangeDir(isChange: Boolean?)
    }

    var listener: DirListChangeListener? = null
    fun setDirListChangeListener(l: DirListChangeListener?) {
        listener = l
    }

    constructor(context: Context?, attrs: AttributeSet?) : super(
        context!!, attrs) { // InitObjects(context);
    }

    fun save(resultstring: String) {
        val data = getPersistedString("err")
        if (data != resultstring) {
            // _context.setDireListChangeStatus(true);
        }
        persistString(resultstring)
        if (listener != null) {
            listener!!.onChangeDir(true)
        }
    }

    val data: String
        get() = getPersistedString("err")

    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(
        context!!, attrs, defStyle) {
        // InitObjects(context);
        if (listener != null) {
            listener!!.onChangeDir(false)
        }
    }
}
