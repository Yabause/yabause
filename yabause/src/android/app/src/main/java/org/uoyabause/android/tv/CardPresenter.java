/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
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

package org.uoyabause.android.tv;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.os.Build;
import androidx.leanback.widget.ImageCardView;
import androidx.leanback.widget.Presenter;

import android.view.ViewGroup;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.request.RequestOptions;

import org.uoyabause.android.GameInfo;
import org.uoyabause.uranus.R;

import java.io.File;

/*
 * A CardPresenter is used to generate Views and bind Objects to them on demand. 
 * It contains an Image CardView
 */
public class CardPresenter extends Presenter {
    private static final String TAG = "CardPresenter";

    private static int CARD_WIDTH = 320;
    private static int CARD_HEIGHT = 224;
    private static int sSelectedBackgroundColor;
    private static int sDefaultBackgroundColor;
    private Drawable mDefaultCardImage;

    private static void updateCardBackgroundColor(ImageCardView view, boolean selected) {
        int color = selected ? sSelectedBackgroundColor : sDefaultBackgroundColor;
        // Both background colors should be set because the view's background is temporarily visible
        // during animations.
        view.setBackgroundColor(color);
        view.findViewById(R.id.info_field).setBackgroundColor(color);
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {
        sDefaultBackgroundColor = parent.getResources().getColor(R.color.default_background);
        sSelectedBackgroundColor = parent.getResources().getColor(R.color.selected_background);
        mDefaultCardImage = parent.getResources().getDrawable(R.drawable.missing);

        ImageCardView cardView = new ImageCardView(parent.getContext()) {
            @Override
            public void setSelected(boolean selected) {
                updateCardBackgroundColor(this, selected);
                super.setSelected(selected);
            }
        };

        cardView.setFocusable(true);
        cardView.setFocusableInTouchMode(true);
        updateCardBackgroundColor(cardView, false);
        return new ViewHolder(cardView);
    }

    @Override
    public void onBindViewHolder(Presenter.ViewHolder viewHolder, Object item) {
        GameInfo game = (GameInfo) item;
        ImageCardView cardView = (ImageCardView) viewHolder.view;
        cardView.setTitleText(game.game_title);
        String rate="";
        for( int i=0; i < game.rating; i++ ){
            rate += "★";
        }
        if( game.device_infomation.equals("CD-1/1")){

        }else {
            rate += " " + game.device_infomation;
        }

        cardView.setContentText(rate);
        cardView.setMainImageDimensions(CARD_WIDTH, CARD_HEIGHT);
        Activity activity = (Activity)viewHolder.view.getContext();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 && activity.isDestroyed()) {
            return;
        }
        if( !game.image_url.equals("")) {
            if( game.image_url.startsWith("http")){
                Glide.with(viewHolder.view.getContext())
                        .load(game.image_url)
                        .apply(new RequestOptions().transforms(new CenterCrop() ).error(mDefaultCardImage))
                        .into(cardView.getMainImageView());

            }else {
                Glide.with(viewHolder.view.getContext())
                        .load(new File(game.image_url))
                        .apply(  new RequestOptions().transforms(new CenterCrop() ).error(mDefaultCardImage)  )
                        .into(cardView.getMainImageView());
            }
        }else{
            cardView.getMainImageView().setImageDrawable(mDefaultCardImage);
        }
    }

    @Override
    public void onUnbindViewHolder(Presenter.ViewHolder viewHolder) {
        ImageCardView cardView = (ImageCardView) viewHolder.view;
        // Remove references to images so that the garbage collector can free up memory
        cardView.setBadgeImage(null);
        cardView.setMainImage(null);
    }
}
