package org.uoyabause.android.backup;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.design.widget.TabLayout;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupMenu;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.android.R;
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
        super.onDetach();
        mListener = null;
    }

    public interface OnFragmentInteractionListener {
        void onFragmentInteraction(Uri uri);
    }


}
