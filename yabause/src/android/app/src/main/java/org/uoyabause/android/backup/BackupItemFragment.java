package org.uoyabause.android.backup;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Base64;
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

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

class BackupDevice {
    public int id_;
    public String name_;
}

class BackupItem {
    public int index_;
    public String _filename;
    public String _comment;
    public int _language;
    public Date _savedate;
    public int _datasize;
    public int _blocksize;

    public BackupItem(){
    }
    public BackupItem(
            int index,
            String filename,
            String comment,
            int language,
            Date savedate,
            int datasize,
            int blocksize

    ){
        index_ = index;
        _filename = filename;
        _comment =  comment;
        _language = language;
        _savedate = savedate;
        _datasize = datasize;
        _blocksize = blocksize;
    }
}


/**
 * A fragment representing a list of Items.
 * <p/>
 * Activities containing this fragment MUST implement the {@link OnListFragmentInteractionListener}
 * interface.
 */
public class BackupItemFragment extends Fragment implements BackupItemRecyclerViewAdapter.OnItemClickListener  {

    static final String TAG = "BackupItemFragment";
    private List<BackupDevice> backup_devices_;

    // TODO: Customize parameter argument names
    private static final String ARG_COLUMN_COUNT = "column-count";
    // TODO: Customize parameters
    private int mColumnCount = 3;
    private OnListFragmentInteractionListener mListener;

    private ArrayList<BackupItem> _items;
    int currentpage_ = 0;

    RecyclerView view_;
    TextView sum_;
    BackupItemRecyclerViewAdapter adapter_;

    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public BackupItemFragment() {
    }

    public static BackupItemFragment getInstance(int position) {
        BackupItemFragment f = new BackupItemFragment();
        Bundle args = new Bundle();
        args.putInt("position", position);
        args.putInt(ARG_COLUMN_COUNT, 1);
        f.setArguments(args);
        return f;
    }

    // TODO: Customize parameter initialization
    @SuppressWarnings("unused")
    public static BackupItemFragment newInstance(int columnCount) {
        BackupItemFragment fragment = new BackupItemFragment();
        Bundle args = new Bundle();
        args.putInt(ARG_COLUMN_COUNT, columnCount);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            updateSaveList(currentpage_);
        }else {
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            mColumnCount = getArguments().getInt(ARG_COLUMN_COUNT);
            currentpage_ = getArguments().getInt("position");
        }
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

    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_backupitem_list, container, false);
        //View v = inflater.inflate(R.layout.fragment_tab_backup, container, false);
        view_ = (RecyclerView) view.findViewById(R.id.list);
        sum_ = (TextView) view.findViewById(R.id.tvSum);

            Context context = view.getContext();
            if (mColumnCount <= 1) {
                view_.setLayoutManager(new LinearLayoutManager(context));
            } else {
                view_.setLayoutManager(new GridLayoutManager(context, mColumnCount));
            }
            //recyclerView.setAdapter(new BackupItemRecyclerViewAdapter(DummyContent.ITEMS, mListener));
            //adapter_ = new BackupItemRecyclerViewAdapter(_items,mListener);
            //view_.setAdapter(adapter_);
            updateSaveList(currentpage_);

        return view;
    }
    private int totalsize_ = 0;
    private int freesize_ = 0;


    void updateSaveList( int device ){
        String jsonstr = YabauseRunnable.getFilelist(device);
        _items = new ArrayList<BackupItem>();
        try {
            JSONObject json = new JSONObject(jsonstr);
            totalsize_ = json.getJSONObject("status").getInt("totalsize");
            freesize_ = json.getJSONObject("status").getInt("freesize");
            JSONArray array = json.getJSONArray("saves");
            for (int i = 0; i < array.length(); i++) {
                JSONObject data = array.getJSONObject(i);
                BackupItem tmp = new BackupItem();
                tmp.index_ = data.getInt("index");

                byte [] bfilename = Base64.decode( data.getString("filename"), 0 );
                try {
                    tmp._filename = new String(bfilename, "MS932");
                }catch(Exception e){
                    tmp._filename = data.getString("filename");
                }
                bfilename = Base64.decode( data.getString("comment"), 0 );
                try {
                    tmp._comment = new String(bfilename, "MS932");
                }catch(Exception e){
                    tmp._comment = data.getString("comment");
                }
                tmp._datasize = data.getInt("datasize");
                tmp._blocksize = data.getInt("blocksize");

                String datestr = "";
                datestr += String.format("%04d",data.getInt("year")+1980);
                datestr += String.format("%02d",data.getInt("month"));
                datestr += String.format("%02d",data.getInt("day"));
                datestr += " ";
                datestr += String.format("%02d",data.getInt("hour")) + ":";
                datestr += String.format("%02d",data.getInt("minute")) + ":00";

                SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd HH:mm:ss");
                try{ tmp._savedate = sdf.parse(datestr); }catch( Exception e) {
                    Log.e(TAG, "fail to convert datestr",e);
                    tmp._savedate = new Date();
                }
                _items.add(tmp);
            }

            if( view_ != null ) {
                adapter_ = new BackupItemRecyclerViewAdapter(currentpage_,_items,this);
                view_.setAdapter(adapter_);
            }

            if( sum_ != null ){
                sum_.setText( String.format("%,d Byte free of %,d Byte",freesize_,totalsize_) );
            }

        }catch( JSONException e ){
            Log.e(TAG, "Fail to convert to json", e);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
//        if (context instanceof OnListFragmentInteractionListener) {
//            mListener = (OnListFragmentInteractionListener) context;
//        } else {
//            throw new RuntimeException(context.toString()
//                    + " must implement OnListFragmentInteractionListener");
//        }
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
     * <p/>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnListFragmentInteractionListener {
        // TODO: Update argument type and name
        void onItemClickx(int currentpage, int position, BackupItem backupitem, View v );
    }

    @Override
    public void onItemClick(int currentpage, int position, BackupItem backupitem, View v ) {
        PopupMenu popup = new PopupMenu(getActivity(), v);
        MenuInflater inflate = popup.getMenuInflater();
        inflate.inflate(R.menu.backup, popup.getMenu());
        final BackupItem backupitemi = backupitem;
        final int position_ = position;

        Menu popupMenu = popup.getMenu();

        if( currentpage == 0) {
            popupMenu.findItem(R.id.copy_to_internal).setVisible(false);
        }
        else if( currentpage == 1) {
            popupMenu.findItem(R.id.copy_to_external).setVisible(false);
        }
        else if( currentpage == 2) {
            popupMenu.findItem(R.id.copy_to_cloud).setVisible(false);
        }

        // No external device
        if( backup_devices_.size() <= 1 ){
            popupMenu.findItem(R.id.copy_to_external).setVisible(false);
        }

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {

            @Override
            public boolean onMenuItemClick(MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.copy_to_internal:

                        if( BackupItemFragment.this.backup_devices_.size() >= 1 ) {
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(0);
                            YabauseRunnable.copy(bk.id_, backupitemi.index_ );
                        }

                        break;
                    case R.id.copy_to_external:

                        if( BackupItemFragment.this.backup_devices_.size() >= 2 ) {
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(1);
                            YabauseRunnable.copy(bk.id_, backupitemi.index_ );
                        }

                        break;
                    case R.id.copy_to_cloud:

                        break;
                    case R.id.delete:
                        YabauseRunnable.deletefile( backupitemi.index_ );
                        BackupItemFragment.this.view_.removeViewAt(position_);
                        BackupItemFragment.this._items.remove(position_);
                        BackupItemFragment.this.adapter_.notifyItemRemoved(position_);
                        BackupItemFragment.this.adapter_.notifyItemRangeChanged(position_, BackupItemFragment.this._items.size());
                        break;
                    default:
                        return false;
                }
                return false;
            }
        });
        popup.show();
    }

}
