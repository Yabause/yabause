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
import android.content.Context
import org.uoyabause.android.cheat.LocalCheatItemRecyclerViewAdapter
import org.uoyabause.android.cheat.CheatItem
import com.google.firebase.database.DatabaseReference
import androidx.recyclerview.widget.RecyclerView
import android.os.Bundle
import org.uoyabause.android.cheat.LocalCheatItemFragment
import org.uoyabause.android.cheat.CheatConverter
import android.view.LayoutInflater
import android.view.ViewGroup
import org.devmiyax.yabasanshiro.R
import androidx.recyclerview.widget.LinearLayoutManager
import android.widget.Toast
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.database.DataSnapshot
import org.uoyabause.android.cheat.TabCheatFragment
import com.google.firebase.database.DatabaseError
import android.view.MenuInflater
import org.uoyabause.android.cheat.LocalCheatEditDialog
import android.content.Intent
import android.content.DialogInterface
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.PopupMenu
import androidx.fragment.app.Fragment
import java.util.ArrayList

/**
 * A fragment representing a list of Items.
 *
 *
 * Activities containing this fragment MUST implement the [OnListFragmentInteractionListener]
 * interface.
 */
class LocalCheatItemFragment
/**
 * Mandatory empty constructor for the fragment manager to instantiate the
 * fragment (e.g. upon screen orientation changes).
 */
    : Fragment(), LocalCheatItemRecyclerViewAdapter.OnItemClickListener {
    private var mColumnCount = 1
    private var mListener: OnListFragmentInteractionListener? = null
    private var _items: ArrayList<CheatItem?>? = null
    private var mGameCode: String? = null
    private var backcode_ = ""
    var database_: DatabaseReference? = null
    var root_view_: View? = null
    var listview_: RecyclerView? = null
    var adapter_: LocalCheatItemRecyclerViewAdapter? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        mColumnCount = requireArguments().getInt(ARG_COLUMN_COUNT)
        mGameCode = requireArguments().getString(ARG_GAME_ID)
        val cnv = CheatConverter()
        if (cnv.hasOldVersion()) {
            cnv.execute()
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_localcheatitem_list, container, false)
        listview_ = view.findViewById<View>(R.id.list) as RecyclerView
        val context = view.context
        listview_!!.layoutManager = LinearLayoutManager(context)
        updateCheatList()
        root_view_ = view
        val add = view.findViewById<View>(R.id.button_add) as Button
        add.setOnClickListener { v -> onAddItem(v) }
        return view
    }

    override fun setUserVisibleHint(isVisibleToUser: Boolean) {
        super.setUserVisibleHint(isVisibleToUser)
        if (isVisibleToUser) {
            //updateCheatList();
        } else {
        }
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is OnListFragmentInteractionListener) {
            mListener = context
        } else {
//            throw new RuntimeException(context.toString()
//                    + " must implement OnListFragmentInteractionListener");
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    interface OnListFragmentInteractionListener {
        fun onListFragmentInteraction(item: CheatItem?)
    }

    fun showErrorMessage() {
        Toast.makeText(context, "You need to Sign in before use this function", Toast.LENGTH_LONG)
            .show()
    }

    fun updateCheatList() {
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            showErrorMessage()
            return
        }
        if (listview_ == null) {
            return
        }
        _items = ArrayList()
        val baseref = FirebaseDatabase.getInstance().reference
        val baseurl = "/user-posts/" + auth.currentUser!!.uid + "/cheat/" + mGameCode
        database_ = baseref.child(baseurl)
        if (database_ == null) {
            showErrorMessage()
            return
        }
        adapter_ = LocalCheatItemRecyclerViewAdapter(_items, this@LocalCheatItemFragment)
        listview_!!.adapter = adapter_
        val DataListener: ValueEventListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                if (dataSnapshot.hasChildren()) {
                    _items!!.clear()
                    for (child in dataSnapshot.children) {
                        val newitem = child.getValue(CheatItem::class.java)
                        newitem!!.key = child.key!!
                        val frag = tabCheatFragmentInstance
                        if (frag != null) {
                            newitem.enable = frag.isActive(newitem.cheat_code)
                        }
                        _items!!.add(newitem)
                    }
                    adapter_ =
                        LocalCheatItemRecyclerViewAdapter(_items, this@LocalCheatItemFragment)
                    listview_!!.adapter = adapter_
                } else {
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.key)
                }
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Log.e("CheatEditDialog", "Bad Data " + databaseError.message)
            }
        }
        database_!!.addValueEventListener(DataListener)
    }

    val tabCheatFragmentInstance: TabCheatFragment?
        get() {
            var xFragment: TabCheatFragment? = null
            for (fragment in requireFragmentManager().fragments) {
                if (fragment is TabCheatFragment) {
                    xFragment = fragment
                    break
                }
            }
            return xFragment
        }

    override fun onItemClick(position: Int, item: CheatItem?, v: View?) {
        val popup = PopupMenu(activity, v)
        val inflate = popup.menuInflater
        inflate.inflate(R.menu.local_cheat, popup.menu)
        val cheatitem = item
        val position_ = position
        val popupMenu = popup.menu
        val mitem = popupMenu.getItem(0)

        if( cheatitem == null ){
            return
        }

        if (cheatitem.enable) {
            mitem.setTitle(R.string.disable)
        } else {
            mitem.setTitle(R.string.enable)
        }
        val sitem = popupMenu.getItem(2)
        if (cheatitem.sharedKey != "") {
            sitem.setTitle(R.string.unsahre)
        } else {
            sitem.setTitle(R.string.share)
        }
        popup.setOnMenuItemClickListener(PopupMenu.OnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.acp_activate -> {
                    cheatitem.enable = !cheatitem.enable
                    val frag = tabCheatFragmentInstance
                    if (frag != null) {
                        if (cheatitem.enable) {
                            frag.AddActiveCheat(cheatitem.cheat_code)
                        } else {
                            frag.RemoveActiveCheat(cheatitem.cheat_code)
                        }
                    }
                    adapter_!!.notifyDataSetChanged()
                }
                R.id.acp_edit -> {
                    backcode_ = cheatitem.cheat_code
                    val newFragment = LocalCheatEditDialog()
                    newFragment.setEditTarget(cheatitem)
                    newFragment.setTargetFragment(this@LocalCheatItemFragment, EDIT_ITEM)
                    newFragment.show( requireFragmentManager(), "Cheat")
                }
                R.id.acp_share -> if ( cheatitem.sharedKey != "") {
                    UnShare(cheatitem)
                } else {
                    Share(cheatitem)
                }
                R.id.delete -> {
                    RemoveCheat(cheatitem)
                    adapter_!!.notifyDataSetChanged()
                }
                else -> return@OnMenuItemClickListener false
            }
            false
        })
        popup.show()
    }

    fun onAddItem(v: View?) {
        val newFragment = LocalCheatEditDialog()
        newFragment.setTargetFragment(this, NEW_ITEM)
        newFragment.show(requireFragmentManager(), "Cheat")
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == NEW_ITEM) {
            if (resultCode == LocalCheatEditDialog.APPLY) {
                if (database_ == null) {
                    showErrorMessage()
                    return
                }
                val key = database_!!.push().key
                val desc = data!!.getStringExtra(LocalCheatEditDialog.DESC)
                val code = data.getStringExtra(LocalCheatEditDialog.CODE)
                database_!!.child(key!!).child("description").setValue(desc)
                database_!!.child(key).child("cheat_code").setValue(code)
                adapter_!!.notifyDataSetChanged()
            }
        } else if (requestCode == EDIT_ITEM) {
            if (resultCode == LocalCheatEditDialog.APPLY) {
                if (database_ == null) {
                    showErrorMessage()
                    return
                }
                val frag = tabCheatFragmentInstance
                frag?.RemoveActiveCheat(backcode_)
                val key = data!!.getStringExtra(LocalCheatEditDialog.KEY)
                val desc = data.getStringExtra(LocalCheatEditDialog.DESC)
                val code = data.getStringExtra(LocalCheatEditDialog.CODE)
                database_!!.child(key!!).child("description").setValue(desc)
                database_!!.child(key).child("cheat_code").setValue(code)
                adapter_!!.notifyDataSetChanged()
            }
        }
    }

    fun RemoveCheat(cheatitem: CheatItem) {
        AlertDialog.Builder(activity)
            .setMessage(getString(R.string.are_you_sure_to_delete) + cheatitem.description + "?")
            .setPositiveButton(R.string.yes, DialogInterface.OnClickListener { dialog, which ->
                if (database_ == null) {
                    showErrorMessage()
                    return@OnClickListener
                }
                database_!!.child(cheatitem.key).removeValue()
                if (cheatitem.sharedKey !== "") {
                    val baseref = FirebaseDatabase.getInstance().reference
                    val baseurl = "/shared-cheats/$mGameCode"
                    val sharedb = baseref.child(baseurl)
                    sharedb.child(cheatitem.sharedKey).removeValue()
                }
            })
            .setNegativeButton(R.string.no, null)
            .show()
    }

    fun Remove(index: Int) {
        if (database_ == null) {
            showErrorMessage()
            return
        }
        database_!!.child(_items!![index]!!.key).removeValue()
    }

    fun Share(cheatitem: CheatItem) {
        if (database_ == null) {
            showErrorMessage()
            return
        }
        if (cheatitem.sharedKey != "") {
            return
        }
        val baseref = FirebaseDatabase.getInstance().reference
        val baseurl = "/shared-cheats/$mGameCode"
        val sharedb = baseref.child(baseurl)
        val key = sharedb.push().key
        sharedb.child(key!!).child("description").setValue(cheatitem.description)
        sharedb.child(key).child("cheat_code").setValue(cheatitem.cheat_code)
        database_!!.child(cheatitem.key).child("shared_key").setValue(key)
    }

    fun UnShare(cheatitem: CheatItem) {
        if (database_ == null) {
            showErrorMessage()
            return
        }
        if (cheatitem.sharedKey == "") {
            return
        }
        val baseref = FirebaseDatabase.getInstance().reference
        val baseurl = "/shared-cheats/" + mGameCode + "/" + cheatitem.sharedKey
        val sharedb = baseref.child(baseurl)
        if (sharedb != null) {
            sharedb.removeValue()
        }
        database_!!.child(cheatitem.key).child("shared_key").setValue("")
    }

    companion object {
        private const val ARG_COLUMN_COUNT = "column-count"
        private const val ARG_GAME_ID = "game_id"
        @JvmStatic
        fun newInstance(gameid: String?, columnCount: Int): LocalCheatItemFragment {
            val fragment = LocalCheatItemFragment()
            val args = Bundle()
            args.putString(ARG_GAME_ID, gameid)
            args.putInt(ARG_COLUMN_COUNT, columnCount)
            fragment.arguments = args
            return fragment
        }

        const val NEW_ITEM = 0
        const val EDIT_ITEM = 1
    }
}