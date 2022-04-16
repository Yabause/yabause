package org.uoyabause.android.phone

import android.content.res.Configuration
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.DefaultItemAnimator
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.R

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
class GameListFragment : Fragment() {

    lateinit var recyclerView: RecyclerView
    var rootview_: View? = null
    var gameList: GameItemAdapter? = null
    var index: String? = null

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        // Checks the orientation of the screen
        if (newConfig.orientation === Configuration.ORIENTATION_LANDSCAPE) {
            recyclerView.layoutManager = GridLayoutManager(activity, 2)
        } else if (newConfig.orientation === Configuration.ORIENTATION_PORTRAIT) {
            recyclerView.layoutManager =
                    LinearLayoutManager(activity, LinearLayoutManager.VERTICAL, false)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {

        if (savedInstanceState != null) {
            index = savedInstanceState.getString("index")
            if (index != null) {
                gameList = GameSelectFragmentPhone.getInstance()!!.getGameItemAdapter(index!!)
            }
        }

        rootview_ = inflater.inflate(R.layout.content_game_select_list_phone, container, false)
        if (rootview_ != null) {
            recyclerView = rootview_!!.findViewById<View>(R.id.my_recycler_view) as RecyclerView
            recyclerView.setHasFixedSize(true)
            recyclerView.itemAnimator = DefaultItemAnimator()

            if (activity?.resources?.configuration?.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                recyclerView.layoutManager = GridLayoutManager(activity, 2)
            } else {
                recyclerView.layoutManager =
                        LinearLayoutManager(activity, LinearLayoutManager.VERTICAL, false)
            }
            recyclerView.adapter = gameList
        }
        return rootview_
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        if (index != null) {
            outState.putString("index", index)
        }
    }

    companion object {
        const val TAG = "BackupItemFragment"
        private const val RC_SIGN_IN = 1232
        @JvmStatic
        fun getInstance(position: Int, index: String, gameList: GameItemAdapter): GameListFragment {
            val f = GameListFragment()
            f.gameList = gameList
            f.index = index
            val args = Bundle()
            args.putInt("position", position)
            f.arguments = args
            gameList.notifyDataSetChanged()
            return f
        }
    }
}
