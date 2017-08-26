package org.uoyabause.android;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TextView;

import com.bumptech.glide.Glide;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Date;
import java.util.List;

import static android.webkit.ConsoleMessage.MessageLevel.LOG;

/*
typedef struct
        {
        char filename[12];
        char comment[11];
        u8 language;
        u8 year;
        u8 month;
        u8 day;
        u8 hour;
        u8 minute;
        u8 week;
        u32 datasize;
        u16 blocksize;
        } saveinfo_struct;
*/

class BackupItem {
    public String _filename;
    public String _comment;
    public int _language;
    public Date _savedate;
    public int _datasize;
    public int _blocksize;

    public BackupItem(){
    }
    public BackupItem(
            String filename,
            String comment,
            int language,
            Date savedate,
            int datasize,
            int blocksize

    ){
        _filename = filename;
        _comment =  comment;
        _language = language;
        _savedate = savedate;
        _datasize = datasize;
        _blocksize = blocksize;
    }
}

class BackupItemComparator implements Comparator<BackupItem> {
    @Override
    public int compare(BackupItem p1, BackupItem p2) {
        int diff = p1._savedate.compareTo(p2._savedate);
        if( diff > 0 ){
            return -1;
        }
        return 1;
    }
}

class BackupItemInvComparator implements Comparator<BackupItem> {
    @Override
    public int compare(BackupItem p1, BackupItem p2) {
        int diff = p1._savedate.compareTo(p2._savedate);
        if( diff > 0 ){
            return 1;
        }
        return -1;
    }
}

class BackupItemAdapter extends RecyclerView.Adapter<BackupItemAdapter.ViewHolder> implements View.OnClickListener {
    private static final String TAG = "BackupItemAdapter";
    private Yabause _yabause;
    private ArrayList<BackupItem> _items = null;
    private RecyclerView mRecycler;
    private int selectpos=0;
    public void setSelected( int pos ){
        selectpos = pos;
    }

    public void setBackupItems( ArrayList<BackupItem> items ){
        this._items = items;
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);
        mRecycler= recyclerView;
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
        super.onDetachedFromRecyclerView(recyclerView);
        mRecycler = null;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        private final TextView textView;
        private final CardView cardView;
        //private final Toolbar toolbar;

        public ViewHolder(View v) {
            super(v);
            v.setClickable(true);
            textView = (TextView) v.findViewById(R.id.tvName);
            cardView = (CardView) v.findViewById(R.id.cardview);
        }
        public TextView getTextView() {
            return textView;
        }
        public CardView getCardView() {
            return cardView;
        }
        //public Toolbar getToolBar() {
        //    return toolbar;
        //}
    }

    private OnItemClickListener mListener;

    public void setOnItemClickListener(OnItemClickListener listener) {
        mListener = listener;
    }

    public static interface OnItemClickListener {
        public void onItemClick(BackupItemAdapter adapter, int position, BackupItem item);
    }

    @Override
    public void onClick(View view) {
        if (mRecycler == null) {
            return;
        }
        if (mListener != null) {
            int position = mRecycler.getChildAdapterPosition(view);
            BackupItem item = _items.get(position);
            mListener.onItemClick(this, position, item);
        }
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType) {
        // Create a new view.
        View v = LayoutInflater.from(viewGroup.getContext())
                .inflate(R.layout.backuplist_item, viewGroup, false);

        v.setOnClickListener(this);
        v.setFocusable(true);
        v.setFocusableInTouchMode(true);
        return new ViewHolder(v);
    }

    public int dp2px( Context cx, int dp) {
        WindowManager wm = (WindowManager) cx
                .getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        DisplayMetrics displaymetrics = new DisplayMetrics();
        display.getMetrics(displaymetrics);
        return (int) (dp * displaymetrics.density + 0.5f);
    }

    static public final String DATE_PATTERN ="yyyy/MM/dd HH:mm:ss";

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, final int position) {
        Log.d(TAG, "Element " + position + " set.");
        viewHolder.getTextView().setText(new SimpleDateFormat(DATE_PATTERN).format(_items.get(position)._savedate));
        //viewHolder.getToolBar().setTitle(new SimpleDateFormat(DATE_PATTERN).format(_state_items.get(position)._savedate));
        //Context cx = viewHolder.getImageView().getContext();
        //Glide.with(cx)
        //        .load(new File(_state_items.get(position)._image_filename))
        //        .centerCrop()
        //        .override(dp2px(cx,220),dp2px(cx,220)*3/4)
        //        .into(viewHolder.getImageView());

        if( selectpos == position ){
            //viewHolder.getCardView().setBackgroundColor( cx.getResources().getColor(R.color.selected_background) );
            viewHolder.getCardView().setSelected(true);
        }else{
            //viewHolder.getCardView().setBackgroundColor( cx.getResources().getColor(R.color.default_background) );
            viewHolder.getCardView().setSelected(false);
        }
    }

    @Override
    public int getItemCount() {
        return _items.size();
    }

    public void remove( int position ) {
/*
        File file = new File(_items.get(position)._filename);
        if( file != null ){
            file.delete();
        }
        file = new File(_items.get(position)._filename);
        if( file != null ){
            file.delete();
        }
*/
        _items.remove(position);
        notifyItemRemoved(position);
    }
}

class BackupDevice {
    public int id_;
    public String name_;
}


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link BackupManagerFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link BackupManagerFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class BackupManagerFragment extends Fragment {

    public static String TAG = "BackupManagerFragment";
    // TODO: Rename parameter arguments, choose names that match
    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
    private static final String ARG_PARAM1 = "param1";
    private static final String ARG_PARAM2 = "param2";

    // TODO: Rename and change types of parameters
    private String mParam1;
    private String mParam2;

    private List<BackupDevice> backup_devices_;

    private OnFragmentInteractionListener mListener;

    public BackupManagerFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment BackupManagerFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static BackupManagerFragment newInstance(String param1, String param2) {
        BackupManagerFragment fragment = new BackupManagerFragment();
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



        //backup_devices_[0].name_;


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View v = inflater.inflate(R.layout.fragment_backup_manager, container, false);

        RadioButton b1 = (RadioButton)v.findViewById(R.id.radioButton_internal);
        b1.setText(backup_devices_.get(0).name_);
        if( backup_devices_.size() >= 2 ){
            b1 = (RadioButton)v.findViewById(R.id.radioButton_external);
            b1.setText(backup_devices_.get(1).name_);
        }else{
            b1 = (RadioButton)v.findViewById(R.id.radioButton_external);
            b1.setEnabled(false);
        }

        // Inflate the layout for this fragment
        return v;
    }


    public void onButtonPressed(Uri uri) {
 //       if (mListener != null) {
 //           mListener.onFragmentInteraction(uri);
 //       }
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
