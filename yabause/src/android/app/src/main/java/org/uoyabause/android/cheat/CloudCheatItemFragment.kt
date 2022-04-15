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

import android.content.Context
import org.uoyabause.android.AuthFragment
import org.uoyabause.android.cheat.CloudCheatItemRecyclerViewAdapter
import org.uoyabause.android.cheat.CheatItem
import com.google.firebase.database.DatabaseReference
import androidx.recyclerview.widget.RecyclerView
import android.os.Bundle
import org.uoyabause.android.cheat.CloudCheatItemFragment
import android.view.LayoutInflater
import android.view.ViewGroup
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.cheat.TabCheatFragment
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import android.view.MenuInflater
import android.content.DialogInterface
import android.util.Log
import android.view.View
import android.widget.PopupMenu
import androidx.appcompat.app.AlertDialog
import java.lang.Exception
import java.util.ArrayList
import java.util.Collections

/**
 * A fragment representing a list of Items.
 *
 *
 * Activities containing this fragment MUST implement the [OnListFragmentInteractionListener]
 * interface.
 */
class CloudCheatItemFragment
/**
 * Mandatory empty constructor for the fragment manager to instantiate the
 * fragment (e.g. upon screen orientation changes).
 */
    : AuthFragment(), CloudCheatItemRecyclerViewAdapter.OnItemClickListener {
    private var mColumnCount = 1
    private var mListener: OnListFragmentInteractionListener? = null
    private var _items: ArrayList<CheatItem?>? = null
    private var mGameCode: String? = null
    var database_: DatabaseReference? = null
    var root_view_: View? = null
    var listview_: RecyclerView? = null
    var adapter_: CloudCheatItemRecyclerViewAdapter? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        mColumnCount = requireArguments().getInt(ARG_COLUMN_COUNT)
        mGameCode = requireArguments().getString(ARG_GAME_ID)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_cloudcheatitem_list, container, false)
        listview_ = view.findViewById<View>(R.id.list) as RecyclerView
        //sum_ = (TextView) view.findViewById(R.id.tvSum);
        val context = view.context
        //LinearLayoutManager layoutManager = new LinearLayoutManager(context);
        //layoutManager.setReverseLayout(true);
        //layoutManager.setStackFromEnd(true);
        //listview_.setLayoutManager(layoutManager);
        root_view_ = view
        updateCheatList()
        return view
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is OnListFragmentInteractionListener) {
            mListener = context
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
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

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     *
     *
     * See the Android Training lesson [Communicating with Other Fragments](http://developer.android.com/training/basics/fragments/communicating.html) for more information.
     */
    internal interface OnListFragmentInteractionListener {
        // TODO: Update argument type and name
        fun onListFragmentInteraction(item: CheatItem?)
    }

    override fun OnAuthAccepted() {
        updateCheatList()
    }

    fun updateCheatList() {
        val auth = checkAuth() ?: return
        _items = ArrayList()
        val baseref = FirebaseDatabase.getInstance().reference
        val baseurl = "/shared-cheats/$mGameCode"
        database_ = baseref.child(baseurl)
        if (database_ == null) {
            return
        }
        adapter_ = CloudCheatItemRecyclerViewAdapter(_items, this@CloudCheatItemFragment)
        listview_!!.adapter = adapter_
        val DataListener: ValueEventListener = object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                if (dataSnapshot.hasChildren()) {
                    _items!!.clear()
                    for (child in dataSnapshot.children) {
                        try {
                            val newitem = child.getValue(CheatItem::class.java)
                            newitem!!.key = child.key!!
                            val frag = tabCheatFragmentInstance
                            if (frag != null && newitem.cheat_code != null ) {
                                newitem.enable = frag.isActive(newitem.cheat_code!!)
                            }
                            _items!!.add(newitem)
                        } catch (e: Exception) {
                        }
                    }
                    Collections.reverse(_items)
                    adapter_ =
                        CloudCheatItemRecyclerViewAdapter(_items, this@CloudCheatItemFragment)
                    listview_!!.adapter = adapter_
                    adapter_!!.notifyDataSetChanged()
                } else {
                    Log.e(TAG, "Bad Data " + dataSnapshot.key)
                }
            }

            override fun onCancelled(databaseError: DatabaseError) {
                Log.e(TAG, "onCancelled " + databaseError.message)
            }
        }
        database_!!.orderByChild("star_count").addValueEventListener(DataListener)
    }

    override fun onItemClick(position: Int, item: CheatItem?, v: View?) {
        val popup = PopupMenu(activity, v)
        val inflate = popup.menuInflater
        inflate.inflate(R.menu.cloud_cheat, popup.menu)
        val cheatitem = item
        val popupMenu = popup.menu
        val mitem = popupMenu.getItem(0)

        if( cheatitem == null ) return

        if (cheatitem.enable) {
            mitem.setTitle(R.string.disable)
        } else {
            mitem.setTitle(R.string.enable)
        }
        popup.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.acp_activate -> {
                    cheatitem.enable = !cheatitem.enable
                    val frag = tabCheatFragmentInstance
                    if (frag != null && cheatitem.cheat_code != null ) {
                        if (cheatitem.enable) {
                            frag.AddActiveCheat(cheatitem.cheat_code)
                        } else {
                            frag.RemoveActiveCheat(cheatitem.cheat_code)
                        }
                    }
                    adapter_!!.notifyDataSetChanged()
                }
                R.id.acp_rate -> starItem(cheatitem)
            }
            false
        }
        popup.show()
    }

    fun starItem(item: CheatItem) {
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            return
        }
        val items = arrayOf("★", "★★", "★★★", "★★★★", "★★★★★")
        val defaultItem = 0
        val checkedItems: MutableList<Int> = ArrayList()
        checkedItems.add(defaultItem)
        AlertDialog.Builder(requireActivity())
            .setTitle("Rate this cheat")
            .setSingleChoiceItems(items, defaultItem) { dialog, which ->
                checkedItems.clear()
                checkedItems.add(which)
            }
            .setPositiveButton(R.string.ok, DialogInterface.OnClickListener { dialog, which ->
                if (!checkedItems.isEmpty()) {
                    Log.d("checkedItem:", "" + checkedItems[0])
                    val baseref = FirebaseDatabase.getInstance().reference
                    val baseurl = "/shared-cheats/" + mGameCode + "/" + item.key + "/scores"
                    val scoresRef = baseref.child(baseurl)
                    scoresRef.limitToFirst(10).addValueEventListener(object : ValueEventListener {
                        override fun onDataChange(dataSnapshot: DataSnapshot) {
                            if (dataSnapshot.hasChildren()) {
                                var score = 0.0
                                var cnt = 0
                                for (child in dataSnapshot.children) {
                                    score += child.getValue(Double::class.java)!!
                                    cnt++
                                }
                                score = score / cnt.toDouble()
                                database_!!.child(item.key).child("star_count").setValue(score)
                            }
                        }

                        override fun onCancelled(databaseError: DatabaseError) {
                            // Getting Post failed, log a message
                            Log.w(TAG, "loadPost:onCancelled", databaseError.toException())
                            // ...
                        }
                    })
                    val auth = FirebaseAuth.getInstance()
                    if (auth.currentUser == null) {
                        return@OnClickListener
                    }
                    database_!!.child(item.key).child("scores").child(auth.currentUser!!.uid)
                        .setValue(
                            checkedItems[0] + 1)
                }
            })
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    companion object {
        private const val TAG = "CloudCheatItemFragment"
        private const val ARG_COLUMN_COUNT = "column-count"
        private const val ARG_GAME_ID = "game_id"
        @JvmStatic
        fun newInstance(gameid: String?, columnCount: Int): CloudCheatItemFragment {
            val fragment = CloudCheatItemFragment()
            val args = Bundle()
            args.putString(ARG_GAME_ID, gameid)
            args.putInt(ARG_COLUMN_COUNT, columnCount)
            fragment.arguments = args
            return fragment
        }
    }
}