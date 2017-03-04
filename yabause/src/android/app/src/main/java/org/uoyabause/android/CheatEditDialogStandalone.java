package org.uoyabause.android;

import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;
import com.activeandroid.query.Select;

import org.uoyabause.android.CheatEditDialog;

import java.util.ArrayList;

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
