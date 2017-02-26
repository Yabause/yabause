package org.uoyabause.android;


import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by shinya on 2017/02/25.
 */
class Cheat {
    public String key;
    public String description;
    public String cheat_code;
    public boolean enable;

    public Cheat(){

    }
    public Cheat( String description, String cheat_code){
        this.description = description;
        this.cheat_code = cheat_code;
        this.enable = false;
    }
}

class CheatListAdapter extends BaseAdapter implements ListAdapter {
    private ArrayList<Cheat> list = new ArrayList<Cheat>();
    private Context context;
    private CheatEditDialog parent;


    public CheatListAdapter(CheatEditDialog parent, ArrayList<Cheat> list, Context context) {
        this.list = list;
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

        //Handle buttons and add onClickListeners
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

/*
        enableBtn.setOnCheckedChangeListener( new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton toggleButton, boolean isChecked) {
                //setItemChecked(position,isChecked);
                list.get(position).enable = isChecked;
            }
        }) ;
*/
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


public class CheatEditDialog extends DialogFragment {

    private DatabaseReference mDatabase;
    private String mGameCode;
    ListView mListView;
    private ArrayList<Cheat> Cheats = new ArrayList<Cheat>();
    CheatListAdapter CheatAdapter;
    //List<Cheat> Cheats;
    String[] mCheatListCode;
    ArrayAdapter<String> adapter;

    View mContent;
    Animation inAnimation;
    Animation outAnimation;

    int editing_ = -1;

    void setGameCode( String st, String [] cheat_code  ){
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

        Cheats = new ArrayList<Cheat>();
        CheatAdapter = new CheatListAdapter(this,Cheats,getActivity());

        mDatabase = FirebaseDatabase.getInstance().getReference().child("cheats").child(mGameCode);
        if( mDatabase == null){
            return null;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
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

        LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mContent = inflater.inflate(R.layout.cheat, null);
        builder.setView(mContent);
        mListView = (ListView)mContent.findViewById(R.id.list_cheats);
        mListView.setAdapter(CheatAdapter);

        // Edit
        inAnimation = (Animation) AnimationUtils.loadAnimation(getActivity(), R.anim.in_animation);
        outAnimation= (Animation) AnimationUtils.loadAnimation(getActivity(), R.anim.out_animation);

        View edit_view = mContent.findViewById(R.id.edit_cheat_view);
        edit_view.setVisibility(View.GONE);

        // Add new item
        Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
        add_new_button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                View edit_view = mContent.findViewById(R.id.edit_cheat_view);
                edit_view.startAnimation(inAnimation);
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
                edit_view.startAnimation(outAnimation);
                edit_view.setVisibility(View.GONE);
                Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
                add_new_button.setVisibility(View.VISIBLE);

                // new item
                if( editing_ == -1 ){
                    String key = mDatabase.push().getKey();
                    mDatabase.child(key).child("description").setValue(desc.getText().toString());
                    mDatabase.child(key).child("cheat_code").setValue(ccode.getText().toString());
                }else{
                     mDatabase.child(Cheats.get(editing_).key).child("description").setValue(desc.getText().toString());
                    mDatabase.child(Cheats.get(editing_).key).child("cheat_code").setValue(ccode.getText().toString());
                }
            }
        });

        Button button_cancel = (Button)mContent.findViewById(R.id.button_edit_cheat_cancel);
        button_cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                View edit_view = mContent.findViewById(R.id.edit_cheat_view);
                edit_view.startAnimation(outAnimation);
                edit_view.setVisibility(View.GONE);
                Button add_new_button = (Button)mContent.findViewById(R.id.add_new_cheat);
                add_new_button.setVisibility(View.VISIBLE);
            }
        });
        return builder.create();
    }

    public void RemoveCheat( int pos ){

        editing_ = pos;
        new AlertDialog.Builder(getActivity())
                .setMessage("Are you sure to delete " + Cheats.get(editing_).description + "?")
                .setPositiveButton("YES", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        CheatEditDialog.this.removeCurrent();
                    }
                })
                .setNegativeButton("No", null)
                .show();
    }

    public void removeCurrent(){
        mDatabase.child(Cheats.get(editing_).key).removeValue();
    }

    public void EditCheat( int pos ){
        View edit_view = mContent.findViewById(R.id.edit_cheat_view);
        edit_view.startAnimation(inAnimation);
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