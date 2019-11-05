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


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.core.content.ContextCompat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import org.uoyabause.android.Yabause;
import org.uoyabause.uranus.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by shinya on 2017/02/25.
 */

class CheatListAdapter extends BaseAdapter implements ListAdapter {
    private List<Cheat> list = new ArrayList<Cheat>();
    private Context context;
    private CheatEditDialog parent;


    public CheatListAdapter(CheatEditDialog parent, List<Cheat> list, Context context) {
        if( list != null ) {
            this.list = list;
        }
        this.parent = parent;
        this.context = context;
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Object getItem(int pos) {
        return list.get(pos);
    }

    @Override
    public long getItemId(int pos) {
        return pos;
        //just return 0 if your list items do not have an Id variable.
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.cheat_item, null);
        }

        //Handle TextView and display string from your list
        TextView listItemText = (TextView)view.findViewById(R.id.text_description);
        listItemText.setText(list.get(position).description);

        if( list.get(position).enable ){
            listItemText.setTextColor( ContextCompat.getColor(context,R.color.colorAccent) );
        }else{
            listItemText.setTextColor( ContextCompat.getColor(context,R.color.disable) );
        }

        //Handle buttons and add onClickListeners
/*
        Button deleteBtn = (Button)view.findViewById(R.id.button_delete_cheat);
        Button addBtn = (Button)view.findViewById(R.id.button_cheat_edit);
        ToggleButton enableBtn = (ToggleButton)view.findViewById(R.id.toggleButton_enable_cheat);
        enableBtn.setChecked( list.get(position).enable );


        enableBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                list.get(position).enable = !(list.get(position).enable);
            }
        });

        deleteBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                CheatListAdapter.this.parent.RemoveCheat(position);
            }
        });
        addBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                CheatListAdapter.this.parent.EditCheat(position);
            }
        });
*/
        return view;
    }

    void add( Cheat cheat ){
        list.add(cheat);
        notifyDataSetChanged();
    }

    void setItemChecked( int position, boolean checked ){
        list.get(position).enable = checked;
        notifyDataSetChanged();
    }
}


public class CheatEditDialog extends DialogFragment implements AdapterView.OnItemClickListener {

    private DatabaseReference mDatabase;
    protected String mGameCode;
    ListView mListView;
    protected List<Cheat> Cheats = new ArrayList<Cheat>();
    CheatListAdapter CheatAdapter;
    //List<Cheat> Cheats;
    String[] mCheatListCode;
    ArrayAdapter<String> adapter;

    View mContent;

    int editing_ = -1;

    public void setGameCode( String st, String [] cheat_code  ){
        mGameCode = st;
        mCheatListCode = cheat_code;
    }

    @Override
    public void onDismiss(final DialogInterface dialog) {
        super.onDismiss(dialog);

        int cntChoice = CheatAdapter.getCount();
        int cindex = 0;
        for (int i = 0; i < cntChoice; i++) {
            if (Cheats.get(i).enable ) {
                cindex++;
            }
        }

        mCheatListCode = new String[cindex];
        cindex = 0;
        for (int i = 0; i < cntChoice; i++) {
            if (Cheats.get(i).enable ) {
                mCheatListCode[cindex] = Cheats.get(i).cheat_code;
                cindex++;
            }
        }


        Yabause activity = (Yabause) getActivity();
        activity.updateCheatCode(mCheatListCode);
    }



    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {

        if( mGameCode == null ){
            return null;
        }

        if( LoadData(mGameCode) == -1 ) {
            return null;
        }

        if( Cheats == null ){
            return null;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mContent = inflater.inflate(R.layout.cheat, null);
        builder.setView(mContent);
        mListView = (ListView)mContent.findViewById(R.id.list_cheats);
        mListView.setAdapter(CheatAdapter);
        mListView.setOnItemClickListener(this);
        //getActivity().registerForContextMenu(mListView);

        View edit_view = mContent.findViewById(R.id.edit_cheat_view);
        edit_view.setVisibility(View.GONE);

        // Add new item
        Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
        add_new_button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                View edit_view = mContent.findViewById(R.id.edit_cheat_view);
                edit_view.setVisibility(View.VISIBLE);
                Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
                add_new_button.setVisibility(View.GONE);
                View desc = mContent.findViewById(R.id.editText_cheat_desc);
                desc.requestFocus();
                InputMethodManager imm = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(desc, InputMethodManager.SHOW_IMPLICIT);
                editing_ = -1;
            }
        });

        // Apply Button
        Button button_apply = (Button)mContent.findViewById(R.id.button_cheat_edit_apply);
        button_apply.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                EditText  desc = (EditText )mContent.findViewById(R.id.editText_cheat_desc);
                EditText  ccode = (EditText)mContent.findViewById(R.id.editText_code);

                View edit_view = mContent.findViewById(R.id.edit_cheat_view);
                edit_view.setVisibility(View.GONE);
                Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
                add_new_button.setVisibility(View.VISIBLE);

                // new item
                if( editing_ == -1 ){
                    NewItem( mGameCode, desc.getText().toString(), ccode.getText().toString() );
                }else{
                    UpdateItem( editing_ , mGameCode, desc.getText().toString(), ccode.getText().toString() );
                }

            }
        });

        Button button_cancel = (Button)mContent.findViewById(R.id.button_edit_cheat_cancel);
        button_cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                View edit_view = mContent.findViewById(R.id.edit_cheat_view);
                edit_view.setVisibility(View.GONE);
                Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
                add_new_button.setVisibility(View.VISIBLE);
            }
        });

        ((EditText)mContent.findViewById(R.id.editText_code)).setOnEditorActionListener(
                new EditText.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                        //For ShieldTV
                        if ( actionId == EditorInfo.IME_ACTION_NEXT) {
                            EditText edit_view = (EditText)mContent.findViewById(R.id.editText_code);
                            String currentstr = edit_view.getText().toString();
                            currentstr += System.lineSeparator();
                            edit_view.setText(currentstr);
                            return true;
                        }
                        // Return true if you have consumed the action, else false.
                        return false;
                    }
                });

        return builder.create();
    }

    public static class MenuDialogFragment extends DialogFragment {

        public boolean isEnable = false;
        public int position;
        public CheatEditDialog parent;
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            CharSequence[] items = {"Enable", "Edit", "Delete"};

            final Activity activity = getActivity();

            items[0] = getResources().getString(R.string.enable);
            items[1] = getResources().getString(R.string.edit);
            items[2] = getResources().getString(R.string.delete);

            if( parent.Cheats.get(position).local == false ){
                CharSequence[]  clouditems = {"Enable"};
                clouditems[0] = getResources().getString(R.string.enable);
                items = clouditems;
            }

            if( isEnable == true ){
                items[0] = getResources().getString(R.string.disable);
            }

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setItems(items, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    switch (which) {
                        case 0:
                            parent.Cheats.get(position).enable = !isEnable;
                            parent.CheatAdapter.notifyDataSetChanged();
                            break;
                        case 1:
                            parent.EditCheat(position);
                            break;
                        case 2:
                            parent.RemoveCheat(position);
                            break;
                        default:
                            break;
                    }
                }
            });
            return builder.create();
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id){
        // ダイアログを表示する
        MenuDialogFragment newFragment = new MenuDialogFragment();
        newFragment.isEnable = Cheats.get(position).enable;
        newFragment.position = position;
        newFragment.parent = this;
        newFragment.show(getFragmentManager(), "cheat_edit_menu");
    }

    void NewItem( String gameid, String desc, String value ){
        String key = mDatabase.push().getKey();
        mDatabase.child(key).child("description").setValue(desc);
        mDatabase.child(key).child("cheat_code").setValue(value);
    }

    void UpdateItem( int index , String gameid, String desc, String value ){
        mDatabase.child(Cheats.get(index).key).child("description").setValue(desc);
        mDatabase.child(Cheats.get(index).key).child("cheat_code").setValue(value);
    }

    public void Remove( int index ) {
        mDatabase.child(Cheats.get(index).key).removeValue();
    }

    public int LoadData( String gameid ) {
        Cheats = new ArrayList<Cheat>();
        CheatAdapter = new CheatListAdapter(this,Cheats,getActivity());
        mDatabase = FirebaseDatabase.getInstance().getReference().child("cheats").child(mGameCode);
        if( mDatabase == null){
            return -1;
        }

        ValueEventListener cheatDataListener = new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {

                if( dataSnapshot.hasChildren() ) {
                    Cheats.clear();
                    for (DataSnapshot child : dataSnapshot.getChildren()) {
                        //ids.add(child.getKey());
                        Cheat newitem = new Cheat();
                        newitem.key =  (String)child.getKey();
                        newitem.description = (String)child.child("description").getValue();
                        newitem.cheat_code = (String)child.child("cheat_code").getValue();
                        newitem.enable = false;
                        newitem.local = false;
                        CheatAdapter.add(newitem);
                    }

                    if( mCheatListCode != null ) {
                        int cntChoice = CheatAdapter.getCount();
                        for (int i = 0; i < cntChoice; i++) {
                            CheatAdapter.setItemChecked(i, false);
                            for (int j = 0; j < mCheatListCode.length; j++) {
                                if (Cheats.get(i).cheat_code.equals(mCheatListCode[j])) {
                                    CheatAdapter.setItemChecked(i, true);
                                }
                            }
                        }
                    }
                }else{
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.getKey() );
                }
            }
            @Override
            public void onCancelled(DatabaseError databaseError) {
            }
        };
        mDatabase.addValueEventListener(cheatDataListener);
        return 0;
    }

    public void RemoveCheat( int pos ){

        editing_ = pos;
        new AlertDialog.Builder(getActivity())
                .setMessage("Are you sure to delete " + Cheats.get(editing_).description + "?")
                .setPositiveButton("YES", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        CheatEditDialog.this.Remove( editing_ );
                    }
                })
                .setNegativeButton("No", null)
                .show();
    }

    public void EditCheat( int pos ){
        View edit_view = mContent.findViewById(R.id.edit_cheat_view);
        edit_view.setVisibility(View.VISIBLE);
        Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
        add_new_button.setVisibility(View.GONE);
        EditText cheat = (EditText)mContent.findViewById(R.id.editText_code);
        cheat.setText(Cheats.get(pos).cheat_code);
        cheat.requestFocus();
        EditText desc = (EditText)mContent.findViewById(R.id.editText_cheat_desc);
        desc.setText(Cheats.get(pos).description);
        InputMethodManager imm = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(cheat, InputMethodManager.SHOW_IMPLICIT);
        editing_ = pos;

    }
}