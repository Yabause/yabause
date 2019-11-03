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


package org.uoyabause.android;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.fragment.app.Fragment;
import androidx.cardview.widget.CardView;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.ItemTouchHelper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.request.RequestOptions;

import org.uoyabause.uranus.R;

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

class StateItemInvComparator implements Comparator<StateItem> {
    @Override
    public int compare(StateItem p1, StateItem p2) {
        int diff = p1._savedate.compareTo(p2._savedate);
        if( diff > 0 ){
            return 1;
        }
        return -1;
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
        //private final Toolbar toolbar;

        public ViewHolder(View v) {
            super(v);
            v.setClickable(true);
            textView = (TextView) v.findViewById(R.id.textView);
/*
            toolbar = (Toolbar) v.findViewById(R.id.card_toolbar);
            if (toolbar != null) {
                // inflate your menu
                toolbar.inflateMenu(R.menu.state);
                toolbar.setOnMenuItemClickListener(new Toolbar.OnMenuItemClickListener() {
                    @Override
                    public boolean onMenuItemClick(MenuItem menuItem) {
                        return true;
                    }
                });
            }
*/
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
        //public Toolbar getToolBar() {
        //    return toolbar;
        //}
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
        //viewHolder.getToolBar().setTitle(new SimpleDateFormat(DATE_PATTERN).format(_state_items.get(position)._savedate));
        Context cx = viewHolder.getImageView().getContext();
        Glide.with(cx)
                .load(new File(_state_items.get(position)._image_filename))
                .apply(new RequestOptions().transforms(new CenterCrop() ))
                .into(viewHolder.getImageView());

                        //.override(dp2px(cx,220),dp2px(cx,220)*3/4)

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

    public void remove( int position ) {
        File file = new File(_state_items.get(position)._filename);
        if( file != null ){
            file.delete();
        }
        file = new File(_state_items.get(position)._image_filename);
        if( file != null ){
            file.delete();
        }
        _state_items.remove(position);
        notifyItemRemoved(position);
    }
}


public class StateListFragment extends Fragment implements StateItemAdapter.OnItemClickListener, View.OnKeyListener {

    public static final String TAG = "StateListFragment";
    protected RecyclerView mRecyclerView;
    protected RecyclerView.LayoutManager mLayoutManager;
    protected View emptyView;
    protected StateItemAdapter mAdapter;
    ArrayList<StateItem> _state_items = null;
    ItemTouchHelper mHelper;
    int mSelectedItem = 0;
    protected String _basepath = "";

    public void setBasePath( String basepath ){
        this._basepath = basepath;
    }

    public static void checkMaxFileCount( String basepath ){
        ArrayList<StateItem>  state_items = new ArrayList<StateItem>();

        File folder = new File(basepath);
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
                        item._filename = basepath + "/" + filename;
                        item._image_filename = basepath + "/" + filename.substring(0, point) + ".png";
                        item._savedate =  new Date(listOfFiles[i].lastModified());
                        state_items.add(item);
                    }
                }
            }
        }

        int filecount = state_items.size();
        if( filecount > 10 ){
            for( int i = 0; i < filecount-10; i++ ) {
                Collections.sort(state_items, new StateItemInvComparator());
                File save_file = new File(state_items.get(i)._filename);
                save_file.delete();
                File img_file = new File(state_items.get(i)._image_filename);
                img_file.delete();
            }
        }

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
        emptyView = rootView.findViewById(R.id.textView_empty);
        mRecyclerView = (RecyclerView) rootView.findViewById(R.id.recyclerView);

        if( _state_items.size() == 0 ){

            mRecyclerView.setVisibility(View.GONE);
            return rootView;
        }

        emptyView.setVisibility(View.GONE);
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

        ItemTouchHelper.SimpleCallback callback = new ItemTouchHelper.SimpleCallback(0, ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT) {
            @Override
            public boolean onMove(RecyclerView recyclerView, RecyclerView.ViewHolder viewHolder, RecyclerView.ViewHolder target) {
                return false;
            }

            @Override
            public void onSwiped(RecyclerView.ViewHolder viewHolder, int direction) {

                // 横にスワイプされたら要素を消す
                int swipedPosition = viewHolder.getAdapterPosition();
                mAdapter.remove(swipedPosition);
                if( mAdapter.getItemCount() == 0 ){
                    emptyView.setVisibility(View.VISIBLE);
                    mRecyclerView.setVisibility(View.GONE);
                }
            }
        };

        mHelper = new ItemTouchHelper(new ItemTouchHelper.Callback() {
            @Override
            public int getMovementFlags(RecyclerView recyclerView, RecyclerView.ViewHolder viewHolder) {
//ここでドラッグ動作、Swipe動作を指定します
//ドラッグさせたくないとか、Swipeさせたくない場合はここで分岐してアクションを指定しないことでドラッグできない行などを指定できます
//ドラッグは長押しで自動的に開始されます
                return makeFlag(ItemTouchHelper.ACTION_STATE_IDLE, ItemTouchHelper.RIGHT) | makeFlag(ItemTouchHelper.ACTION_STATE_SWIPE, ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT) |
                        makeFlag(ItemTouchHelper.ACTION_STATE_DRAG, ItemTouchHelper.DOWN | ItemTouchHelper.UP);
            }


            //ドラッグで場所を移動した際の処理を記述します
            @Override
            public boolean onMove(RecyclerView recyclerView, RecyclerView.ViewHolder viewHolder, RecyclerView.ViewHolder viewHolder1) {
                //((StateItemAdapter) recyclerView.getAdapter()).move(viewHolder.getAdapterPosition(), viewHolder1.getAdapterPosition());
                return true;
            }


            //選択ステータスが変更された場合の処理を指定します
//この例ではAdapterView内のcontainerViewを表示にしています
//containerViewには背景色を指定しており、ドラッグが開始された際に見やすくなるようにしています
            @Override
            public void onSelectedChanged(RecyclerView.ViewHolder viewHolder, int actionState) {
                super.onSelectedChanged(viewHolder, actionState);

                //if (actionState == ItemTouchHelper.ACTION_STATE_DRAG)
                 //   ((StateItemAdapter.holder) viewHolder).container.setVisibility(View.VISIBLE);
            }
            //選択が終わった時（Dragが終わった時など）の処理を指定します
//今回はアイテムをDropした際にcontainerViewを非表示にして通常表示に戻しています
            @Override
            public void clearView(RecyclerView recyclerView, RecyclerView.ViewHolder viewHolder) {
                super.clearView(recyclerView, viewHolder);
               // ((StateItemAdapter.holder) viewHolder).container.setVisibility(View.GONE);
            }


            //Swipeされた際の処理です。
//
            @Override
            public void onSwiped(RecyclerView.ViewHolder viewHolder, int i) {
                ((StateItemAdapter) mRecyclerView.getAdapter()).remove(viewHolder.getAdapterPosition());
                if( mAdapter.getItemCount() == 0 ){
                    emptyView.setVisibility(View.VISIBLE);
                    mRecyclerView.setVisibility(View.GONE);
                }
            }
        });
        mHelper.attachToRecyclerView(mRecyclerView);
        mRecyclerView.addItemDecoration(mHelper);
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
        selectItem(mSelectedItem-1);
    }

    void  selectNext(){
        if (mSelectedItem >= mAdapter.getItemCount()-1)
            return;
        selectItem(mSelectedItem+1);
    }


    private void selectItem(int position){
        if( position < 0 ) position = 0;
        if( position >= mAdapter.getItemCount() ) position = mAdapter.getItemCount()-1;

        int pre = mSelectedItem;

        mSelectedItem = position;
        mAdapter.setSelected(mSelectedItem);
        mAdapter.notifyItemChanged(pre);
        mAdapter.notifyItemChanged(mSelectedItem);
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
            case KeyEvent.KEYCODE_BUTTON_X:

                new AlertDialog.Builder(getActivity())
                        .setMessage(getString(R.string.delete_confirm))
                        .setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                mAdapter.remove( mSelectedItem );
                                if( mAdapter.getItemCount() == 0 ){
                                    emptyView.setVisibility(View.VISIBLE);
                                    mRecyclerView.setVisibility(View.GONE);
                                }else {

                                    int newsel =mSelectedItem;
                                    if (newsel >= mAdapter.getItemCount() - 1) {
                                        newsel -= 1;
                                    }
                                    selectItem(newsel);
                                }
                            }
                        })
                        .setNegativeButton(getString(R.string.cancel), null)
                        .show();

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
