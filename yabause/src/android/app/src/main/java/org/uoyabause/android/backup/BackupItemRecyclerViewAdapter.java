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

package org.uoyabause.android.backup;

import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.uoyabause.uranus.R;

import java.util.List;

public class BackupItemRecyclerViewAdapter extends RecyclerView.Adapter<BackupItemRecyclerViewAdapter.ViewHolder> {

    private final List<BackupItem> mValues;
    //private final OnListFragmentInteractionListener mListener;
    int currentpage_ = 0;
    private int focusedItem = 0;

    private OnItemClickListener mListener;
    public void setOnItemClickListener(OnItemClickListener listener) {
        mListener = listener;
    }
    public static interface OnItemClickListener {
        public void onItemClick(int currentpage, int position, BackupItem backupitem, View v  );
    }

    public BackupItemRecyclerViewAdapter(int currentpage, List<BackupItem> items, OnItemClickListener listener) {
        mValues = items;
        mListener = listener;
        currentpage_ = currentpage;
    }

    @Override
    public void onAttachedToRecyclerView(final RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);

        // Handle key up and key down and attempt to move selection
        recyclerView.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                RecyclerView.LayoutManager lm = recyclerView.getLayoutManager();

                // Return false if scrolled to the bounds and allow focus to move off the list
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
                        return tryMoveSelection(lm, 1);
                    } else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
                        return tryMoveSelection(lm, -1);
                    }else if(keyCode == KeyEvent.KEYCODE_BUTTON_A){
                        if( mListener != null) {
                            ViewHolder holder = (ViewHolder)recyclerView.findViewHolderForAdapterPosition(focusedItem);
                            mListener.onItemClick(currentpage_, focusedItem, holder.mItem, holder.mView );
                        }
                    }
                }

                return false;
            }
        });
    }

    private boolean tryMoveSelection(RecyclerView.LayoutManager lm, int direction) {
        int tryFocusItem = focusedItem + direction;

        // If still within valid bounds, move the selection, notify to redraw, and scroll
        if (tryFocusItem >= 0 && tryFocusItem < getItemCount()) {
            notifyItemChanged(focusedItem);
            focusedItem = tryFocusItem;
            notifyItemChanged(focusedItem);
            lm.scrollToPosition(focusedItem);
            return true;
        }

        return false;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.fragment_backupitem, parent, false);
        return new ViewHolder(view);
    }

    final String DATE_PATTERN ="yyyy/MM/dd HH:mm";

    @Override
    public void onBindViewHolder(final ViewHolder holder, int position) {
        holder.mItem = mValues.get(position);
        holder.mNameView.setText(mValues.get(position).filename);
        holder.tvComment.setText(mValues.get(position).comment);
        holder.mSizeView.setText(String.format("%,dByte",mValues.get(position).datasize));
        //holder.mDateView.setText(new SimpleDateFormat(DATE_PATTERN).format(mValues.get(position)._savedate));
        holder.mDateView.setText(mValues.get(position).savedate);
        final int position_ = position;

        holder.itemView.setSelected(focusedItem == position);

        if(focusedItem == position)
            holder.itemView.setBackgroundColor(ContextCompat.getColor(holder.itemView.getContext(),R.color.colorPrimaryDark));
        else
            holder.itemView.setBackgroundColor(ContextCompat.getColor(holder.itemView.getContext(),R.color.halfTransparent));

        holder.mView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                    notifyItemChanged(focusedItem);
                    focusedItem = position_;
                    notifyItemChanged(focusedItem);
                    if (null != mListener) {
                    // Notify the active callbacks interface (the activity, if the
                    // fragment is attached to one) that an item has been selected.
                    //mListener.onListFragmentInteraction(holder.mItem);
                    mListener.onItemClick(currentpage_, position_, holder.mItem, holder.mView );
                }
            }
        });

    }

    @Override
    public int getItemCount() {
        return mValues.size();
    }

    public class ViewHolder extends RecyclerView.ViewHolder {
        public final View mView;
        public final TextView mNameView;
        public final TextView tvComment;
        public final TextView mSizeView;
        public final TextView mDateView;
        public BackupItem mItem;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mNameView = (TextView) view.findViewById(R.id.tvName);
            tvComment = (TextView) view.findViewById(R.id.tvComment);
            mSizeView = (TextView) view.findViewById(R.id.tvSize);
            mDateView = (TextView) view.findViewById(R.id.tvDate);

            // Handle item click and set the selection
            itemView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // Redraw the old selection and the new
                    notifyItemChanged(focusedItem);
                    focusedItem = getLayoutPosition();
                    notifyItemChanged(focusedItem);
                }
            });

        }

        @Override
        public String toString() {
            return super.toString() + " '" + mSizeView.getText() + "'" + mDateView.getText() + "'" ;
        }
    }


}
