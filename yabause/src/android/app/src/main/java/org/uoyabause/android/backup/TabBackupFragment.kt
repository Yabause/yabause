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
package org.uoyabause.android.backup

import android.content.Context
import android.net.Uri
import org.uoyabause.android.backup.BackupItemFragment.Companion.getInstance
import androidx.fragment.app.FragmentStatePagerAdapter
import org.uoyabause.android.backup.BackupDevice
import org.uoyabause.android.backup.BackupItemFragment
import com.google.android.material.tabs.TabLayout
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.YabauseRunnable
import org.json.JSONObject
import org.json.JSONArray
import org.json.JSONException
import org.uoyabause.android.backup.TabBackupFragment
import androidx.viewpager.widget.ViewPager
import java.util.ArrayList

internal class ViewPagerAdapter(fm: FragmentManager?) : FragmentStatePagerAdapter(
    fm!!) {
    private var backup_devices_: List<BackupDevice>? = null
    fun setDevices(devs: List<BackupDevice>?) {
        backup_devices_ = devs
    }

    override fun getItem(position: Int): Fragment {
        return getInstance(position)
    }

    override fun getCount(): Int {
        return backup_devices_!!.size
    }

    override fun getPageTitle(position: Int): CharSequence? {
        return backup_devices_!![position].name_
    }
}

class TabBackupFragment : Fragment() {
    private var backup_devices_: MutableList<BackupDevice> = ArrayList()
    lateinit var mainv_: View
    lateinit var tablayout_: TabLayout
    private var mListener: OnFragmentInteractionListener? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (arguments != null) {
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        mainv_ = inflater.inflate(R.layout.fragment_tab_backup, container, false)
        val jsonstr: String?
        jsonstr = YabauseRunnable.getDevicelist()
        backup_devices_ = ArrayList()
        try {
            val json = JSONObject(jsonstr)
            val array = json.getJSONArray("devices")
            for (i in 0 until array.length()) {
                val data = array.getJSONObject(i)
                val tmp = BackupDevice()
                tmp.name_ = data.getString("name")
                tmp.id_ = data.getInt("id")
                backup_devices_.add(tmp)
            }
            val tmp = BackupDevice()
            tmp.name_ = "cloud"
            tmp.id_ = BackupDevice.DEVICE_CLOUD
            backup_devices_.add(tmp)
        } catch (e: JSONException) {
            Log.e(TAG, "Fail to convert to json", e)
        }
        if (backup_devices_.size == 0) {
            Log.e(TAG, "Can't find device")
        }
        tablayout_ = mainv_.findViewById<View>(R.id.tab_devices) as TabLayout
        val viewPager_ = mainv_.findViewById<View>(R.id.view_pager_backup) as ViewPager
        val adapter = ViewPagerAdapter(
            requireActivity().supportFragmentManager)
        adapter.setDevices(backup_devices_)
        viewPager_.adapter = adapter
        tablayout_!!.setupWithViewPager(viewPager_)
        return mainv_
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

    override fun onDestroyView() {
        tablayout_!!.setupWithViewPager(null)
        super.onDestroyView()
    }

    fun onButtonPressed(uri: Uri?) {
        if (mListener != null) {
            mListener!!.onFragmentInteraction(uri)
        }
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        //        disableFullScreen();
/*
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
*/
    }

    override fun onDetach() {
        //enableFullScreen();
        super.onDetach()
        mListener = null
    }

    interface OnFragmentInteractionListener {
        fun onFragmentInteraction(uri: Uri?)
    }

    companion object {
        const val TAG = "TabBackupFragment"
        fun newInstance(param1: String?, param2: String?): TabBackupFragment {
            val fragment = TabBackupFragment()
            val args = Bundle()
            fragment.arguments = args
            return fragment
        }
    }
}