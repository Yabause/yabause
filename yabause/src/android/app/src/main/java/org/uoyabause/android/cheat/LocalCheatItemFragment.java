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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;

import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupMenu;

import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import org.devmiyax.yabasanshiro.R;

import java.util.ArrayList;

import android.widget.Toast;

/**
 * A fragment representing a list of Items.
 * <p/>
 * Activities containing this fragment MUST implement the {@link OnListFragmentInteractionListener}
 * interface.
 */
public class LocalCheatItemFragment extends Fragment
        implements LocalCheatItemRecyclerViewAdapter.OnItemClickListener
{
    private static final String ARG_COLUMN_COUNT = "column-count";
    private static final String ARG_GAME_ID = "game_id";
    private int mColumnCount = 1;
    private OnListFragmentInteractionListener mListener;
    private ArrayList<CheatItem> _items;
    private String mGameCode;
    private String backcode_ = "";

    DatabaseReference database_ = null;
    View root_view_;
    RecyclerView listview_;
    LocalCheatItemRecyclerViewAdapter adapter_;

    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public LocalCheatItemFragment() {
    }

    @SuppressWarnings("unused")
    public static LocalCheatItemFragment newInstance( String gameid, int columnCount) {
        LocalCheatItemFragment fragment = new LocalCheatItemFragment();
        android.os.Bundle args = new Bundle();
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
        CheatConverter cnv = new CheatConverter();
        if( cnv.hasOldVersion() ){
            cnv.execute();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_localcheatitem_list, container, false);
        listview_ = (RecyclerView) view.findViewById(R.id.list);

        Context context = view.getContext();
        listview_.setLayoutManager(new LinearLayoutManager(context));
        updateCheatList();
        root_view_ = view;

        Button add = (Button)view.findViewById(R.id.button_add);
        add.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                onAddItem(v);
            }
        });
        return view;

    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            //updateCheatList();
        }else {
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnListFragmentInteractionListener) {
            mListener = (OnListFragmentInteractionListener) context;
        } else {
//            throw new RuntimeException(context.toString()
//                    + " must implement OnListFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach() {

        super.onDetach();
        mListener = null;
    }

    public interface OnListFragmentInteractionListener {
        void onListFragmentInteraction(CheatItem item);
    }

    void showErrorMessage(){
        Toast.makeText(getContext(),"You need to Sign in before use this function",Toast.LENGTH_LONG).show();
    }

    void updateCheatList(){
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            showErrorMessage();
            return;
        }
        if( listview_== null ){
            return;
        }
        _items = new ArrayList<CheatItem>();
        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/user-posts/" + auth.getCurrentUser().getUid()  + "/cheat/" + mGameCode;
        database_ =  baseref.child(baseurl);
        if( database_ == null ){
            showErrorMessage();
            return;
        }
        adapter_ = new LocalCheatItemRecyclerViewAdapter( _items, LocalCheatItemFragment.this);
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
                    adapter_ = new LocalCheatItemRecyclerViewAdapter( _items, LocalCheatItemFragment.this);
                    listview_.setAdapter(adapter_);

                }else{
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.getKey() );
                }
            }
            @Override
            public void onCancelled(DatabaseError databaseError) {
                Log.e("CheatEditDialog", "Bad Data " + databaseError.getMessage() );
            }
        };
        database_.addValueEventListener(DataListener);
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

    @Override
    public void onItemClick( int position, CheatItem item, View v ) {
        PopupMenu popup = new PopupMenu(getActivity(), v);
        MenuInflater inflate = popup.getMenuInflater();
        inflate.inflate(R.menu.local_cheat, popup.getMenu());
        final CheatItem cheatitem = item;
        final int position_ = position;

        Menu popupMenu = popup.getMenu();
        MenuItem mitem = popupMenu.getItem(0);
        if( cheatitem.getEnable() ){
            mitem.setTitle(R.string.disable);
        }else{
            mitem.setTitle(R.string.enable);
        }

        MenuItem sitem = popupMenu.getItem(2);
        if( !cheatitem.getSharedKey().equals("")  ){
            sitem.setTitle(R.string.unsahre);
        }else {
            sitem.setTitle(R.string.share);
        }

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {

            @Override
            public boolean onMenuItemClick(MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.acp_activate:
                        cheatitem.setEnable( !cheatitem.getEnable() );
                        TabCheatFragment frag = getTabCheatFragmentInstance();
                        if( frag != null ){
                            if( cheatitem.getEnable() == true ){
                                frag.AddActiveCheat(cheatitem.cheat_code);
                            }else{
                                frag.RemoveActiveCheat(cheatitem.cheat_code);
                            }
                        }

                        adapter_.notifyDataSetChanged();
                        break;
                    case R.id.acp_edit:
                        backcode_ = cheatitem.getCheatCode();
                        LocalCheatEditDialog newFragment = new LocalCheatEditDialog();
                        newFragment.setEditTarget(cheatitem);
                        newFragment.setTargetFragment(LocalCheatItemFragment.this,EDIT_ITEM);
                        newFragment.show(getFragmentManager(), "Cheat");
                        break;
                    case R.id.acp_share:
                        if( !cheatitem.getSharedKey().equals("")  ){
                            UnShare(cheatitem);
                        }else {
                            Share(cheatitem);
                        }
                        break;
                    case R.id.delete:
                        RemoveCheat(cheatitem);
                        adapter_.notifyDataSetChanged();
                        break;
                    default:
                        return false;
                }
                return false;
            }
        });

        popup.show();
    }

    static final int NEW_ITEM = 0;
    static final int EDIT_ITEM = 1;

    void onAddItem( View v){
        LocalCheatEditDialog newFragment = new LocalCheatEditDialog();
        newFragment.setTargetFragment(this,NEW_ITEM);
        newFragment.show(getFragmentManager(), "Cheat");


    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == NEW_ITEM) {
            if(resultCode == LocalCheatEditDialog.APPLY) {
                if( database_ == null ){
                    showErrorMessage();
                    return;
                }
                String key = database_.push().getKey();
                String desc = data.getStringExtra( LocalCheatEditDialog.DESC );
                String code = data.getStringExtra( LocalCheatEditDialog.CODE );
                database_.child(key).child("description").setValue(desc);
                database_.child(key).child("cheat_code").setValue(code);
                adapter_.notifyDataSetChanged();
            }
        }else if(requestCode == EDIT_ITEM){
            if(resultCode == LocalCheatEditDialog.APPLY) {
                if( database_ == null ){
                    showErrorMessage();
                    return;
                }
                TabCheatFragment frag = getTabCheatFragmentInstance();
                if( frag != null ){
                    frag.RemoveActiveCheat(backcode_);
                }
                String key = data.getStringExtra( LocalCheatEditDialog.KEY );
                String desc = data.getStringExtra( LocalCheatEditDialog.DESC );
                String code = data.getStringExtra( LocalCheatEditDialog.CODE );
                database_.child(key).child("description").setValue(desc);
                database_.child(key).child("cheat_code").setValue(code);
                adapter_.notifyDataSetChanged();
            }
        }
    }

    public void RemoveCheat( final CheatItem cheatitem ){
        new AlertDialog.Builder(getActivity())
                .setMessage(getString(R.string.are_you_sure_to_delete) + cheatitem.description + "?")
                .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if( database_ == null ){
                            showErrorMessage();
                            return;
                        }
                        database_.child(cheatitem.key).removeValue();
                        if( cheatitem.getSharedKey() != "" ){
                            DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
                            String baseurl = "/shared-cheats/" + mGameCode;
                            DatabaseReference sharedb =  baseref.child(baseurl);
                            sharedb.child( cheatitem.getSharedKey() ).removeValue();
                        }
                    }
                })
                .setNegativeButton(R.string.no, null)
                .show();
    }

    public void Remove( int index ) {
        if( database_ == null ){
            showErrorMessage();
            return;
        }
        database_.child(_items.get(index).key).removeValue();
    }

    public void Share(  final CheatItem cheatitem ){
        if( database_ == null ){
            showErrorMessage();
            return;
        }

        if( !cheatitem.getSharedKey().equals("")  ) {
            return;
        }

        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/shared-cheats/" + mGameCode;
        DatabaseReference sharedb =  baseref.child(baseurl);

        String key = sharedb.push().getKey();
        sharedb.child(key).child("description").setValue(cheatitem.getDescription());
        sharedb.child(key).child("cheat_code").setValue(cheatitem.getCheatCode());

        database_.child(cheatitem.getKey()).child("shared_key").setValue(key);
    }


    public void UnShare(  final CheatItem cheatitem ){
        if( database_ == null ){
            showErrorMessage();
            return;
        }
        if( cheatitem.getSharedKey().equals("")  ) {
            return;
        }
        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/shared-cheats/" + mGameCode + "/" + cheatitem.getSharedKey();
        DatabaseReference sharedb =  baseref.child(baseurl);
        if( sharedb != null ){
            sharedb.removeValue();
        }
        database_.child(cheatitem.getKey()).child("shared_key").setValue("");
    }


}
