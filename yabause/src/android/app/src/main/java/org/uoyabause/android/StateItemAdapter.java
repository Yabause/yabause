package org.uoyabause.android;

import android.content.Context;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.cardview.widget.CardView;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.request.RequestOptions;

import org.devmiyax.yabasanshiro.R;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;

public class StateItemAdapter extends RecyclerView.Adapter<StateItemAdapter.ViewHolder> implements View.OnClickListener {
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

    public int dp2px(Context cx, int dp) {
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
