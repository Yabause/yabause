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

import android.content.Context;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.request.RequestOptions;

import org.uoyabause.android.GameInfo;
import org.uoyabause.uranus.R;

import java.io.File;
import java.util.List;

public class GameItemAdapter extends RecyclerView.Adapter<GameItemAdapter.GameViewHolder> {

    private List<GameInfo> dataSet;
    private static int CARD_WIDTH = 320;
    private static int CARD_HEIGHT = 224;

    public static class GameViewHolder extends RecyclerView.ViewHolder {

        public View rootview;
        TextView textViewName;
        TextView textViewVersion;
        ImageView imageViewIcon;

        public GameViewHolder(View itemView) {
            super(itemView);
            rootview = itemView;
            this.textViewName = (TextView) itemView.findViewById(R.id.textViewName);
            this.textViewVersion = (TextView) itemView.findViewById(R.id.textViewVersion);
            this.imageViewIcon = (ImageView) itemView.findViewById(R.id.imageView);
            /*
            ViewGroup.LayoutParams lp = imageViewIcon.getLayoutParams();
            lp.width = CARD_WIDTH;
            lp.height = CARD_HEIGHT;
            imageViewIcon.setLayoutParams(lp);
            */
        }
    }
    public GameItemAdapter(List<GameInfo> data) {
        this.dataSet = data;
    }
    @Override
    public GameViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.cards_layout, parent, false);

        view.setOnClickListener(GameSelectFragmentPhone.myOnClickListener);

        GameViewHolder myViewHolder = new GameViewHolder(view);
        return myViewHolder;
    }

    private GameItemAdapter.OnItemClickListener mListener;
    public void setOnItemClickListener(GameItemAdapter.OnItemClickListener listener) {
        mListener = listener;
    }
    public  interface OnItemClickListener {
        void onItemClick(int position, GameInfo item, View v  );
    }

    @Override
    public void onBindViewHolder(GameViewHolder holder, int position) {
        TextView textViewName = holder.textViewName;
        TextView textViewVersion = holder.textViewVersion;
        ImageView imageView = holder.imageViewIcon;
        final Context ctx = holder.rootview.getContext();
        final int position_ = position;

        GameInfo game = dataSet.get(position);

        textViewName.setText(game.game_title);
        //textViewVersion.setText(game.product_number);

        String rate="";
        for( int i=0; i < game.rating; i++ ){
            rate += "★";
        }
        if( game.device_infomation.equals("CD-1/1")){

        }else {
            rate += " " + game.device_infomation;
        }
        textViewVersion.setText(rate);

        if( !game.image_url.equals("")) {
            //try {
                if (game.image_url.startsWith("http")) {
                    Glide.with(ctx)
                            .load(game.image_url)
                            .apply(new RequestOptions().transforms(new CenterCrop() ))
                            .into(imageView);
                   } else {
                    Glide.with(holder.rootview.getContext())
                            .load(new File(game.image_url))
                            .apply(new RequestOptions().transforms(new CenterCrop() ))
                            //.error(mDefaultCardImage)
                            .into(imageView);
                }
        }else{

        }

        holder.rootview.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //notifyItemChanged(focusedItem);
                //focusedItem = position_;
                //notifyItemChanged(focusedItem);
                if (null != mListener) {
                    mListener.onItemClick(position_, dataSet.get(position_), null );
                }
            }
        });
    }

    @Override
    public int getItemCount() {
        return dataSet.size();
    }

}
