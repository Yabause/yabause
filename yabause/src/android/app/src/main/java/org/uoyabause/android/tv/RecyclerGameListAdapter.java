package org.uoyabause.android.tv;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.CardView;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import com.activeandroid.query.Select;

import org.uoyabause.android.GameInfo;
import org.uoyabause.android.R;
import org.uoyabause.android.YabauseStorage;

import java.util.List;

/**
 * Created by shinya on 2016/01/03.
 */
public class RecyclerGameListAdapter extends RecyclerView.Adapter<RecyclerGameListAdapter.ViewHolder> {
    List<GameInfo> _gamelist;

    // Provide a reference to the views for each data item
    // Complex data items may need more than one view per item, and
    // you provide access to all the views for a data item in a view holder
    public static class ViewHolder extends RecyclerView.ViewHolder {
        // each data item is just a string in this case
        public CardView _cardview;
        public TextView _title;
        public ViewHolder(CardView v) {
            super(v);
            _cardview = v;
            _title = (TextView) v.findViewById(R.id.game_title);
        }
    }

    // Provide a suitable constructor (depends on the kind of dataset)
    public RecyclerGameListAdapter() {
        YabauseStorage yb = YabauseStorage.getStorage();
        yb.generateGameDB(3);
        _gamelist = new Select()
                .from(GameInfo.class)
                .orderBy("game_title ASC")
                .execute();
    }

    // Create new views (invoked by the layout manager)
    @Override
    public RecyclerGameListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent,
                                                   int viewType) {
        // create a new view
        CardView v = (CardView)LayoutInflater.from(parent.getContext())
                .inflate(R.layout.game_card_listitem, parent, false);
        // set the view's size, margins, paddings and layout parameters
        ViewHolder vh = new ViewHolder(v);
        return vh;
    }

    // Replace the contents of a view (invoked by the layout manager)
    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element
        //holder.mTextView.setText(mDataset[position]);

        holder._title.setText( _gamelist.get(position).game_title );

    }

    // Return the size of your dataset (invoked by the layout manager)
    @Override
    public int getItemCount() {
        return _gamelist.size();
    }
}
