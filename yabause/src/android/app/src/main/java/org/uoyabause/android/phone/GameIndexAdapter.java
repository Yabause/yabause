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

package org.uoyabause.android.phone;

import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.uoyabause.uranus.R;

public class GameIndexAdapter extends RecyclerView.Adapter<GameIndexAdapter.GameIndexViewHolder> {

    String index_title_;
    public GameIndexAdapter( String index_title) {
        index_title_ = index_title;
    }

    public static class GameIndexViewHolder extends RecyclerView.ViewHolder {
        TextView textViewName;
        View rootview;
        public GameIndexViewHolder(View itemView) {
            super(itemView);
            rootview = itemView;
            this.textViewName = (TextView) itemView.findViewById(R.id.text_index);
        }
    }

    @Override
    public GameIndexViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.game_item_index, parent, false);

        view.setOnClickListener(GameSelectFragmentPhone.myOnClickListener);

        GameIndexViewHolder myViewHolder = new GameIndexViewHolder(view);
        return myViewHolder;
    }

    @Override
    public void onBindViewHolder(GameIndexViewHolder holder, int position) {
        TextView textViewName = holder.textViewName;
        textViewName.setText(index_title_);
    }

    @Override
    public int getItemCount() {
        return 1;
    }
}
