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

import android.annotation.SuppressLint
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.RatingBar
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R

/**
 * [RecyclerView.Adapter] that can display a [CheatItem] and makes a call to the
 * specified [OnListFragmentInteractionListener].
 * TODO: Replace the implementation with code for your data type.
 */
class CloudCheatItemRecyclerViewAdapter(
    private val mValues: List<CheatItem?>?,
    private var mListener: OnItemClickListener?
) : RecyclerView.Adapter<CloudCheatItemRecyclerViewAdapter.ViewHolder>() {
    private var focusedItem = 0
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
                        mListener!!.onItemClick(focusedItem, holder!!.mItem, holder.mView)
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

    fun setOnItemClickListener(listener: OnItemClickListener?) {
        mListener = listener
    }

    interface OnItemClickListener {
        fun onItemClick(position: Int, item: CheatItem?, v: View?)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.fragment_cloudcheatitem, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, @SuppressLint("RecyclerView") position: Int) {
        holder.mItem = mValues?.get(position)
        holder.mIdView.text = mValues?.get(position)?.description
        holder.mContentView.text = mValues?.get(position)?.cheat_code
        holder.itemView.isSelected = focusedItem == position
        if (focusedItem == position) holder.itemView.setBackgroundColor(ContextCompat.getColor(
            holder.itemView.context,
            R.color.colorPrimaryDark)) else holder.itemView.setBackgroundColor(ContextCompat.getColor(
            holder.itemView.context,
            R.color.halfTransparent))
        holder.mcb.isChecked = holder.mItem!!.enable
        holder.mRate.rating = holder.mItem!!.star_count.toFloat()
        holder.mRateScore.text = String.format("%.1f", holder.mItem!!.star_count.toFloat())
        holder.mView.setOnClickListener {
            notifyItemChanged(focusedItem)
            focusedItem = position
            notifyItemChanged(focusedItem)
            if (null != mListener) {
                mListener!!.onItemClick(position, holder.mItem, holder.mView)
            }
        }
    }

    override fun getItemCount(): Int {
        return if (mValues == null) 0 else mValues.size
    }

    inner class ViewHolder(val mView: View) : RecyclerView.ViewHolder(
        mView) {
        val mIdView: TextView
        val mContentView: TextView
        var mcb: CheckBox
        var mItem: CheatItem? = null
        var mRate: RatingBar
        var mRateScore: TextView
        override fun toString(): String {
            return super.toString() + " '" + mContentView.text + "'"
        }

        init {
            mIdView = mView.findViewById<View>(R.id.id) as TextView
            mContentView = mView.findViewById<View>(R.id.content) as TextView
            mcb = mView.findViewById<View>(R.id.checkBox_enable) as CheckBox
            mcb.isEnabled = false
            mcb.isFocusable = false
            mRate = mView.findViewById<View>(R.id.ratingBar) as RatingBar
            mRateScore = mView.findViewById<View>(R.id.textView_rate) as TextView
        }
    }
}
