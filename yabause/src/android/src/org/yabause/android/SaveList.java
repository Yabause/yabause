/*  Copyright 2016 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

package org.yabause.android;

import java.util.TreeSet;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.GridView;

import org.yabause.android.SaveListAdapter;
import org.yabause.android.SaveListModeListener;
import org.yabause.android.Yabause;

public class SaveList extends Activity implements OnItemClickListener
{
    private static final String TAG = "Yabause";
    private String game;
    private SaveListAdapter adapter;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        game = intent.getStringExtra("org.yabause.android.FileName");

        SaveListAdapter adapter = new SaveListAdapter(this, game);
        this.adapter = adapter;

        setContentView(R.layout.save_list);

        GridView listView = (GridView) findViewById(R.id.save_listview);
        listView.setAdapter(this.adapter);
        listView.setOnItemClickListener(this);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            listView.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE_MODAL);
            listView.setMultiChoiceModeListener(new SaveListModeListener(adapter));
        } else {
            registerForContextMenu(listView);
        }
    }

    public void onItemClick(AdapterView<?> l, View v, int position, long id) {

        Integer slot = (Integer) l.getItemAtPosition(position);

        Intent intent = new Intent(this, Yabause.class);
        intent.putExtra("org.yabause.android.FileName", game);
        intent.putExtra("org.yabause.android.Slot", slot);
        startActivity(intent);
    }

    public void onStartWithAutosave(View view)
    {
        Intent intent = new Intent(this, Yabause.class);
        intent.putExtra("org.yabause.android.FileName", game);
        intent.putExtra("org.yabause.android.Slot", adapter.getEmptySlot());
        startActivity(intent);
    }

    public void onStartWithoutAutosave(View view)
    {
        Intent intent = new Intent(this, Yabause.class);
        intent.putExtra("org.yabause.android.FileName", game);
        startActivity(intent);
    }

    // Compatibility for Android < 3
    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
    {
        super.onCreateContextMenu(menu, v, menuInfo);
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.save_list, menu);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item)
    {
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
        switch (item.getItemId()) {
            case R.id.delete_save:
                adapter.deleteSlot((int) info.id);
                return true;
            default:
                return super.onContextItemSelected(item);
        }
    }
}
