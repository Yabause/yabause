/*  Copyright 2015 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
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
package org.uoyabause.android

import android.content.Context
import android.view.ViewGroup
import android.widget.TextView
import android.view.LayoutInflater
import org.devmiyax.yabasanshiro.R
import android.database.DataSetObserver
import android.view.View
import android.widget.ListAdapter

internal class GameListAdapter(ctx: Context) : ListAdapter {
    private val storage: YabauseStorage
    private val gamefiles: Array<String?>
    private val context: Context
    override fun getCount(): Int {
        return gamefiles.size
    }

    override fun getItem(position: Int): String? {
        return gamefiles[position]
    }

    override fun getItemId(position: Int): Long {
        return position.toLong()
    }

    override fun getItemViewType(position: Int): Int {
        return 0
    }

    override fun getView(position: Int, convertView: View, parent: ViewGroup): View {
        val tv: TextView
        tv = if (convertView != null) {
            convertView as TextView
        } else {
            val inflater =
                context.getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
            inflater.inflate(R.layout.game_item, parent, false) as TextView
        }
        tv.text = gamefiles[position]
        return tv
    }

    override fun getViewTypeCount(): Int {
        return 1
    }

    override fun hasStableIds(): Boolean {
        return false
    }

    override fun isEmpty(): Boolean {
        return count == 0
    }

    override fun registerDataSetObserver(observer: DataSetObserver) {}
    override fun unregisterDataSetObserver(observer: DataSetObserver) {}
    override fun areAllItemsEnabled(): Boolean {
        return true
    }

    override fun isEnabled(position: Int): Boolean {
        return true
    }

    init {
        storage = YabauseStorage.storage
        val res = ctx.resources
        gamefiles = storage.getGameFiles(res.getString(R.string.select_from_other_directory))
        context = ctx
    }
}