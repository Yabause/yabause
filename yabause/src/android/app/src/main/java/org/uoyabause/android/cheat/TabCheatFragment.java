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

package org.uoyabause.android.cheat;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import com.google.android.material.tabs.TabLayout;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.uoyabause.android.Yabause;
import org.uoyabause.uranus.R;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

class ViewPagerAdapter extends FragmentStatePagerAdapter {

    static final int PAGE_LOCAL = 0;
    static final int PAGE_CLOUD = 1;
    String gameid_;

    public ViewPagerAdapter(FragmentManager fm) {
        super(fm);
    }

    public void setGmaeid( String gameid ){
        gameid_ = gameid;
    }

    @Override
    public Fragment getItem(int position) {
        switch(position){
            case PAGE_LOCAL:
                return LocalCheatItemFragment.newInstance(gameid_,1);
            case PAGE_CLOUD:
                return CloudCheatItemFragment.newInstance(gameid_,1);
        }
        return null;
    }

    @Override
    public int getCount() {
        return 2;
    }

    @Override
    public CharSequence getPageTitle(int position) {

        switch(position){
            case PAGE_LOCAL:
                return "Local";
            case PAGE_CLOUD:
                return "Shared";
        }
        return "";
    }

}

public class TabCheatFragment extends Fragment {

    public static final String TAG = "TabBackupFragment";
    private static final String ARG_GAME_ID = "gameid";
    private static final String ARG_CURRENT_CHEAT = "current_cheats";
    private String mGameid;
    private OnFragmentInteractionListener mListener;

    List<String> active_cheats_;

    View mainv_;
    TabLayout tablayout_;

    public TabCheatFragment() {
    }

    public static TabCheatFragment newInstance(String gameid, String[] current_cheat_code ) {
        TabCheatFragment fragment = new TabCheatFragment();
        Bundle args = new Bundle();
        args.putString(ARG_GAME_ID, gameid);
        args.putStringArray(ARG_CURRENT_CHEAT, current_cheat_code);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        active_cheats_ = new ArrayList<String>();
        if (getArguments() != null) {
            mGameid = getArguments().getString(ARG_GAME_ID);
            String[] current_cheat_code = getArguments().getStringArray(ARG_CURRENT_CHEAT);
            if( current_cheat_code != null ) {
                for (int i = 0; i < current_cheat_code.length; i++) {
                    active_cheats_.add(current_cheat_code[i]);
                }
            }
        }
    }

    public void AddActiveCheat( String v ){
        active_cheats_.add(v);
        Set<String> set = new HashSet<String>();
        set.addAll(active_cheats_);
        active_cheats_.addAll(set);
    }

    public  void RemoveActiveCheat( String v ){
        active_cheats_.remove(v);
        active_cheats_.remove(v);
    }

    public  boolean isActive( String v ){

        if( active_cheats_ == null ){
            return false;
        }

        for (String string : active_cheats_) {
            if(string.equals(v)){
                return true;
            }
        }
        return false;
    }


    void sendCheatListToYabause(){
        int cntChoice = active_cheats_.size();
        if( cntChoice > 0 ) {
            String[] cheatitem = new String[cntChoice];
            int cindex = 0;
            for (int i = 0; i < cntChoice; i++) {
                cheatitem[cindex] = active_cheats_.get(cindex);
                cindex++;
            }
            Yabause activity = (Yabause) getActivity();
            activity.updateCheatCode(cheatitem);
        }else{
            Yabause activity = (Yabause) getActivity();
            activity.updateCheatCode(null);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        mainv_ =  inflater.inflate(R.layout.fragment_tab_cheat, container, false);
        tablayout_ = (TabLayout)mainv_.findViewById(R.id.tab_cheat_source);
        ViewPager viewPager_ = (ViewPager)mainv_.findViewById(R.id.view_pager_cheat);
        ViewPagerAdapter adapter = new ViewPagerAdapter(getActivity().getSupportFragmentManager());
        adapter.setGmaeid( mGameid );
        viewPager_.setAdapter(adapter);
        tablayout_.setupWithViewPager(viewPager_);
        return mainv_;
    }

    public void onButtonPressed(Uri uri) {
        if (mListener != null) {
            mListener.onFragmentInteraction(uri);
        }
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
    public void onAttach(Context context) {
        disableFullScreen();
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
        }
    }


    @Override
    public void onDetach() {
        enableFullScreen();
        sendCheatListToYabause();
        super.onDetach();
        mListener = null;
    }

    public interface OnFragmentInteractionListener {
        void onFragmentInteraction(Uri uri);
    }

}
