package org.uoyabause.android

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.database.Cursor
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.Button
import android.widget.ListAdapter
import android.widget.ListView
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.documentfile.provider.DocumentFile
import androidx.preference.PreferenceDialogFragmentCompat
import java.io.File
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.FileDialog.DirectorySelectedListener
import org.uoyabause.android.YabauseStorage.Companion.storage

class DirectoryListAdapter(private val context: Context) : BaseAdapter(), ListAdapter {
  var list = ArrayList<String>()
    private set

  fun setDirectorytList(directoryList: ArrayList<String>) {
    list = directoryList
  }

  override fun getCount(): Int {
    return list.size
  }

  override fun getItem(position: Int): Any {
    return list[position]
  }

  override fun getItemId(position: Int): Long {
    return position.toLong() // _directoryList.get(position).getId();
  }

  fun addDirectory(path: String) {
    list.add(path)
  }

  override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
    var view = convertView
    if (view == null) {
      val inflater = context.getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
      view = inflater.inflate(R.layout.game_directories_listitem, null)
    }

    (view!!.findViewById<View>(R.id.game_directory_item) as TextView).text = list[position]
    val deleteBtn = view.findViewById<View>(R.id.button_delete) as Button
    deleteBtn.tag = position
    deleteBtn.setOnClickListener { v ->
      val clickPosition = v.tag as Int
      list.removeAt(clickPosition)
      notifyDataSetChanged()
    }
    return view
  }
}

class GameDirectoriesDialogFragment : PreferenceDialogFragmentCompat(), View.OnClickListener,
  DirectorySelectedListener {
  var adapter: DirectoryListAdapter? = null
  var listView: ListView? = null

  override fun onPrepareDialogBuilder(builder: AlertDialog.Builder) {
    super.onPrepareDialogBuilder(builder)
    val view = layoutInflater.inflate(R.layout.game_directories, null, false)

    val list: ArrayList<String>
    list = ArrayList()
    val `is` = preference as GameDirectoriesDialogPreference
    val data = `is`.data
    if (data == "err") {
      val yb = storage
      list.add(yb.gamePath)
    } else {
      val paths = data.split(";").dropLastWhile { it.isEmpty() }
        .toTypedArray()
      for (i in paths.indices) {
        list.add(paths[i])
      }
    }
    listView = view.findViewById<View>(R.id.listView) as ListView
    adapter = DirectoryListAdapter(requireActivity())
    adapter!!.setDirectorytList(list)
    listView!!.adapter = adapter
    val button: View = view.findViewById<View>(R.id.button_add) as Button
    button.setOnClickListener(this)

    builder.setView(view)
  }

  override fun onDialogClosed(positiveResult: Boolean) {
    if (positiveResult) {
      var resultstring = ""
      val list = adapter!!.list
      for (i in list.indices) {
        resultstring += list[i]
        resultstring += ";"
      }
      val `is` = preference as GameDirectoriesDialogPreference
      `is`.save(resultstring)
    }
  }

  override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    super.onActivityResult(requestCode, resultCode, data)
    if (requestCode == 111 && resultCode == Activity.RESULT_OK) {

      val contentResolver = YabauseApplication.appContext.contentResolver
      val takeFlags: Int = Intent.FLAG_GRANT_READ_URI_PERMISSION
      contentResolver.takePersistableUriPermission(data!!.data!!, takeFlags)

      var cursor: Cursor? = null
      try {
        val pickedDir = DocumentFile.fromTreeUri(YabauseApplication.appContext, data.data!!)
        for (file in pickedDir!!.listFiles()) {
          Log.d("Yabause", "Found file " + file.name + " with size " + file.length())
        }
        adapter!!.addDirectory(data.data!!.toString())
        adapter!!.notifyDataSetChanged()
      } catch (e: Exception) {
        e.localizedMessage?.let { Log.e("Yabause", it) }
      } finally {
        cursor?.close()
      }
    }
  }

  override fun onClick(v: View) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {

      val rtn = YabauseApplication.checkDonated(requireActivity(), "")
      if (rtn == 0) {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
          flags = Intent.FLAG_GRANT_READ_URI_PERMISSION
        }
        startActivityForResult(intent, 111)
      }
    } else {
      val fd = FileDialog(requireActivity(), "")
      fd.setSelectDirectoryOption(true)
      fd.addDirectoryListener(this)
      fd.showDialog()
    }
  }

  override fun directorySelected(directory: File?) {
    if (directory != null) {
      adapter!!.addDirectory(directory.absolutePath)
      adapter!!.notifyDataSetChanged()
    }
  }

  companion object {
    fun newInstance(key: String?): GameDirectoriesDialogFragment {
      val fragment = GameDirectoriesDialogFragment()
      val b = Bundle(1)
      b.putString(ARG_KEY, key)
      fragment.arguments = b
      return fragment
    }
  }
}
