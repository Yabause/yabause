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

package org.uoyabause.android.cheat;

import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.TextView;





import java.util.List;
import org.uoyabause.uranus.R;


public class LocalCheatItemRecyclerViewAdapter extends RecyclerView.Adapter<LocalCheatItemRecyclerViewAdapter.ViewHolder> {

    private final List<CheatItem> mValues;
    private int focusedItem = 0;

    public LocalCheatItemRecyclerViewAdapter(List<CheatItem> items, OnItemClickListener listener) {
        mValues = items;
        mListener = listener;
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
                            LocalCheatItemRecyclerViewAdapter.ViewHolder holder = (LocalCheatItemRecyclerViewAdapter.ViewHolder)recyclerView.findViewHolderForAdapterPosition(focusedItem);
                            mListener.onItemClick( focusedItem, holder.mItem, holder.mView );
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

    private OnItemClickListener mListener;
    public void setOnItemClickListener(OnItemClickListener listener) {
        mListener = listener;
    }
    public interface OnItemClickListener {
        void onItemClick( int position, CheatItem item, View v  );
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.fragment_localcheatitem, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(final ViewHolder holder, int position) {
        holder.mItem = mValues.get(position);
        holder.mIdView.setText(mValues.get(position).getDescription());
        holder.mContentView.setText(mValues.get(position).getCheatCode());


        final int position_ = position;
        holder.itemView.setSelected(focusedItem == position);

        if(focusedItem == position)
            holder.itemView.setBackgroundColor(ContextCompat.getColor(holder.itemView.getContext(),R.color.colorPrimaryDark));
        else
            holder.itemView.setBackgroundColor(ContextCompat.getColor(holder.itemView.getContext(),R.color.halfTransparent));

        if( holder.mItem.getEnable() ){
            holder.mcb.setChecked(true);
        }else{
            holder.mcb.setChecked(false);
        }

        holder.mView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                notifyItemChanged(focusedItem);
                focusedItem = position_;
                notifyItemChanged(focusedItem);
                if (null != mListener) {
                    mListener.onItemClick(position_, holder.mItem, holder.mView );
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
        public final TextView mIdView;
        public final TextView mContentView;
        public CheckBox mcb;
        public CheatItem mItem;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mIdView = (TextView) view.findViewById(R.id.id);
            mContentView = (TextView) view.findViewById(R.id.content);
            mcb = (CheckBox)view.findViewById(R.id.checkBox_enable);
            mcb.setEnabled(false);
            mcb.setFocusable(false);
        }

        @Override
        public String toString() {
            return super.toString() + " '" + mContentView.getText() + "'";
        }
    }




}
