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

import com.activeandroid.query.Select;


/**
 * Created by shinya on 2017/03/04.
 */


public class CheatEditDialogStandalone extends CheatEditDialog {

    @Override
    public int LoadData( String gameid ) {
        //super.LoadData();
        Cheats = new Select().from(Cheat.class).where("gameid = ?", gameid).execute();
        if( Cheats == null ) {
            return -1;
        }
        CheatAdapter = new CheatListAdapter(this,Cheats,getActivity());
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
        return 0;
    }

    @Override
    void NewItem( String gameid, String desc, String value ){
        Cheat item = new Cheat(gameid,desc,value);
        item.save();
        CheatAdapter.add(item);
    }

    @Override
    void UpdateItem( int index , String gameid, String desc, String value ){
        Cheat item = Cheats.get(index);
        item.gameid = gameid;
        item.description = desc;
        item.cheat_code = value;
        item.save();
        CheatAdapter.notifyDataSetChanged();
    }

    @Override
    public void Remove( int index ) {
        Cheats.get(index).delete();
        LoadData( this.mGameCode );
        mListView.setAdapter(CheatAdapter);
    }
}
