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
package org.uoyabause.android.backup

import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.util.Base64
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.PopupMenu
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.gms.tasks.OnSuccessListener
import com.google.android.material.snackbar.Snackbar
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.Exclude
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.IgnoreExtraProperties
import com.google.firebase.database.ValueEventListener
import com.google.firebase.storage.FirebaseStorage
import com.google.firebase.storage.StorageMetadata
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.json.JSONException
import org.json.JSONObject
import org.uoyabause.android.AuthFragment
import org.uoyabause.android.YabauseRunnable
import org.uoyabause.android.backup.BackupItemFragment.OnListFragmentInteractionListener

internal class BackupDevice {
    @JvmField
    var id_ = 0
    @JvmField
    var name_: String? = null

    companion object {
        @JvmField
        var DEVICE_CLOUD = 48
    }
}

@IgnoreExtraProperties
class BackupItem {
    var index_ = 0
    @JvmField
    var filename: String? = null
    @JvmField
    var comment: String? = null
    var language = 0
    @JvmField
    var savedate: String? = null
    @JvmField
    var datasize = 0
    var blocksize = 0
    var url = ""
    var key: String? = ""

    // public Map<String, Boolean> stars = new HashMap<>();
    constructor() {}

    constructor(
        index: Int,
        filename: String?,
        comment: String?,
        language: Int,
        savedate: String?,
        datasize: Int,
        blocksize: Int,
        url: String
    ) {
        index_ = index
        this.filename = filename
        this.comment = comment
        this.language = language
        this.savedate = savedate
        this.datasize = datasize
        this.blocksize = blocksize
        this.url = url
    }

    fun getindex(): Int {
        return index_
    }

    val DATE_PATTERN = "yyyy/MM/dd HH:mm"
    @Exclude
    fun toMap(): Map<String, Any?> {
        val result =
            HashMap<String, Any?>()
        result["filename"] = filename
        result["comment"] = comment
        result["language"] = language
        result["savedate"] = savedate
        result["datasize"] = datasize
        result["blocksize"] = blocksize
        result["url"] = url
        result["key"] = key
        return result
    }
}

/**
 * A fragment representing a list of Items.
 *
 *
 * Activities containing this fragment MUST implement the [OnListFragmentInteractionListener]
 * interface.
 */
class BackupItemFragment : AuthFragment(),
    BackupItemRecyclerViewAdapter.OnItemClickListener {
    internal var backup_devices_: MutableList<BackupDevice>? = null
    private var mListener: OnListFragmentInteractionListener? =
        null
    private var _items: ArrayList<BackupItem> = ArrayList()
    var currentpage_ = 0
    var root_view_: View? = null
    var view_: RecyclerView? = null
    var sum_: TextView? = null
    var adapter_: BackupItemRecyclerViewAdapter? = null
    var database_: DatabaseReference? = null
    private var totalsize_ = 0
    private var freesize_ = 0
    private var backup_list_count_: Long = 0
    private var backup_max_list_count_: Long = 0
    override fun setUserVisibleHint(isVisibleToUser: Boolean) {
        super.setUserVisibleHint(isVisibleToUser)
        if (isVisibleToUser) {
            updateSaveList(currentpage_)
        } else {
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (arguments != null) {
            currentpage_ = requireArguments().getInt("position")
        }
        val jsonstr: String
        jsonstr = YabauseRunnable.getDevicelist()
        backup_devices_ = mutableListOf()
        try {
            val json = JSONObject(jsonstr)
            val array = json.getJSONArray("devices")
            for (i in 0 until array.length()) {
                val data = array.getJSONObject(i)
                val tmp = BackupDevice()
                tmp.name_ = data.getString("name")
                tmp.id_ = data.getInt("id")
                backup_devices_?.add(tmp)
            }
            val tmp = BackupDevice()
            tmp.name_ = "cloud"
            tmp.id_ = BackupDevice.DEVICE_CLOUD
            backup_devices_?.add(tmp)
        } catch (e: JSONException) {
            Log.e(TAG, "Fail to convert to json", e)
        }

        if (backup_devices_?.size == 0) {
            Log.e(TAG, "Can't find device")
        }
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view =
            inflater.inflate(R.layout.fragment_backupitem_list, container, false)
        view_ = view.findViewById<View>(R.id.list) as RecyclerView
        sum_ = view.findViewById<View>(R.id.tvSum) as TextView
        val context = view.context
        view_!!.layoutManager = LinearLayoutManager(context)
        root_view_ = view
        updateSaveList(currentpage_)
        return view
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    override fun OnAuthAccepted() {
        updateSaveListCloud()
    }

    fun updateSaveListCloud() {
        val auth = checkAuth() ?: return
        if (auth.currentUser == null) {
            return
        }

        val baseref = FirebaseDatabase.getInstance().reference
        val user_ref = "/user-posts/" + auth.currentUser!!.uid
        val prefs: SharedPreferences? = getActivity()?.getSharedPreferences("private", Context.MODE_PRIVATE)
        var hasDonated = false
        if (prefs != null) {
            hasDonated = prefs.getBoolean("donated", false)
        }
        if (BuildConfig.BUILD_TYPE == "pro" || hasDonated) {
            baseref.child(user_ref).child("max_backup_count").setValue(256)
        } else {
            baseref.child(user_ref).child("max_backup_count").setValue(3)
        }

        val baseurl = "/user-posts/" + auth.currentUser!!.uid + "/backup/"
        database_ = baseref.child(baseurl)
        if (database_ == null) {
            return
        }
        val DataListener: ValueEventListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                if (dataSnapshot.hasChildren()) {
                    backup_list_count_ = dataSnapshot.childrenCount
                    _items!!.clear()
                    var index = 0
                    for (child in dataSnapshot.children) {
                        val newitem = child.getValue(BackupItem::class.java)!!
                        newitem.index_ = index
                        index++
                        _items!!.add(newitem)
                    }
                    if (view_ != null) {
                        adapter_ = BackupItemRecyclerViewAdapter(
                            currentpage_,
                            _items,
                            this@BackupItemFragment
                        )
                        view_!!.adapter = adapter_
                    }
                    if (sum_ != null) {
                        sum_!!.text = backup_list_count_.toString() + "/" + backup_max_list_count_.toString()
                    }
                } else {
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.key)
                }
            }

            override fun onCancelled(databaseError: DatabaseError) {}
        }
        // database_.addListenerForSingleValueEvent(DataListener);
        database_!!.addValueEventListener(DataListener)

        val count_url = "/user-posts/" + auth.currentUser!!.uid + "/max_backup_count"
        val count_baseref = baseref.child(count_url)
        val dataCountListner = object : ValueEventListener {
            override fun onCancelled(p0: DatabaseError) {
            }
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                backup_max_list_count_ = dataSnapshot.getValue() as Long
                if (sum_ != null) {
                    sum_!!.text = backup_list_count_.toString() + "/" + backup_max_list_count_.toString()
                }
            }
        }
        count_baseref.addValueEventListener(dataCountListner)
    }

    fun updateSaveList(device: Int) {
        if (backup_devices_ == null) {
            return
        }
        if (backup_devices_!![device].id_ == BackupDevice.DEVICE_CLOUD) {
            updateSaveListCloud()
            return
        }
        val jsonstr = YabauseRunnable.getFilelist(device)
        _items.clear()
        try {
            val json = JSONObject(jsonstr)
            totalsize_ = json.getJSONObject("status").getInt("totalsize")
            freesize_ = json.getJSONObject("status").getInt("freesize")
            val array = json.getJSONArray("saves")
            for (i in 0 until array.length()) {
                val data = array.getJSONObject(i)
                val charset = Charsets.ISO_8859_1 // ToDO MS932
                val tmp = BackupItem()
                tmp.index_ = data.getInt("index")
                var bfilename =
                    Base64.decode(data.getString("filename"), 0) as ByteArray
                try {
                    tmp.filename = String(bfilename, 0, bfilename.size, charset("MS932")) // String(bfilename!!, "MS932")
                } catch (e: Exception) {
                    tmp.filename = data.getString("filename")
                }
                bfilename = Base64.decode(data.getString("comment"), 0)as ByteArray
                try {
                    tmp.comment = String(bfilename, 0, bfilename.size, charset("MS932")) // String(bfilename!!, "MS932")
                } catch (e: Exception) {
                    tmp.comment = data.getString("comment")
                }
                tmp.datasize = data.getInt("datasize")
                tmp.blocksize = data.getInt("blocksize")
                var datestr = ""
                datestr += String.format("%04d", data.getInt("year") + 1980) + "/"
                datestr += String.format("%02d", data.getInt("month")) + "/"
                datestr += String.format("%02d", data.getInt("day"))
                datestr += " "
                datestr += String.format("%02d", data.getInt("hour")) + ":"
                datestr += String.format("%02d", data.getInt("minute")) + ":00"
                tmp.savedate = datestr
                _items!!.add(tmp)
            }
            if (view_ != null) {
                adapter_ = BackupItemRecyclerViewAdapter(currentpage_, _items, this)
                view_!!.adapter = adapter_
            }
            if (sum_ != null) {
                sum_!!.text = String.format(
                    "%,d Byte free of %,d Byte",
                    freesize_,
                    totalsize_
                )
            }
        } catch (e: JSONException) {
            Log.e(TAG, "Fail to convert to json", e)
        }
    }
/*
    fun setFullscreen(activity: Activity) {
        if (Build.VERSION.SDK_INT > 10) {
            var flags =
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_FULLSCREEN
            if (isImmersiveAvailable) {
                flags =
                    flags or (View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)
            }
            activity.window.decorView.systemUiVisibility = flags
        } else {
            activity.window
                .setFlags(
                    WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN
                )
        }
    }

    fun exitFullscreen(activity: Activity) {
        if (Build.VERSION.SDK_INT > 10) {
            activity.window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_VISIBLE
        } else {
            activity.window
                .setFlags(
                    WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN
                )
        }
    }
*/
    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     *
     *
     * See the Android Training lesson [Communicating with Other Fragments](http://developer.android.com/training/basics/fragments/communicating.html) for more information.
     */
    interface OnListFragmentInteractionListener {
        // TODO: Update argument type and name
        fun onItemClickx(
            currentpage: Int,
            position: Int,
            backupitem: BackupItem?,
            v: View?
        )
    }

    override fun onItemClick(
        currentpage: Int,
        position: Int,
        backupitem: BackupItem,
        v: View
    ) {
        val popup = PopupMenu(activity, v)
        val inflate = popup.menuInflater
        inflate.inflate(R.menu.backup, popup.menu)
        val popupMenu = popup.menu
        popupMenu.findItem(R.id.copy_to_internal).isVisible = false
        popupMenu.findItem(R.id.copy_to_cloud).isVisible = false
        popupMenu.findItem(R.id.copy_to_external).isVisible = false
        for (i in backup_devices_!!.indices) {
            if (currentpage != i) {
                if (backup_devices_!![i].id_ == 0) {
                    popupMenu.findItem(R.id.copy_to_internal).isVisible = true
                } else if (backup_devices_!![i].id_ == BackupDevice.DEVICE_CLOUD) {
                    popupMenu.findItem(R.id.copy_to_cloud).isVisible = true
                } else {
                    popupMenu.findItem(R.id.copy_to_external).isVisible = true
                }
            }
        }
        popup.setOnMenuItemClickListener(PopupMenu.OnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.copy_to_internal -> {
                    if (backup_devices_!![currentpage_].id_ == BackupDevice.DEVICE_CLOUD) {
                        val bk = backup_devices_!![0]
                        downalodfromCloud(bk.id_, backupitem)
                    }
                    if (backup_devices_!!.size >= 1) {
                        val bk = backup_devices_!![0]
                        YabauseRunnable.copy(bk.id_, backupitem.index_)
                    }
                }
                R.id.copy_to_external -> {
                    if (backup_devices_!![currentpage_].id_ == BackupDevice.DEVICE_CLOUD) {
                        if (backup_devices_!!.size < 3) {
                            Snackbar
                                .make(
                                    root_view_!!,
                                    "Internal device is not found.",
                                    Snackbar.LENGTH_SHORT
                                )
                                .show()
                        }
                        val bk = backup_devices_!![1]
                        downalodfromCloud(bk.id_, backupitem)
                    }
                    if (backup_devices_!!.size >= 2) {
                        val bk = backup_devices_!![1]
                        YabauseRunnable.copy(bk.id_, backupitem.index_)
                    }
                }
                R.id.copy_to_cloud -> uploadToCloud(backupitem)
                R.id.delete -> AlertDialog.Builder(activity)
                    .setTitle(R.string.deleting_file)
                    .setMessage(R.string.sure_delete)
                    .setNegativeButton(R.string.no, null)
                    .setPositiveButton(R.string.yes) { dialog, which ->
                        if (backup_devices_!![currentpage_].id_ == BackupDevice.DEVICE_CLOUD) {
                            deleteCloudItem(backupitem)
                        } else {
                            YabauseRunnable.deletefile(backupitem.index_)
                        }
                        // view_!!.clearFocus()
                        // view_!!.removeViewAt(position)
                        _items!!.removeAt(position)
                        adapter_!!.notifyItemRemoved(position)
                        adapter_!!.notifyItemRangeChanged(
                            position,
                            _items!!.size
                        )
                    }
                    .show()
                else -> return@OnMenuItemClickListener false
            }
            false
        })
        popup.show()
    }

    fun downalodfromCloud(deviceid: Int, backupitemi: BackupItem) {
        if (backupitemi.url == "") {
            return
        }
        val storage = FirebaseStorage.getInstance()
        val httpsReference = storage.getReferenceFromUrl(backupitemi.url)
        val ONE_MEGABYTE = 1024 * 1024.toLong()
        httpsReference.getBytes(ONE_MEGABYTE)
            .addOnSuccessListener(OnSuccessListener { bytes ->
                val jsonstr = YabauseRunnable.getFilelist(deviceid)
                var freesize = 0
                freesize = try {
                    val json = JSONObject(jsonstr)
                    json.getJSONObject("status").getInt("freesize")
                } catch (e: Exception) {
                    return@OnSuccessListener
                }
                if (backupitemi.datasize >= freesize) {
                    Snackbar
                        .make(
                            root_view_!!,
                            "Not enough free space in the target device",
                            Snackbar.LENGTH_SHORT
                        )
                        .show()
                    return@OnSuccessListener
                }
                val str = String(bytes!!)
                YabauseRunnable.putFile(str)
                Snackbar
                    .make(root_view_!!, "Finished!", Snackbar.LENGTH_SHORT)
                    .show()
            }).addOnFailureListener { exception ->
                Snackbar
                    .make(
                        root_view_!!,
                        "Fail to download from cloud " + exception.localizedMessage,
                        Snackbar.LENGTH_SHORT
                    )
                    .show()
            }
    }

    fun deleteCloudItem(backupitemi: BackupItem) {
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            return
        }
        if (backupitemi.key == "") {
            return
        }
        val storage = FirebaseStorage.getInstance()
        val storageRef = storage.reference
        val base = storageRef.child(auth.currentUser!!.uid)
        val backup = base.child("backup")
        val fileref = backup.child(backupitemi.key!!)
        fileref.delete()
        database_!!.child(backupitemi.key!!).removeValue()
    }

    fun uploadToCloud(backupitemi: BackupItem) {
        val jsonstr = YabauseRunnable.getFile(backupitemi.index_)
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            return
        }

        // Managmaent part
        val baseurl = "/user-posts/" + auth.currentUser!!.uid + "/backup/"
        if (database_ == null) {
            val baseref = FirebaseDatabase.getInstance().reference
            database_ = baseref.child(baseurl)
            if (database_ == null) {
                return
            }
        }
        val dbref: String
        if (backupitemi.key != "") {
            val postValues = backupitemi.toMap()
            dbref = baseurl + backupitemi.key
            database_!!.child(backupitemi.key!!).setValue(postValues)
            uploadData(backupitemi, jsonstr, auth.currentUser!!.uid, dbref)
        } else {
            val DataListener: ValueEventListener = object : ValueEventListener {
                override fun onCancelled(p0: DatabaseError) {
                }

                // Get current record count to check not exceed maxcount
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    val count = dataSnapshot.getChildrenCount()
                    val count_url = "/user-posts/" + auth.currentUser!!.uid + "/max_backup_count"
                    val baseref = FirebaseDatabase.getInstance().reference
                    val dataCountListner = object : ValueEventListener {
                        override fun onDataChange(dataSnapshot: DataSnapshot) {
                            val max_count = dataSnapshot.getValue() as Long
                            if (count < max_count) {

                                val baseurl = "/user-posts/" + auth.currentUser!!.uid + "/backup/"
                                val baseref = FirebaseDatabase.getInstance().reference
                                var db = baseref.child(baseurl)
                                val newPostRef = db.push()
                                val dbref = baseurl + newPostRef.key
                                backupitemi.key = newPostRef.key
                                val postValues = backupitemi.toMap()
                                newPostRef.setValue(postValues)
                                uploadData(backupitemi, jsonstr, auth.currentUser!!.uid, dbref)
                            } else {

                                val v = this@BackupItemFragment.view
                                if (v != null) {
                                    val snackbar = Snackbar.make(v, "You have reached the max slot count. to expand slot count, get pro version.", Snackbar.LENGTH_LONG)
                                    snackbar.setAction("OK!", object : View.OnClickListener {
                                        override fun onClick(v: View?) {
                                            val intent = Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=org.devmiyax.yabasanshioro2.pro"))
                                            startActivity(intent)
                                        }
                                    })
                                    snackbar.show()
                                }
                            }
                        }
                        override fun onCancelled(databaseError: DatabaseError) {
                        }
                    }
                    baseref.child(count_url).addListenerForSingleValueEvent(dataCountListner)
                }
            }
            val baseref = FirebaseDatabase.getInstance().reference
            baseref.child(baseurl).addListenerForSingleValueEvent(DataListener)
        }
    }

    fun uploadData(backupitemi: BackupItem, jsonstr: String, uid: String, dbref: String) {
        // data part
        val storage = FirebaseStorage.getInstance()
        val storageRef = storage.reference
        val base = storageRef.child(uid)
        val backup = base.child("backup")
        val fileref = backup.child(backupitemi.key!!)
        val data = jsonstr.toByteArray()
        val metadata = StorageMetadata.Builder()
                .setContentType("text/json")
                .setCustomMetadata("dbref", dbref)
                .setCustomMetadata("uid", uid)
                .setCustomMetadata("filename", backupitemi.filename)
                .setCustomMetadata("comment", backupitemi.comment)
                .setCustomMetadata("size", String.format("%dByte", backupitemi.datasize))
                .setCustomMetadata("date", backupitemi.savedate)
                .build()
        val uploadTask = fileref.putBytes(data, metadata)
        // Listen for state changes, errors, and completion of the upload.
        val message =
                uploadTask.addOnProgressListener { taskSnapshot ->
                    val progress =
                            100.0 * taskSnapshot.bytesTransferred / taskSnapshot.totalByteCount
                    println("Upload is $progress% done")
                }
                        .addOnPausedListener { println("Upload is paused") }
                        .addOnFailureListener { exception ->
                            Snackbar
                                    .make(
                                            root_view_!!,
                                            "Failed to upload " + exception.localizedMessage,
                                            Snackbar.LENGTH_SHORT
                                    )
                                    .show()
                        }
                        .addOnSuccessListener { taskSnapshot ->
                            Snackbar
                                    .make(root_view_!!, "Success to upload ", Snackbar.LENGTH_SHORT)
                                    .show()
                            val dbref = taskSnapshot.metadata!!.getCustomMetadata("dbref")
                        }
        val urlTask =
                uploadTask.continueWithTask { task ->
                    if (!task.isSuccessful) {
                        throw task.exception!!
                    }
                    fileref.downloadUrl
                }.addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        val downloadUri = task.result
                        val baseref =
                                FirebaseDatabase.getInstance().reference
                        val ref = baseref.child("$dbref/url")
                        ref.setValue(downloadUri.toString())
                    } else {
                    }
                }
    }

    companion object {
        const val TAG = "BackupItemFragment"
        private const val RC_SIGN_IN = 123
        @JvmStatic
        fun getInstance(position: Int): BackupItemFragment {
            val f = BackupItemFragment()
            val args = Bundle()
            args.putInt("position", position)
            f.arguments = args
            return f
        }

        fun newInstance(columnCount: Int): BackupItemFragment {
            val fragment = BackupItemFragment()
            val args = Bundle()
            fragment.arguments = args
            return fragment
        }

        val isImmersiveAvailable: Boolean
            get() = Build.VERSION.SDK_INT >= 19
    }
}
