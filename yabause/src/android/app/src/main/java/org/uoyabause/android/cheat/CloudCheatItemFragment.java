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
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupMenu;

import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import org.uoyabause.android.AuthFragment;
import org.devmiyax.yabasanshiro.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;


/**
 * A fragment representing a list of Items.
 * <p/>
 * Activities containing this fragment MUST implement the {@link OnListFragmentInteractionListener}
 * interface.
 */
public class CloudCheatItemFragment extends AuthFragment implements CloudCheatItemRecyclerViewAdapter.OnItemClickListener {

    private static final String TAG ="CloudCheatItemFragment";
    private static final String ARG_COLUMN_COUNT = "column-count";
    private static final String ARG_GAME_ID = "game_id";

    private int mColumnCount = 1;
    private OnListFragmentInteractionListener mListener;
    private ArrayList<CheatItem> _items;
    private String mGameCode;
    DatabaseReference database_;
    View root_view_;
    RecyclerView listview_;
    CloudCheatItemRecyclerViewAdapter adapter_;


    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public CloudCheatItemFragment() {
    }

    @SuppressWarnings("unused")
    public static CloudCheatItemFragment newInstance(String gameid, int columnCount) {
        CloudCheatItemFragment fragment = new CloudCheatItemFragment();
        Bundle args = new Bundle();
        args.putString(ARG_GAME_ID, gameid);
        args.putInt(ARG_COLUMN_COUNT, columnCount);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            mColumnCount = getArguments().getInt(ARG_COLUMN_COUNT);
            mGameCode = getArguments().getString(ARG_GAME_ID);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_cloudcheatitem_list, container, false);
        listview_ = (RecyclerView) view.findViewById(R.id.list);
        //sum_ = (TextView) view.findViewById(R.id.tvSum);
        Context context = view.getContext();
        //LinearLayoutManager layoutManager = new LinearLayoutManager(context);
        //layoutManager.setReverseLayout(true);
        //layoutManager.setStackFromEnd(true);
        //listview_.setLayoutManager(layoutManager);
        root_view_ = view;
        updateCheatList();
        return view;
    }



    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnListFragmentInteractionListener) {
            mListener = (OnListFragmentInteractionListener) context;
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    TabCheatFragment getTabCheatFragmentInstance(){
        TabCheatFragment xFragment = null;
        for(Fragment fragment : getFragmentManager().getFragments()){
            if(fragment instanceof TabCheatFragment){
                xFragment = (TabCheatFragment) fragment;
                break;
            }
        }
        return xFragment;
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
     interface OnListFragmentInteractionListener {
        // TODO: Update argument type and name
        void onListFragmentInteraction(CheatItem item);
    }

    @Override
    protected void OnAuthAccepted(){
        updateCheatList();
    }

    void updateCheatList(){
        FirebaseAuth auth = checkAuth();
        if( auth == null ){
            return;
        }
        _items = new ArrayList<CheatItem>();
        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/shared-cheats/" + mGameCode;
        database_ =  baseref.child(baseurl);
        if( database_ == null ){
            return;
        }
        adapter_ = new CloudCheatItemRecyclerViewAdapter( _items, CloudCheatItemFragment.this);
        listview_.setAdapter(adapter_);

        ValueEventListener DataListener = new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {

                if( dataSnapshot.hasChildren() ) {
                    _items.clear();
                    for (DataSnapshot child : dataSnapshot.getChildren()) {
                        CheatItem newitem = child.getValue(CheatItem.class);
                        newitem.key = child.getKey();

                        TabCheatFragment frag = getTabCheatFragmentInstance();
                        if( frag != null ) {
                            newitem.setEnable( frag.isActive( newitem.getCheatCode() ));
                        }
                        _items.add(newitem);
                    }

                    Collections.reverse(_items);
                    adapter_ = new CloudCheatItemRecyclerViewAdapter( _items, CloudCheatItemFragment.this);
                    listview_.setAdapter(adapter_);
                    adapter_.notifyDataSetChanged();

                }else{
                    Log.e(TAG, "Bad Data " + dataSnapshot.getKey() );
                }
            }
            @Override
            public void onCancelled(DatabaseError databaseError) {
                Log.e(TAG, "onCancelled " + databaseError.getMessage() );
            }
        };
        database_.orderByChild("star_count").addValueEventListener(DataListener);
    }

    @Override
    public void onItemClick( int position, CheatItem item, View v ) {
        PopupMenu popup = new PopupMenu(getActivity(), v);
        MenuInflater inflate = popup.getMenuInflater();
        inflate.inflate(R.menu.cloud_cheat, popup.getMenu());
        final CheatItem cheatitem = item;

        Menu popupMenu = popup.getMenu();
        MenuItem mitem = popupMenu.getItem(0);
        if( cheatitem.getEnable() ){
            mitem.setTitle(R.string.disable);
        }else{
            mitem.setTitle(R.string.enable);
        }

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {

            @Override
            public boolean onMenuItemClick(MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.acp_activate:
                        cheatitem.setEnable( !cheatitem.getEnable() );
                        TabCheatFragment frag = getTabCheatFragmentInstance();
                        if( frag != null ){
                            if( cheatitem.getEnable() ){
                                frag.AddActiveCheat(cheatitem.cheat_code);
                            }else{
                                frag.RemoveActiveCheat(cheatitem.cheat_code);
                            }
                        }
                        adapter_.notifyDataSetChanged();
                        break;
                    case R.id.acp_rate:
                       starItem(cheatitem);
                       break;

                }
                return false;
            }
        });

        popup.show();
    }

    void starItem( final CheatItem item ){

        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            return;
        }

        final String[] items = {"★", "★★", "★★★","★★★★","★★★★★"};
        int defaultItem = 0;
        final List<Integer> checkedItems = new ArrayList<>();
        checkedItems.add(defaultItem);
        new AlertDialog.Builder(getActivity())
                .setTitle("Rate this cheat")
                .setSingleChoiceItems(items, defaultItem, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        checkedItems.clear();
                        checkedItems.add(which);
                    }
                })
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (!checkedItems.isEmpty()) {
                            Log.d("checkedItem:", "" + checkedItems.get(0));

                            DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
                            String baseurl = "/shared-cheats/" + mGameCode + "/" + item.getKey() + "/scores";
                            final DatabaseReference scoresRef =  baseref.child(baseurl);
                            scoresRef.limitToFirst(10).addValueEventListener(new ValueEventListener() {
                                @Override
                                public void onDataChange(DataSnapshot dataSnapshot) {
                                    if( dataSnapshot.hasChildren() ) {
                                        double score = 0;
                                        int cnt = 0;
                                        for (DataSnapshot child : dataSnapshot.getChildren()) {
                                            score += child.getValue(Double.class);
                                            cnt++;
                                        }
                                        score = score / (double)cnt;
                                        database_.child(item.getKey()).child("star_count").setValue(score);
                                    }
                                }

                                @Override
                                public void onCancelled(DatabaseError databaseError) {
                                    // Getting Post failed, log a message
                                    Log.w(TAG, "loadPost:onCancelled", databaseError.toException());
                                    // ...
                                }
                            });
                            FirebaseAuth auth = FirebaseAuth.getInstance();
                            if (auth.getCurrentUser() == null) {
                                return;
                            }
                            database_.child(item.getKey()).child("scores").child(auth.getCurrentUser().getUid()).setValue(checkedItems.get(0)+1);
                        }
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
    }

}
