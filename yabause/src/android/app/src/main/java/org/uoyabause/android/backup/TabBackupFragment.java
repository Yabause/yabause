package org.uoyabause.android.backup;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.design.widget.TabLayout;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.app.Fragment;
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


class ViewPagerAdapter extends FragmentPagerAdapter {

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

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link TabBackupFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link TabBackupFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class TabBackupFragment extends Fragment  {

    public static final String TAG = "TabBackupFragment";
    private List<BackupDevice> backup_devices_;

    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
    private static final String ARG_PARAM1 = "param1";
    private static final String ARG_PARAM2 = "param2";
    View mainv_;
    TabLayout tablayout_;


    // TODO: Rename and change types of parameters
    private String mParam1;
    private String mParam2;

    private OnFragmentInteractionListener mListener;

    public TabBackupFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment TabBackupFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static TabBackupFragment newInstance(String param1, String param2) {
        TabBackupFragment fragment = new TabBackupFragment();
        Bundle args = new Bundle();
        args.putString(ARG_PARAM1, param1);
        args.putString(ARG_PARAM2, param2);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            mParam1 = getArguments().getString(ARG_PARAM1);
            mParam2 = getArguments().getString(ARG_PARAM2);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        //View v = inflater.inflate(R.layout.fragment_tab_backup, container, false);
        //mainv_ = v.findViewById(R.id.list);

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
        //tablayout_.addTab(tablayout_.newTab().setText("Intarnal backup memory"));
        //tablayout_.addTab(tablayout_.newTab().setText("External backup memory"));
        //tablayout_.addTab(tablayout_.newTab().setText("Cloud backup memory"));


        ViewPager viewPager_ = (ViewPager)mainv_.findViewById(R.id.view_pager_backup);
        ViewPagerAdapter adapter = new ViewPagerAdapter(getActivity().getSupportFragmentManager());
        adapter.setDevices(backup_devices_);
        viewPager_.setAdapter(adapter);
        tablayout_.setupWithViewPager(viewPager_);

        return  mainv_;
    }

    // TODO: Rename method, update argument and hook method into UI event
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

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnFragmentInteractionListener {
        // TODO: Update argument type and name
        void onFragmentInteraction(Uri uri);
    }


}
