package org.uoyabause.android;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.PreferenceDialogFragmentCompat;

import org.devmiyax.yabasanshiro.R;
import org.uoyabause.android.tv.GameSelectFragment;

import java.io.File;
import java.util.ArrayList;

class DirectoryListAdapter extends BaseAdapter implements ListAdapter {

    private Context context;
    private ArrayList<String> _directoryList = new ArrayList<String>();

    ArrayList<String> getList(){ return _directoryList; }

    public DirectoryListAdapter(Context context) {
        this.context = context;
    }

    public void setDirectorytList(ArrayList<String> directoryList) {
        this._directoryList = directoryList;
    }

    @Override
    public int getCount() {
        return _directoryList.size();
    }

    @Override
    public Object getItem(int position) {
        return _directoryList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position; //_directoryList.get(position).getId();
    }

    public void addDirectory( String path ){
        _directoryList.add(path);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.game_directories_listitem, null);
        }
        ((TextView)view.findViewById(R.id.game_directory_item)).setText(_directoryList.get(position));

        Button deleteBtn = (Button)view.findViewById(R.id.button_delete);
        deleteBtn.setTag(position);
        deleteBtn.setOnClickListener(new View.OnClickListener(){

            @Override
            public void onClick(View v) {
                Integer position = (Integer)v.getTag();
                if( position != null ){
                    _directoryList.remove(position.intValue()); //or some other task
                    notifyDataSetChanged();
                    GameSelectFragment.refresh_level = 3;
                }
            }
        });

        return view;
    }
}

public class GameDirectoriesDialogFragment extends PreferenceDialogFragmentCompat implements View.OnClickListener , FileDialog.DirectorySelectedListener {

    DirectoryListAdapter adapter;
    ListView listView;

    public static GameDirectoriesDialogFragment newInstance(String key) {
        GameDirectoriesDialogFragment fragment = new GameDirectoriesDialogFragment();
        Bundle b = new Bundle(1);
        b.putString(PreferenceDialogFragmentCompat.ARG_KEY, key);
        fragment.setArguments(b);
        return fragment;
    }

    @Override
    public View onCreateDialogView(Context context){
        View v =  super.onCreateDialogView(context);
        return v;
    }

    @Override
    protected void onPrepareDialogBuilder( AlertDialog.Builder builder) {
        super.onPrepareDialogBuilder(builder);
        LayoutInflater inflater = (LayoutInflater)getActivity().getLayoutInflater();
        View view = inflater.inflate(R.layout.game_directories, null, false);
        onBindDialogView(view);
        builder.setView(view);
    }

    @Override
    public void onDialogClosed(boolean positiveResult) {
        if(positiveResult){
            String resultstring = "";
            ArrayList<String> list = adapter.getList();
            for( int i=0; i< list.size(); i++ ){
                resultstring += list.get(i);
                resultstring += ";";
            }
            GameDirectoriesDialogPreference is = (GameDirectoriesDialogPreference)getPreference();
            is.save(resultstring);
        }
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        ArrayList<String> list;
        list = new ArrayList<String>();

        GameDirectoriesDialogPreference is = (GameDirectoriesDialogPreference)getPreference();
        String data = is.getData();
        if (data.equals("err")) {
            YabauseStorage yb = YabauseStorage.getStorage();
            list.add(yb.getGamePath());
        } else {
            String[] paths = data.split(";", 0);
            for (int i = 0; i < paths.length; i++) {
                list.add(paths[i]);
            }
        }
        listView = (ListView) view.findViewById(R.id.listView);
        adapter = new DirectoryListAdapter(requireActivity());
        adapter.setDirectorytList(list);
        listView.setAdapter(adapter);
        View button = (Button) view.findViewById(R.id.button_add);
        button.setOnClickListener(this);
    }

    public void onClick(View v) {
        FileDialog fd = new FileDialog(requireActivity(),"");
        fd.setSelectDirectoryOption(true);
        fd.addDirectoryListener(this);
        fd.showDialog();
    }

    @Override
    public void directorySelected(File directory){
        adapter.addDirectory(directory.getAbsolutePath());
        adapter.notifyDataSetChanged();
        GameSelectFragment.refresh_level = 3;
    }
}
