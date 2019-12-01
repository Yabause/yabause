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
package org.uoyabause.android.phone

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import org.uoyabause.android.GameInfo
import org.uoyabause.android.phone.GameItemAdapter.GameViewHolder
import org.uoyabause.uranus.R
import java.io.File

class GameItemAdapter(private val dataSet: List<GameInfo?>? ) :
    RecyclerView.Adapter<GameViewHolder>() {

    class GameViewHolder(var rootview: View) :
        RecyclerView.ViewHolder(rootview) {
        var textViewName: TextView
        var textViewVersion: TextView
        var imageViewIcon: ImageView

        init {
            textViewName =
                rootview.findViewById<View>(R.id.textViewName) as TextView
            textViewVersion =
                rootview.findViewById<View>(R.id.textViewVersion) as TextView
            imageViewIcon =
                rootview.findViewById<View>(R.id.imageView) as ImageView
            /*
            ViewGroup.LayoutParams lp = imageViewIcon.getLayoutParams();
            lp.width = CARD_WIDTH;
            lp.height = CARD_HEIGHT;
            imageViewIcon.setLayoutParams(lp);
            */
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): GameViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.cards_layout, parent, false)
        view.setOnClickListener(GameSelectFragmentPhone.myOnClickListener)
        return GameViewHolder(view)
    }

    private var mListener: OnItemClickListener? = null
    fun setOnItemClickListener(listener: OnItemClickListener?) {
        mListener = listener
    }

    interface OnItemClickListener {
        fun onItemClick(position: Int, item: GameInfo?, v: View?)
    }

    override fun onBindViewHolder(holder: GameViewHolder, position: Int) {
        val textViewName = holder.textViewName
        val textViewVersion = holder.textViewVersion
        val imageView = holder.imageViewIcon
        val ctx = holder.rootview.context
        val game = dataSet?.get(position)
        if( game != null ) {
            textViewName.text = game.game_title
            //textViewVersion.setText(game.product_number);
            var rate = ""
            for (i in 0 until game.rating) {
                rate += "★"
            }
            if (game.device_infomation == "CD-1/1") {
            } else {
                rate += " " + game.device_infomation
            }
            textViewVersion.text = rate
            if (game.image_url != "") { //try {
                if (game.image_url.startsWith("http")) {
                    Glide.with(ctx)
                        .load(game.image_url) //.apply(new RequestOptions().transforms(new CenterCrop() ))
                        .into(imageView)
                } else {
                    Glide.with(holder.rootview.context)
                        .load(File(game.image_url)) //.apply(new RequestOptions().transforms(new CenterCrop() ))
//.error(mDefaultCardImage)
                        .into(imageView)
                }
            } else {
            }
            holder.rootview.setOnClickListener {
                //notifyItemChanged(focusedItem);
//focusedItem = position_;
//notifyItemChanged(focusedItem);
                if (null != mListener) {
                    mListener!!.onItemClick(position, dataSet?.get(position), null)
                }

        }
        }
    }

    override fun getItemCount(): Int {
        return dataSet!!.size
    }

    companion object {
        private const val CARD_WIDTH = 320
        private const val CARD_HEIGHT = 224
    }
}