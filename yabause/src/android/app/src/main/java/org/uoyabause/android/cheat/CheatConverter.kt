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
package org.uoyabause.android.cheat

import com.activeandroid.Model
import com.activeandroid.query.Delete
import com.activeandroid.query.Select
import com.activeandroid.util.Log
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import java.lang.Exception

class CheatConverter {
    fun hasOldVersion(): Boolean {
        val Cheats: List<Cheat>?
        Cheats = try {
            Select().from(Cheat::class.java).execute()
        } catch (e: Exception) {
            return false
        }
        if (Cheats == null) {
            return false
        }
        return if (Cheats.size == 0) {
            false
        } else true
    }

    fun execute(): Int {
        val Cheats: List<Cheat>?
        val database_: DatabaseReference
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser == null) {
            return -1
        }
        Cheats = Select().from(Cheat::class.java).execute()
        if (Cheats == null || Cheats.isEmpty()) {
            return -1
        }
        val baseref = FirebaseDatabase.getInstance().reference
        val baseurl = "/user-posts/" + auth.currentUser!!.uid + "/cheat/"
        database_ = baseref.child(baseurl)
        for (cheat in Cheats) {
            val key = database_.child(cheat.gameid!!).push().key
            database_.child(cheat.gameid!!).child(key!!).child("description")
                .setValue(cheat.description)
            database_.child(cheat.gameid!!).child(key).child("cheat_code")
                .setValue(cheat.cheat_code)
            Log.d("CheatConverter",
                "game:" + cheat.gameid + " desc:" + cheat.description + " code:" + cheat.cheat_code)
        }
        Delete().from(Cheat::class.java).execute<Model>()
        return 0
    }
}
