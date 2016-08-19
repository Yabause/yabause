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

import android.content.Context;
import android.database.DataSetObserver;
import android.graphics.Bitmap;
import android.os.FileObserver;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import org.yabause.android.YabauseStorage;
import org.yabause.android.GameInfoManager;

import java.util.TreeSet;
import java.util.Locale;

class SavesHandler extends Handler {

    private SaveListAdapter adapter;

    SavesHandler(SaveListAdapter adapter) {
        this.adapter = adapter;
    }

    public void handleMessage(Message msg) {
        adapter.loadSlots(true);
    }
}

class SavesObserver extends FileObserver {

    private SavesHandler handler;

    SavesObserver(String path, SavesHandler handler) {
        super(path, FileObserver.DELETE | FileObserver.CLOSE_WRITE);
        this.handler = handler;
    }

    @Override
    public void onEvent(int event, String path) {
        handler.sendEmptyMessage(0);
    }
}

class SaveListAdapter extends BaseAdapter {
    private YabauseStorage storage;
    private Context context;
    private int[] slots;
    private int emptyslot;
    private String itemnum;
    private SavesHandler handler;
    private SavesObserver observer;

    SaveListAdapter(Context ctx, String game) {
        storage = YabauseStorage.getStorage();
        GameInfoManager gim = new GameInfoManager(ctx);
        GameInfo gi = gim.gameInfo(game);
        itemnum = gi.getItemnum();
        loadSlots(false);
        context = ctx;
        handler = new SavesHandler(this);
        observer = new SavesObserver(storage.getSavesPath(), handler);
        observer.startWatching();
    }

    public int getCount() {
        return slots.length;
    }

    public Object getItem(int position) {
        return slots[position];
    }

    public long getItemId(int position) {
        return slots[position];
    }

    public int getItemViewType(int position) {
        return 0;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        View layout;
        TextView name;
        ImageView screenshot;

        if (convertView != null) {
            layout = convertView;
        } else {
            LayoutInflater inflater = (LayoutInflater) context.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
            layout = inflater.inflate(R.layout.save_item, parent, false);
        }

        name = (TextView) layout.findViewById(R.id.save_name);
        name.setText("Slot " + slots[position]);

        screenshot = (ImageView) layout.findViewById(R.id.save_screenshot);
        Bitmap.Config conf = Bitmap.Config.ARGB_8888;
        Bitmap bitmap = Bitmap.createBitmap(320, 224, conf);
        YabauseRunnable.stateSlotScreenshot(storage.getSavesPath(), itemnum, slots[position], bitmap);
        screenshot.setImageBitmap(bitmap);

        return layout;
    }

    public int getViewTypeCount() {
        return 1;
    }

    public boolean hasStableIds() {
        return false;
    }

    public boolean isEmpty() {
        return false;
    }

    public boolean areAllItemsEnabled() {
        return true;
    }

    public boolean isEnabled(int position) {
        return true;
    }

    public int getEmptySlot() {
        return emptyslot;
    }

    public void deleteSlot(int slot) {
        String[] saves = new String[1];
        saves[0] = String.format(Locale.US, "%s_%03d.yss", itemnum, slot);
        storage.deleteGameSaves(saves);
    }

    public void deleteSlots(int[] slots) {
        String[] saves = new String[slots.length];
        int i = 0;
        for(int slot : slots) {
            saves[i++] = String.format(Locale.US, "%s_%03d.yss", itemnum, slot);
        }
        storage.deleteGameSaves(saves);
    }

    public void loadSlots(boolean notify) {
        String[] savefiles = storage.getGameSaves(itemnum);
        TreeSet<Integer> slotset = new TreeSet<Integer>();
        for(String file : savefiles) {
            int slot = Integer.parseInt(file.substring(itemnum.length() + 1, file.length() - ".yss".length()));
            slotset.add(slot);
        }
        emptyslot = 0;
        while(slotset.contains(emptyslot)) emptyslot++;
        slots = new int[slotset.size()];
        int i = 0;
        for(int slot : slotset) {
            slots[i] = slot;
            i++;
        }

        if (notify) {
            notifyDataSetChanged();
        }
    }
}
