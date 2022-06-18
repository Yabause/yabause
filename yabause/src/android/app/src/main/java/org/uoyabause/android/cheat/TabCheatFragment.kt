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
package org.uoyabause.android.cheat

import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentStatePagerAdapter
import androidx.viewpager.widget.ViewPager
import com.google.android.material.tabs.TabLayout
import java.util.ArrayList
import java.util.HashSet
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.Yabause
import org.uoyabause.android.cheat.CloudCheatItemFragment.Companion.newInstance
import org.uoyabause.android.cheat.LocalCheatItemFragment.Companion.newInstance

internal class ViewPagerAdapter(fm: FragmentManager?) : FragmentStatePagerAdapter(
    fm!!) {
    var gameid_: String? = null
    fun setGmaeid(gameid: String?) {
        gameid_ = gameid
    }

    override fun getItem(position: Int): Fragment {
        when (position) {
            PAGE_LOCAL -> return LocalCheatItemFragment.newInstance(gameid_, 1)
            PAGE_CLOUD -> return CloudCheatItemFragment.newInstance(gameid_, 1)
        }
        return Fragment()
    }

    override fun getCount(): Int {
        return 2
    }

    override fun getPageTitle(position: Int): CharSequence? {
        when (position) {
            PAGE_LOCAL -> return "Local"
            PAGE_CLOUD -> return "Shared"
        }
        return ""
    }

    companion object {
        const val PAGE_LOCAL = 0
        const val PAGE_CLOUD = 1
    }
}

class TabCheatFragment : Fragment() {
    private var mGameid: String? = null
    private var mListener: OnFragmentInteractionListener? = null
    lateinit var active_cheats_: ArrayList<String>
    var mainv_: View? = null
    var tablayout_: TabLayout? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        active_cheats_ = ArrayList()
        mGameid = requireArguments().getString(ARG_GAME_ID)
        val current_cheat_code = requireArguments().getStringArray(ARG_CURRENT_CHEAT)
        if (current_cheat_code != null) {
            for (i in current_cheat_code.indices) {
                active_cheats_.add(current_cheat_code[i])
            }
        }
    }

    fun AddActiveCheat(v: String) {
        active_cheats_.add(v)
        val set: MutableSet<String> = HashSet()
        set.addAll(active_cheats_)
        active_cheats_.addAll(set)
    }

    fun RemoveActiveCheat(v: String) {
        active_cheats_.remove(v)
        active_cheats_.remove(v)
    }

    fun isActive(v: String): Boolean {
        for (string in active_cheats_) {
            if (string == v) {
                return true
            }
        }
        return false
    }

    fun sendCheatListToYabause() {
        val cntChoice = active_cheats_.size
        if (cntChoice > 0) {
            val cheatitem = arrayOfNulls<String>(cntChoice)
            var cindex = 0
            for (i in 0 until cntChoice) {
                cheatitem[cindex] = active_cheats_[cindex]
                cindex++
            }
            val activity = activity as Yabause?
            activity!!.updateCheatCode(cheatitem)
        } else {
            val activity = activity as Yabause?
            activity!!.updateCheatCode(null)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        mainv_ = inflater.inflate(R.layout.fragment_tab_cheat, container, false)
        if (mainv_ == null) {
            return null
        }

        tablayout_ = mainv_?.findViewById<View>(R.id.tab_cheat_source) as TabLayout
        val viewPager_ = mainv_?.findViewById<View>(R.id.view_pager_cheat) as ViewPager
        val adapter = ViewPagerAdapter(requireActivity().supportFragmentManager)
        adapter.setGmaeid(mGameid)
        viewPager_.adapter = adapter
        tablayout_!!.setupWithViewPager(viewPager_)
        return mainv_
    }

    fun onButtonPressed(uri: Uri?) {
        if (mListener != null) {
            mListener!!.onFragmentInteraction(uri)
        }
    }

    fun enableFullScreen() {
        val decorView = requireActivity().findViewById<View>(R.id.drawer_layout)
        if (decorView != null) {
            decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    or View.SYSTEM_UI_FLAG_FULLSCREEN
                    or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)
        }
    }

    fun disableFullScreen() {
        val decorView = requireActivity().findViewById<View>(R.id.drawer_layout)
        if (decorView != null) {
            decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
        }
    }

    override fun onAttach(context: Context) {
        // disableFullScreen();
        super.onAttach(context)
        if (context is OnFragmentInteractionListener) {
            mListener = context
        } else {
        }
    }

    override fun onDetach() {
        // enableFullScreen();
        sendCheatListToYabause()
        super.onDetach()
        mListener = null
    }

    interface OnFragmentInteractionListener {
        fun onFragmentInteraction(uri: Uri?)
    }

    companion object {
        const val TAG = "TabBackupFragment"
        private const val ARG_GAME_ID = "gameid"
        private const val ARG_CURRENT_CHEAT = "current_cheats"
        fun newInstance(gameid: String?, current_cheat_code: Array<String?>?): TabCheatFragment {
            val fragment = TabCheatFragment()
            val args = Bundle()
            args.putString(ARG_GAME_ID, gameid)
            args.putStringArray(ARG_CURRENT_CHEAT, current_cheat_code)
            fragment.arguments = args
            return fragment
        }
    }
}
