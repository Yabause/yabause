package org.uoyabause.android;

import android.content.ClipData;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.provider.MediaStore;
import android.support.v4.app.Fragment;
import android.support.v7.widget.CardView;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TextView;

import com.bumptech.glide.Glide;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;

class StateItem {
    public String _filename;
    public String _image_filename;
    public Date _savedate;

    public StateItem(){
        _filename = "";
        _image_filename = "";
    }
    public StateItem( String filename, String image_filename, Date savedate){
        _filename = filename;
        _image_filename = image_filename;
        _savedate = savedate;
    }
}

class StateItemComparator implements Comparator<StateItem> {
    @Override
    public int compare(StateItem p1, StateItem p2) {
        int diff = p1._savedate.compareTo(p2._savedate);
        if( diff > 0 ){
            return -1;
        }
        return 1;
    }
}


class StateItemAdapter extends RecyclerView.Adapter<StateItemAdapter.ViewHolder> implements View.OnClickListener {
    private static final String TAG = "CustomAdapter";
    private Yabause _yabause;
    private ArrayList<StateItem> _state_items = null;
    private RecyclerView mRecycler;
    private int selectpos=0;
    public void setSelected( int pos ){
        selectpos = pos;
    }

    public void setStateItems( ArrayList<StateItem> state_items ){
        this._state_items = state_items;
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
        private final ImageView imageView;
        private final CardView cardView;

        public ViewHolder(View v) {
            super(v);
            v.setClickable(true);
            textView = (TextView) v.findViewById(R.id.textView);
            imageView = (ImageView) v.findViewById(R.id.imageView);
            cardView = (CardView) v.findViewById(R.id.cardview);
        }

        public TextView getTextView() {
            return textView;
        }
        public ImageView getImageView() {
            return imageView;
        }
        public CardView getCardView() {
            return cardView;
        }
    }

    private OnItemClickListener mListener;

    public void setOnItemClickListener(OnItemClickListener listener) {
        mListener = listener;
    }

    public static interface OnItemClickListener {
        public void onItemClick(StateItemAdapter adapter, int position, StateItem item);
    }

    @Override
    public void onClick(View view) {
        if (mRecycler == null) {
            return;
        }
        if (mListener != null) {
            int position = mRecycler.getChildAdapterPosition(view);
            StateItem item = _state_items.get(position);
            mListener.onItemClick(this, position, item);
        }
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType) {
        // Create a new view.
        View v = LayoutInflater.from(viewGroup.getContext())
                .inflate(R.layout.state_item, viewGroup, false);


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

        viewHolder.getTextView().setText(new SimpleDateFormat(DATE_PATTERN).format(_state_items.get(position)._savedate));
        Context cx = viewHolder.getImageView().getContext();
        Glide.with(cx)
                .load(new File(_state_items.get(position)._image_filename))
                .centerCrop()
                .override(dp2px(cx,220),dp2px(cx,220)*3/4)
                .into(viewHolder.getImageView());

        if( selectpos == position ){
            viewHolder.getCardView().setBackgroundColor( cx.getResources().getColor(R.color.selected_background) );
            viewHolder.getCardView().setSelected(true);
        }else{
            viewHolder.getCardView().setBackgroundColor( cx.getResources().getColor(R.color.default_background) );
            viewHolder.getCardView().setSelected(false);
        }
    }

    @Override
    public int getItemCount() {
        return _state_items.size();
    }
}

public class StateListFragment extends Fragment implements StateItemAdapter.OnItemClickListener, View.OnKeyListener {

    public static final String TAG = "StateListFragment";
    protected RecyclerView mRecyclerView;
    protected RecyclerView.LayoutManager mLayoutManager;
    protected StateItemAdapter mAdapter;
    ArrayList<StateItem> _state_items = null;
    int mSelectedItem = 0;



    protected String _basepath = "";

    public void setBasePath( String basepath ){
        this._basepath = basepath;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        _state_items = new ArrayList<StateItem>();

        File folder = new File(_basepath);
        if( folder == null ){
            return ;
        }

        File[] listOfFiles = folder.listFiles();
        if(listOfFiles==null ){
            return;
        }

        for (int i = 0; i < listOfFiles.length; i++) {
            if (listOfFiles[i].isFile()) {
                String filename = listOfFiles[i].getName();
                int point = filename.lastIndexOf(".");
                if (point != -1) {
                    String ext = filename.substring(point, point + 4);
                    if( ext.equals(".yss")){
                        StateItem item = new StateItem();
                        item._filename = _basepath + "/" + filename;
                        item._image_filename = _basepath + "/" + filename.substring(0, point) + ".png";
                        item._savedate =  new Date(listOfFiles[i].lastModified());
                        _state_items.add(item);
                    }
                }
                //System.out.println("File " + listOfFiles[i].getName());
            } else if (listOfFiles[i].isDirectory()) {
                //System.out.println("Directory " + listOfFiles[i].getName());
            }
        }

        if( _state_items.size() > 1 ){
            Collections.sort(_state_items, new StateItemComparator());
        }


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View rootView = inflater.inflate(R.layout.state_list_frag, container, false);
        rootView.setTag(TAG);
        View v = rootView.findViewById(R.id.textView_empty);
        mRecyclerView = (RecyclerView) rootView.findViewById(R.id.recyclerView);

        if( _state_items.size() == 0 ){

            mRecyclerView.setVisibility(View.GONE);
            return rootView;
        }

        v.setVisibility(View.GONE);
        mAdapter = new StateItemAdapter();
        mAdapter.setStateItems(_state_items);
        mAdapter.setOnItemClickListener(this);



        mRecyclerView.setAdapter(mAdapter);
        int scrollPosition = 0;
        // If a layout manager has already been set, get current scroll position.
        if (mRecyclerView.getLayoutManager() != null) {
            scrollPosition = ((LinearLayoutManager) mRecyclerView.getLayoutManager())
                    .findFirstCompletelyVisibleItemPosition();
        }
        mLayoutManager = new LinearLayoutManager(getActivity());
        mRecyclerView.setLayoutManager(mLayoutManager);
        selectItem(0);
        rootView.setFocusableInTouchMode(true);
        rootView.requestFocus();
        rootView.setOnKeyListener(this);

        return rootView;

    }

    public void onItemClick(StateItemAdapter adapter, int position, StateItem item){
        Yabause main = (Yabause)getActivity();
        if( main != null ) {
            main.loadState( item._filename );
        }
    }

    void  selectPrevious(){
        if (mSelectedItem < 1)
            return;
        selectItem(--mSelectedItem);
    }

    void  selectNext(){
        if (mSelectedItem >= mAdapter.getItemCount()-1)
            return;
        selectItem(++mSelectedItem);
    }


    private void selectItem(int position){
        mSelectedItem = position;
        mAdapter.setSelected(mSelectedItem);
        mAdapter.notifyDataSetChanged();
        mRecyclerView.stopScroll();
        mRecyclerView.smoothScrollToPosition(position);

/*
        mRecyclerView.post(new Runnable() {
            @Override
            public void run() {
                View v;
                for (int i = 0 ; i< mAdapter.getItemCount() ; ++i){
                    v = mLayoutManager.findViewByPosition(i);
                    if (v != null) {
                        v.setSelected(i == mSelectedItem);
                     }
                }
            }
        });
*/
    }


    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if(event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }
        if( _state_items.size() == 0 ){
            return false;
        }
        switch (keyCode) {
            case KeyEvent.KEYCODE_MEDIA_PLAY:
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
            case KeyEvent.KEYCODE_SPACE:
            case KeyEvent.KEYCODE_BUTTON_A:
                Yabause main = (Yabause)getActivity();
                if( main != null ) {
                    main.loadState( _state_items.get(mSelectedItem)._filename );
                }
                break;
            case KeyEvent.KEYCODE_DPAD_UP:
                selectPrevious();
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                selectNext();
                return true;
        }
        return false;
    }

    @Override
    public void onResume() {
        super.onResume();
    };
}
