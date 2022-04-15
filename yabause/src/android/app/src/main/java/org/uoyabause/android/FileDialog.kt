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

import org.uoyabause.android.YabauseStorage.Companion.storage
import org.uoyabause.android.ListenerList.FireHandler
import android.app.Activity
import android.app.AlertDialog
import android.app.Dialog
import org.uoyabause.android.ListenerList
import org.uoyabause.android.FileDialog.FileSelectedListener
import org.uoyabause.android.FileDialog.DirectorySelectedListener
import android.content.DialogInterface
import android.util.Log
import org.uoyabause.android.YabauseStorage
import android.widget.Toast
import java.io.File
import java.io.FilenameFilter
import java.util.ArrayList

internal class ListenerList<L> {
    private val listenerList: MutableList<L> = ArrayList()

    interface FireHandler<L> {
        fun fireEvent(listener: L)
    }

    fun add(listener: L) {
        listenerList.add(listener)
    }

    fun fireEvent(fireHandler: FireHandler<L>) {
        val copy: List<L> = ArrayList(listenerList)
        for (l in copy) {
            fireHandler.fireEvent(l)
        }
    }

    fun remove(listener: L) {
        listenerList.remove(listener)
    }

    fun getListenerList(): List<L> {
        return listenerList
    }
}

class FileDialog(private val activity: Activity, path: String?) {
    private val TAG = javaClass.name
    private var fileList: Array<String> = arrayOf<String>()
    private var currentPath: File? = null

    interface FileSelectedListener {
        fun fileSelected(file: File?)
    }

    interface DirectorySelectedListener {
        fun directorySelected(directory: File?)
    }

    private val fileListenerList = ListenerList<FileSelectedListener>()
    private val dirListenerList = ListenerList<DirectorySelectedListener>()
    private var selectDirectoryOption = false
    private var fileEndsWith: String? = null

    /**
     * @return file dialog
     */
    fun createFileDialog(): Dialog? {
        var dialog: Dialog? = null
        val builder = AlertDialog.Builder(activity)
        builder.setTitle(currentPath!!.path)
        if (selectDirectoryOption) {
            builder.setPositiveButton("Select directory") { dialog, which ->
                Log.d(TAG,
                    currentPath!!.path)
                fireDirectorySelectedEvent(currentPath)
            }
        }
        builder.setItems(fileList) { dialog, which ->
            val fileChosen = fileList[which]
            val chosenFile = getChosenFile(fileChosen)
            if (chosenFile.isDirectory) {
                loadFileList(chosenFile)
                dialog.cancel()
                dialog.dismiss()
                showDialog()
            } else fireFileSelectedEvent(chosenFile)
        }
        builder.setNegativeButton("Cancel") { dialog, which ->
            dialog.dismiss()
            fireFileSelectedEvent(null)
        }
        dialog = builder.show()
        return dialog
    }

    fun addFileListener(listener: FileSelectedListener) {
        fileListenerList.add(listener)
    }

    fun removeFileListener(listener: FileSelectedListener) {
        fileListenerList.remove(listener)
    }

    fun setSelectDirectoryOption(selectDirectoryOption: Boolean) {
        this.selectDirectoryOption = selectDirectoryOption
    }

    fun addDirectoryListener(listener: DirectorySelectedListener) {
        dirListenerList.add(listener)
    }

    fun removeDirectoryListener(listener: DirectorySelectedListener) {
        dirListenerList.remove(listener)
    }

    /**
     * Show file dialog
     */
    fun showDialog() {
        createFileDialog()!!.show()
    }

    private fun fireFileSelectedEvent(file: File?) {
        fileListenerList.fireEvent(object : FireHandler<FileSelectedListener> {
            override fun fireEvent(listener: FileSelectedListener) {
                listener.fileSelected(file)
            }
        })
    }

    private fun fireDirectorySelectedEvent(directory: File?) {
        dirListenerList.fireEvent(object : FireHandler<DirectorySelectedListener> {
            override fun fireEvent(listener: DirectorySelectedListener) {
                listener.directorySelected(directory)
            }
        })
    }

    private fun loadFileList(path: File) {
        var path: File? = path
        if (path == null || path.isDirectory == false) {
            val s = storage
            path = File(s.rootPath)
            if (!path.exists()) path.mkdir()
        }
        currentPath = path
        val r: MutableList<String> = ArrayList()
        if (path.exists()) {
            if (path.parentFile != null) r.add(PARENT_DIR)
            val filter = FilenameFilter { dir, filename ->
                val sel = File(dir, filename)
                if (!sel.canRead()) return@FilenameFilter false
                if (selectDirectoryOption) sel.isDirectory else {
                    val endsWith = if (fileEndsWith != null) filename
                        .toLowerCase().endsWith(fileEndsWith!!) else true
                    endsWith || sel.isDirectory
                }
            }
            val fileList1 = path.list(filter)
            if (fileList1 != null) {
                for (file in fileList1) r.add(file)
            } else {
                Toast.makeText(activity,
                    path.absolutePath + " is not readable. ",
                    Toast.LENGTH_LONG)
            }
        }
        fileList = r.toTypedArray() as Array<String>
    }

    private fun getChosenFile(fileChosen: String): File {
        return if (fileChosen == PARENT_DIR) currentPath!!.parentFile else File(
            currentPath,
            fileChosen)
    }

    fun setFileEndsWith(fileEndsWith: String?) {
        this.fileEndsWith = fileEndsWith?.toLowerCase() ?: fileEndsWith
    }

    companion object {
        private const val PARENT_DIR = ".."
    }

    /**
     * @param activity
     * @param path
     */
    init {
        loadFileList(File(path))
    }
}