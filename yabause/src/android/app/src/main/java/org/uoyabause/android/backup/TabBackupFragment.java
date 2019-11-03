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

package org.uoyabause.android.backup;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import com.google.android.material.tabs.TabLayout;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentStatePagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.uranus.R;
import org.uoyabause.android.YabauseRunnable;

import java.util.ArrayList;
import java.util.List;


class ViewPagerAdapter extends FragmentStatePagerAdapter {

    private List<BackupDevice> backup_devices_;

    public void setDevices( List<BackupDevice> devs ){
        backup_devices_ = devs;
    }

    public ViewPagerAdapter(FragmentManager fm) {
        super(fm);
    }

    @Override
    public Fragment getItem(int position) {
        return BackupItemFragment.getInstance(position);
    }

    @Override
    public int getCount() {
        return backup_devices_.size();
    }

    @Override
    public CharSequence getPageTitle(int position) {
        return backup_devices_.get(position).name_;
    }

}

public class TabBackupFragment extends Fragment  {

    public static final String TAG = "TabBackupFragment";
    private List<BackupDevice> backup_devices_;

    View mainv_;
    TabLayout tablayout_;

    private OnFragmentInteractionListener mListener;

    public TabBackupFragment() {
        // Required empty public constructor
    }


    public static TabBackupFragment newInstance(String param1, String param2) {
        TabBackupFragment fragment = new TabBackupFragment();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        mainv_ = inflater.inflate(R.layout.fragment_tab_backup, container, false);

        String jsonstr;
        jsonstr = YabauseRunnable.getDevicelist();
        backup_devices_ = new ArrayList<BackupDevice>();
        try {

            JSONObject json = new JSONObject(jsonstr);
            JSONArray array = json.getJSONArray("devices");
            for (int i = 0; i < array.length(); i++) {
                JSONObject data = array.getJSONObject(i);
                BackupDevice tmp = new BackupDevice();
                tmp.name_ = data.getString("name");
                tmp.id_= data.getInt("id");
                backup_devices_.add(tmp);
            }

            BackupDevice tmp = new BackupDevice();
            tmp.name_ = "cloud";
            tmp.id_= BackupDevice.DEVICE_CLOUD;
            backup_devices_.add(tmp);

        }catch(JSONException e){
            Log.e(TAG, "Fail to convert to json", e);
        }

        if( backup_devices_.size() == 0 ){
            Log.e(TAG, "Can't find device");
        }

        tablayout_ = (TabLayout)mainv_.findViewById(R.id.tab_devices);

        ViewPager viewPager_ = (ViewPager)mainv_.findViewById(R.id.view_pager_backup);
        ViewPagerAdapter adapter = new ViewPagerAdapter(getActivity().getSupportFragmentManager());
        adapter.setDevices(backup_devices_);
        viewPager_.setAdapter(adapter);
        tablayout_.setupWithViewPager(viewPager_);

        return  mainv_;
    }

    void enableFullScreen(){
        View decorView = getActivity().findViewById(R.id.drawer_layout);
        if (decorView != null) {
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    void disableFullScreen(){
        View decorView = getActivity().findViewById(R.id.drawer_layout);
        if (decorView != null) {
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }
    }

    @Override
    public void onDestroyView (){
        tablayout_.setupWithViewPager(null);
        super.onDestroyView();
    }

    public void onButtonPressed(Uri uri) {
        if (mListener != null) {
            mListener.onFragmentInteraction(uri);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        disableFullScreen();
/*
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
*/
    }

    @Override
    public void onDetach() {
        enableFullScreen();
        super.onDetach();
        mListener = null;
    }

    public interface OnFragmentInteractionListener {
        void onFragmentInteraction(Uri uri);
    }


}
