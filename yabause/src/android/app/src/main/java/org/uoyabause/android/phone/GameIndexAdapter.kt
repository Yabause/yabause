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
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.phone.GameIndexAdapter.GameIndexViewHolder

class GameIndexAdapter(var index_title_: String) :
    RecyclerView.Adapter<GameIndexViewHolder>() {

    class GameIndexViewHolder(var rootview: View) :
        RecyclerView.ViewHolder(rootview) {
        var textViewName: TextView

        init {
            textViewName =
                rootview.findViewById<View>(R.id.text_index) as TextView
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): GameIndexViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.game_item_index, parent, false)
        view.setOnClickListener(GameSelectFragmentPhone.myOnClickListener)
        return GameIndexViewHolder(view)
    }

    override fun onBindViewHolder(holder: GameIndexViewHolder, position: Int) {
        val textViewName = holder.textViewName
        textViewName.text = index_title_
    }

    override fun getItemCount(): Int {
        return 1
    }
}
