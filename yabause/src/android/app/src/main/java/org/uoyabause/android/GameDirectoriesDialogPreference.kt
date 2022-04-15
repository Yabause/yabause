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
package org.uoyabause.android

import org.uoyabause.android.GameDirectoriesDialogPreference.DirListChangeListener
import android.app.Activity
import android.content.Context
import android.util.AttributeSet
import androidx.preference.DialogPreference

/**
 * Created by shinya on 2016/01/07.
 */
class GameDirectoriesDialogPreference : DialogPreference {
    //    private YabauseSettings _context = null;
    //public GameDirectoriesDialogPreference(Context context) {
    //    super(context);
    //    InitObjects(context);
    //}
    interface DirListChangeListener {
        fun onChangeDir(isChange: Boolean?)
    }

    var listener: DirListChangeListener? = null
    fun setDirListChangeListener(l: DirListChangeListener?) {
        listener = l
    }

    constructor(context: Context?, attrs: AttributeSet?) : super(
        context!!, attrs) {        //InitObjects(context);
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
    fun save(resultstring: String) {
        val data = getPersistedString("err")
        if (data != resultstring) {
            //_context.setDireListChangeStatus(true);
        }
        persistString(resultstring)
        if (listener != null) {
            listener!!.onChangeDir(true)
        }
    }

    val data: String
        get() = getPersistedString("err")

    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(
        context!!, attrs, defStyle) {
        //InitObjects(context);
        if (listener != null) {
            listener!!.onChangeDir(false)
        }
    }

    fun setActivity(a: Activity?) {
        //_context = (YabauseSettings)a;
        //_context.setDireListChangeStatus(false);
    } //void InitObjects( Context context) {
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