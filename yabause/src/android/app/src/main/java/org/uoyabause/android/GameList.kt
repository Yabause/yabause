/*  Copyright 2015 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
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

import android.app.ListActivity
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ListView
import androidx.preference.PreferenceManager
import java.io.File
import java.io.UnsupportedEncodingException
import java.net.URLDecoder
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.YabauseStorage.Companion.storage

class GameList : ListActivity(), FileSelectedListener {
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val adapter = GameListAdapter(this)
        listAdapter = adapter
    }

    public override fun onListItemClick(l: ListView, v: View, position: Int, id: Long) {
        val string = l.getItemAtPosition(position) as String
        if (position == 0) {

            // This method is not supported on Android TV
            // Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            // intent.setType("file/*");
            // startActivityForResult(intent, CHOSE_FILE_CODE);
            val yabroot = File(storage.rootPath)
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
            val last_dir = sharedPref.getString("pref_last_dir", yabroot.path)
            val fd = FileDialog(this, last_dir)
            fd.addFileListener(this)
            fd.showDialog()
        } else {
            val intent = Intent(this, Yabause::class.java)
            intent.putExtra("org.uoyabause.android.FileName", string)
            startActivity(intent)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent) {
        try {
            if (requestCode == CHOSE_FILE_CODE && resultCode == RESULT_OK) {
                val filePath = data.dataString!!.replace("file://", "")
                val decodedfilePath = URLDecoder.decode(filePath, "utf-8")
                val intent = Intent(this, Yabause::class.java)
                intent.putExtra("org.uoyabause.android.FileNameEx", decodedfilePath)
                startActivity(intent)
            }
        } catch (e: UnsupportedEncodingException) {
            e.printStackTrace()
        }
    }

    override fun fileSelected(file: File?) {
        val string: String
        string = file!!.absolutePath

        // save last selected dir
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        val editor = sharedPref.edit()
        editor.putString("pref_last_dir", file.parent)
        editor.apply()

        val intent = Intent(this, Yabause::class.java)
        intent.putExtra("org.uoyabause.android.FileNameEx", string)
        startActivity(intent)
    }

    companion object {
        private const val TAG = "Yabause"
        private const val CHOSE_FILE_CODE = 7123
    }
}
