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

import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R

class BackupItemRecyclerViewAdapter(
    currentpage: Int,
    private val mValues: List<BackupItem>,
    private var mListener: OnItemClickListener?
) : RecyclerView.Adapter<BackupItemRecyclerViewAdapter.ViewHolder>() {
    // private final OnListFragmentInteractionListener mListener;
    var currentpage_ = 0
    private var focusedItem = 0
    fun setOnItemClickListener(listener: OnItemClickListener?) {
        mListener = listener
    }

    interface OnItemClickListener {
        fun onItemClick(currentpage: Int, position: Int, backupitem: BackupItem?, v: View?)
    }

    override fun onAttachedToRecyclerView(recyclerView: RecyclerView) {
        super.onAttachedToRecyclerView(recyclerView)

        // Handle key up and key down and attempt to move selection
        recyclerView.setOnKeyListener(View.OnKeyListener { _, keyCode, event ->
            val lm = recyclerView.layoutManager

            // Return false if scrolled to the bounds and allow focus to move off the list
            if (event.action == KeyEvent.ACTION_DOWN) {
                if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
                    return@OnKeyListener tryMoveSelection(lm, 1)
                } else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
                    return@OnKeyListener tryMoveSelection(lm, -1)
                } else if (keyCode == KeyEvent.KEYCODE_BUTTON_A) {
                    if (mListener != null) {
                        val holder =
                            recyclerView.findViewHolderForAdapterPosition(focusedItem) as ViewHolder?
                        mListener!!.onItemClick(currentpage_,
                            focusedItem,
                            holder!!.mItem,
                            holder.mView)
                    }
                }
            }
            false
        })
    }

    private fun tryMoveSelection(lm: RecyclerView.LayoutManager?, direction: Int): Boolean {
        val tryFocusItem = focusedItem + direction

        // If still within valid bounds, move the selection, notify to redraw, and scroll
        if (tryFocusItem >= 0 && tryFocusItem < itemCount) {
            notifyItemChanged(focusedItem)
            focusedItem = tryFocusItem
            notifyItemChanged(focusedItem)
            lm!!.scrollToPosition(focusedItem)
            return true
        }
        return false
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.fragment_backupitem, parent, false)
        return ViewHolder(view)
    }

    val DATE_PATTERN = "yyyy/MM/dd HH:mm"
    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.mItem = mValues[position]
        holder.mNameView.text = mValues[position].filename
        holder.tvComment.text = mValues[position].comment
        holder.mSizeView.text = String.format("%,dByte", mValues[position].datasize)
        // holder.mDateView.setText(new SimpleDateFormat(DATE_PATTERN).format(mValues.get(position)._savedate));
        holder.mDateView.text = mValues[position].savedate
        holder.itemView.isSelected = focusedItem == position
        if (focusedItem == position) holder.itemView.setBackgroundColor(ContextCompat.getColor(
            holder.itemView.context,
            R.color.colorPrimaryDark)) else holder.itemView.setBackgroundColor(ContextCompat.getColor(
            holder.itemView.context,
            R.color.halfTransparent))
        holder.mView.setOnClickListener {
            notifyItemChanged(focusedItem)
            focusedItem = position
            notifyItemChanged(focusedItem)
            if (null != mListener) {
                // Notify the active callbacks interface (the activity, if the
                // fragment is attached to one) that an item has been selected.
                // mListener.onListFragmentInteraction(holder.mItem);
                mListener!!.onItemClick(currentpage_, position, holder.mItem, holder.mView)
            }
        }
    }

    override fun getItemCount(): Int {
        return mValues.size
    }

    inner class ViewHolder(val mView: View) : RecyclerView.ViewHolder(
        mView) {
        val mNameView: TextView
        val tvComment: TextView
        val mSizeView: TextView
        val mDateView: TextView
        var mItem: BackupItem? = null
        override fun toString(): String {
            return super.toString() + " '" + mSizeView.text + "'" + mDateView.text + "'"
        }

        init {
            mNameView = mView.findViewById<View>(R.id.tvName) as TextView
            tvComment = mView.findViewById<View>(R.id.tvComment) as TextView
            mSizeView = mView.findViewById<View>(R.id.tvSize) as TextView
            mDateView = mView.findViewById<View>(R.id.tvDate) as TextView

            // Handle item click and set the selection
            itemView.setOnClickListener { // Redraw the old selection and the new
                notifyItemChanged(focusedItem)
                focusedItem = layoutPosition
                notifyItemChanged(focusedItem)
            }
        }
    }

    init {
        currentpage_ = currentpage
    }
}
