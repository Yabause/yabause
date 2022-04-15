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

import org.uoyabause.android.cheat.CheatEditDialog
import org.uoyabause.android.cheat.Cheat
import android.view.ViewGroup
import android.view.LayoutInflater
import org.devmiyax.yabasanshiro.R
import android.widget.TextView
import androidx.core.content.ContextCompat
import android.widget.AdapterView
import com.google.firebase.database.DatabaseReference
import org.uoyabause.android.cheat.CheatListAdapter
import android.widget.ArrayAdapter
import android.content.DialogInterface
import org.uoyabause.android.Yabause
import android.os.Bundle
import android.widget.EditText
import android.widget.TextView.OnEditorActionListener
import android.view.inputmethod.EditorInfo
import android.app.Activity
import android.app.AlertDialog
import android.app.Dialog
import androidx.fragment.app.DialogFragment
import android.content.Context
import android.util.Log
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.BaseAdapter
import android.widget.Button
import android.widget.ListAdapter
import android.widget.ListView
import androidx.fragment.app.FragmentActivity
import org.uoyabause.android.cheat.CheatEditDialog.MenuDialogFragment
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import java.util.ArrayList

/**
 * Created by shinya on 2017/02/25.
 */
class CheatListAdapter(parent: CheatEditDialog, list: MutableList<Cheat>?, context: Context) :
    BaseAdapter(), ListAdapter {
    private var list: MutableList<Cheat> = ArrayList()
    private val context: Context
    private val parent: CheatEditDialog
    override fun getCount(): Int {
        return list.size
    }

    override fun getItem(pos: Int): Any {
        return list[pos]
    }

    override fun getItemId(pos: Int): Long {
        return pos.toLong()
        //just return 0 if your list items do not have an Id variable.
    }

    override fun getView(position: Int, convertView: View, parent: ViewGroup): View {
        var view = convertView
        if (view == null) {
            val inflater =
                context.getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
            view = inflater.inflate(R.layout.cheat_item, null)
        }

        //Handle TextView and display string from your list
        val listItemText = view.findViewById<View>(R.id.text_description) as TextView
        listItemText.text = list[position].description
        if (list[position].enable) {
            listItemText.setTextColor(ContextCompat.getColor(context, R.color.colorAccent))
        } else {
            listItemText.setTextColor(ContextCompat.getColor(context, R.color.disable))
        }

        //Handle buttons and add onClickListeners
/*
        Button deleteBtn = (Button)view.findViewById(R.id.button_delete_cheat);
        Button addBtn = (Button)view.findViewById(R.id.button_cheat_edit);
        ToggleButton enableBtn = (ToggleButton)view.findViewById(R.id.toggleButton_enable_cheat);
        enableBtn.setChecked( list.get(position).enable );


        enableBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                list.get(position).enable = !(list.get(position).enable);
            }
        });

        deleteBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                CheatListAdapter.this.parent.RemoveCheat(position);
            }
        });
        addBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                CheatListAdapter.this.parent.EditCheat(position);
            }
        });
*/return view
    }

    fun add(cheat: Cheat) {
        list.add(cheat)
        notifyDataSetChanged()
    }

    fun setItemChecked(position: Int, checked: Boolean) {
        list[position].enable = checked
        notifyDataSetChanged()
    }

    init {
        if (list != null) {
            this.list = list
        }
        this.parent = parent
        this.context = context
    }
}

open class CheatEditDialog : DialogFragment(), AdapterView.OnItemClickListener {
    private var mDatabase: DatabaseReference? = null
    @JvmField
    protected var mGameCode: String? = null
    @JvmField
    var mListView: ListView? = null
    @JvmField
    protected var Cheats: MutableList<Cheat>? = ArrayList()
    @JvmField
    var CheatAdapter: CheatListAdapter? = null

    //List<Cheat> Cheats;
    @JvmField
    var mCheatListCode: Array<String?>? = null
    var adapter: ArrayAdapter<String?>? = null
    var mContent: View? = null
    var editing_ = -1
    fun setGameCode(st: String?, cheat_code: Array<String?>?) {
        mGameCode = st
        mCheatListCode = cheat_code
    }

    override fun onDismiss(dialog: DialogInterface) {
        super.onDismiss(dialog)
        val cntChoice = CheatAdapter!!.count
        var cindex = 0
        for (i in 0 until cntChoice) {
            if (Cheats!![i].enable) {
                cindex++
            }
        }
        mCheatListCode = arrayOf()
        cindex = 0
        for (i in 0 until cntChoice) {
            if (Cheats!![i].enable) {
                mCheatListCode!![cindex] = Cheats!![i].cheat_code
                cindex++
            }
        }
        val activity = activity as Yabause
        activity.updateCheatCode(mCheatListCode)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val builder = AlertDialog.Builder(requireActivity())
        if (mGameCode == null) {
            return builder.create()
        }
        if (LoadData(mGameCode) == -1) {
            return builder.create()
        }
        if (Cheats == null) {
            return builder.create()
        }
        val inflater = requireActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
        mContent = inflater.inflate(R.layout.cheat, null)
        if( mContent == null ){
            return builder.create()
        }
        builder.setView(mContent)
        mListView = mContent?.findViewById<View>(R.id.list_cheats) as ListView
        mListView!!.adapter = CheatAdapter
        mListView!!.onItemClickListener = this
        //getActivity().registerForContextMenu(mListView);
        val edit_view = mContent?.findViewById<View>(R.id.edit_cheat_view)
        edit_view!!.visibility = View.GONE

        // Add new item
        val add_new_button = mContent?.findViewById<View>(R.id.add_new_cheat) as Button
        add_new_button.setOnClickListener {
            val edit_view = mContent?.findViewById<View>(R.id.edit_cheat_view)
            edit_view!!.visibility = View.VISIBLE
            val add_new_button = mContent?.findViewById<View>(R.id.add_new_cheat) as Button
            add_new_button.visibility = View.GONE
            val desc = mContent?.findViewById<View>(R.id.editText_cheat_desc)
            desc!!.requestFocus()
            val imm = requireActivity().getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            imm.showSoftInput(desc, InputMethodManager.SHOW_IMPLICIT)
            editing_ = -1
        }

        // Apply Button
        val button_apply = mContent?.findViewById<View>(R.id.button_cheat_edit_apply) as Button
        button_apply.setOnClickListener {
            val desc = mContent?.findViewById<View>(R.id.editText_cheat_desc) as EditText
            val ccode = mContent?.findViewById<View>(R.id.editText_code) as EditText
            val edit_view = mContent?.findViewById<View>(R.id.edit_cheat_view)
            edit_view!!.visibility = View.GONE
            val add_new_button = mContent?.findViewById<View>(R.id.add_new_cheat) as Button
            add_new_button!!.visibility = View.VISIBLE

            // new item
            if (editing_ == -1) {
                NewItem(mGameCode, desc.text.toString(), ccode.text.toString())
            } else {
                UpdateItem(editing_, mGameCode, desc.text.toString(), ccode.text.toString())
            }
        }
        val button_cancel = mContent?.findViewById<View>(R.id.button_edit_cheat_cancel) as Button
        button_cancel.setOnClickListener {
            val edit_view = mContent?.findViewById<View>(R.id.edit_cheat_view)
            edit_view!!.visibility = View.GONE
            val add_new_button = mContent?.findViewById<View>(R.id.add_new_cheat) as Button
            add_new_button!!.visibility = View.VISIBLE
        }
        (mContent?.findViewById<View>(R.id.editText_code) as EditText).setOnEditorActionListener(
            OnEditorActionListener { v, actionId, event -> //For ShieldTV
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    val edit_view = mContent?.findViewById<View>(R.id.editText_code) as EditText
                    var currentstr: String? = edit_view!!.text.toString()
                    currentstr += System.lineSeparator()
                    edit_view.setText(currentstr)
                    return@OnEditorActionListener true
                }
                // Return true if you have consumed the action, else false.
                false
            })
        return builder.create()
    }

    class MenuDialogFragment : DialogFragment() {
        var isEnable = false
        var position = 0
        var parent: CheatEditDialog? = null
        override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
            var items = arrayOf<CharSequence>("Enable", "Edit", "Delete")
            val activity = activity
            items[0] = resources.getString(R.string.enable)
            items[1] = resources.getString(R.string.edit)
            items[2] = resources.getString(R.string.delete)
            if (parent!!.Cheats!![position].local == false) {
                val clouditems = arrayOf<CharSequence>("Enable")
                clouditems[0] = resources.getString(R.string.enable)
                items = clouditems
            }
            if (isEnable == true) {
                items[0] = resources.getString(R.string.disable)
            }
            val builder = AlertDialog.Builder(getActivity())
            builder.setItems(items) { dialog, which ->
                when (which) {
                    0 -> {
                        parent!!.Cheats!![position].enable = !isEnable
                        parent!!.CheatAdapter!!.notifyDataSetChanged()
                    }
                    1 -> parent!!.EditCheat(position)
                    2 -> parent!!.RemoveCheat(position)
                    else -> {}
                }
            }
            return builder.create()
        }
    }

    override fun onItemClick(parent: AdapterView<*>?, view: View, position: Int, id: Long) {
        // ダイアログを表示する
        val newFragment = MenuDialogFragment()
        newFragment.isEnable = Cheats!![position].enable
        newFragment.position = position
        newFragment.parent = this
        newFragment.show( requireActivity().supportFragmentManager, "cheat_edit_menu")
    }

    open fun NewItem(gameid: String?, desc: String?, value: String?) {
        val key = mDatabase!!.push().key
        mDatabase!!.child(key!!).child("description").setValue(desc)
        mDatabase!!.child(key).child("cheat_code").setValue(value)
    }

    open fun UpdateItem(index: Int, gameid: String?, desc: String?, value: String?) {
        mDatabase!!.child(Cheats!![index].key!!).child("description").setValue(desc)
        mDatabase!!.child(Cheats!![index].key!!).child("cheat_code").setValue(value)
    }

    open fun Remove(index: Int) {
        mDatabase!!.child(Cheats!![index].key!!).removeValue()
    }

    open fun LoadData(gameid: String?): Int {
        Cheats = ArrayList()
        CheatAdapter = CheatListAdapter(this, Cheats, requireActivity())
        mDatabase = FirebaseDatabase.getInstance().reference.child("cheats").child(mGameCode!!)
        if (mDatabase == null) {
            return -1
        }
        val cheatDataListener: ValueEventListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                if (dataSnapshot.hasChildren()) {
                    Cheats?.clear()
                    for (child in dataSnapshot.children) {
                        //ids.add(child.getKey());
                        val newitem = Cheat()
                        newitem.key = child.key
                        newitem.description = child.child("description").value as String?
                        newitem.cheat_code = child.child("cheat_code").value as String?
                        newitem.enable = false
                        newitem.local = false
                        CheatAdapter!!.add(newitem)
                    }
                    if (mCheatListCode != null) {
                        val cntChoice = CheatAdapter!!.count
                        for (i in 0 until cntChoice) {
                            CheatAdapter!!.setItemChecked(i, false)
                            for (j in mCheatListCode!!.indices) {
                                if (Cheats?.get(i)?.cheat_code == mCheatListCode!![j]) {
                                    CheatAdapter!!.setItemChecked(i, true)
                                }
                            }
                        }
                    }
                } else {
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.key)
                }
            }

            override fun onCancelled(databaseError: DatabaseError) {}
        }
        mDatabase!!.addValueEventListener(cheatDataListener)
        return 0
    }

    fun RemoveCheat(pos: Int) {
        editing_ = pos
        AlertDialog.Builder(activity)
            .setMessage("Are you sure to delete " + Cheats!![editing_].description + "?")
            .setPositiveButton("YES") { dialog, which -> Remove(editing_) }
            .setNegativeButton("No", null)
            .show()
    }

    fun EditCheat(pos: Int) {
        val edit_view = mContent!!.findViewById<View>(R.id.edit_cheat_view)
        edit_view.visibility = View.VISIBLE
        val add_new_button = mContent!!.findViewById<View>(R.id.add_new_cheat) as Button
        add_new_button.visibility = View.GONE
        val cheat = mContent!!.findViewById<View>(R.id.editText_code) as EditText
        cheat.setText(Cheats!![pos].cheat_code)
        cheat.requestFocus()
        val desc = mContent!!.findViewById<View>(R.id.editText_cheat_desc) as EditText
        desc.setText(Cheats!![pos].description)
        val imm = requireActivity().getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.showSoftInput(cheat, InputMethodManager.SHOW_IMPLICIT)
        editing_ = pos
    }
}