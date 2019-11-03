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

import com.activeandroid.query.Delete;
import com.activeandroid.query.Select;
import com.activeandroid.util.Log;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;

import java.util.ArrayList;
import java.util.List;


public class CheatConverter {

    public boolean hasOldVersion() {
        List<Cheat> Cheats;
        try {
            Cheats = new Select().from(Cheat.class).execute();
        }catch(Exception e){
            return false;
        }
        if( Cheats == null ) {
            return false;
        }
        if( Cheats.size() == 0 ){
            return false;
        }
        return true;
    }

    public int execute() {
        List<Cheat> Cheats;
        DatabaseReference database_;

        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            return -1;
        }

        Cheats = new Select().from(Cheat.class).execute();
        if( Cheats == null && Cheats.size() == 0 ) {
            return -1;
        }

        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/user-posts/" + auth.getCurrentUser().getUid()  + "/cheat/";
        database_ =  baseref.child(baseurl);
        if( database_ == null ){
            return -1;
        }

        for( Cheat cheat : Cheats ){
            String key = database_.child(cheat.gameid).push().getKey();
            database_.child(cheat.gameid).child(key).child("description").setValue(cheat.description);
            database_.child(cheat.gameid).child(key).child("cheat_code").setValue(cheat.cheat_code);
            Log.d("CheatConverter", "game:" + cheat.gameid + " desc:" +  cheat.description + " code:" + cheat.cheat_code );
        }

        new Delete().from(Cheat.class).execute();

        return 0;
    }
}
