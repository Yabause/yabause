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

import android.app.Activity
import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.View
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.tv.RecyclerGameListAdapter

class YabauseGameSelectActivity : Activity() {
    private var mRecyclerView: RecyclerView? = null
    private var mAdapter: RecyclerView.Adapter<*>? = null
    private var mLayoutManager: RecyclerView.LayoutManager? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this)
        val lock_landscape = sharedPref.getBoolean("pref_landscape", false)
        requestedOrientation = if (lock_landscape == true) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
        }
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_yabause_game_select)
        mRecyclerView = findViewById<View>(R.id.game_recycler_view) as RecyclerView

        // use this setting to improve performance if you know that changes
        // in content do not change the layout size of the RecyclerView
        mRecyclerView!!.setHasFixedSize(true)

        // use a linear layout manager
        mLayoutManager = LinearLayoutManager(this, LinearLayoutManager.HORIZONTAL, false)
        mRecyclerView!!.layoutManager = mLayoutManager

        // specify an adapter (see also next example)
        mAdapter = RecyclerGameListAdapter()
        mRecyclerView!!.adapter = mAdapter
    }
}
