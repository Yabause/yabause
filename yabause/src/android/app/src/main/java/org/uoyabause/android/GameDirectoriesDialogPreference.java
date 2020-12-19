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

import android.app.Activity;
import android.content.Context;
import androidx.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;

import org.uoyabause.android.tv.GameSelectFragment;
import org.devmiyax.yabasanshiro.R;

import java.io.File;
import java.util.ArrayList;

/**
 * Created by shinya on 2016/01/07.
 */
public class GameDirectoriesDialogPreference extends DialogPreference {

//    private YabauseSettings _context = null;

    //public GameDirectoriesDialogPreference(Context context) {
    //    super(context);
    //    InitObjects(context);
    //}

    interface DirListChangeListener {
        void onChangeDir( Boolean isChange );
    }

    DirListChangeListener listener = null;

    void setDirListChangeListener( DirListChangeListener l ){
        listener = l;
    }

    public GameDirectoriesDialogPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        //InitObjects(context);
    }

/*
    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if(positiveResult){
            String resultstring = "";
            ArrayList<String> list = adapter.getList();
            for( int i=0; i< list.size(); i++ ){
                resultstring += list.get(i);
                resultstring += ";";
            }
            String data = getPersistedString ("err");
            if( !data.equals(resultstring)){
                _context.setDireListChangeStatus(true);
            }
            persistString(resultstring);
        }
        super.onDialogClosed(positiveResult);
    }
*/

    public void save(String resultstring ){
        String data = getPersistedString ("err");
        if( !data.equals(resultstring)){
            //_context.setDireListChangeStatus(true);
        }
        persistString(resultstring);
        if( listener != null ){
            listener.onChangeDir(true);
        }
    }

    public String getData(){
        return getPersistedString("err");
    }

    public GameDirectoriesDialogPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        //InitObjects(context);
        if( listener != null ){
            listener.onChangeDir(false);
        }
    }

    public void setActivity( Activity a){
        //_context = (YabauseSettings)a;
        //_context.setDireListChangeStatus(false);
    }

    //void InitObjects( Context context) {

        //setDialogLayoutResource(R.layout.game_directories);
    //}
/*
    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        ArrayList<String> list;
        list = new ArrayList<String>();
        String data = getPersistedString("err");
        if (data.equals("err")) {
            //YabauseStorage yb = YabauseStorage.storage;
            //list.add(yb.getGamePath());
            YabauseStorage yb = YabauseStorage.getStorage();
            list.add(yb.getGamePath());
        } else {
            String[] paths = data.split(";", 0);
            for (int i = 0; i < paths.length; i++) {
                list.add(paths[i]);
            }
        }

        listView = (ListView) view.findViewById(R.id.listView);
        adapter = new DirectoryListAdapter(_context);
        adapter.setDirectorytList(list);
        listView.setAdapter(adapter);

        View button = (Button) view.findViewById(R.id.button_add);
        button.setOnClickListener(this);

    }
*/
/*
    public void onClick(View v) {
        FileDialog fd = new FileDialog(_context,"");
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
*/
}
