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

import com.activeandroid.query.Select
import org.uoyabause.android.cheat.CheatEditDialog
import org.uoyabause.android.cheat.Cheat
import org.uoyabause.android.cheat.CheatListAdapter

/**
 * Created by shinya on 2017/03/04.
 */
class CheatEditDialogStandalone : CheatEditDialog() {
    override fun LoadData(gameid: String?): Int {
        //super.LoadData();
        Cheats = Select().from(Cheat::class.java).where("gameid = ?", gameid).execute()
        if (Cheats == null) {
            return -1
        }
        CheatAdapter = CheatListAdapter(this, Cheats, requireActivity())
        if (mCheatListCode != null) {
            val cntChoice = CheatAdapter!!.count
            for (i in 0 until cntChoice) {
                CheatAdapter!!.setItemChecked(i, false)
                for (j in 0 until mCheatListCode?.size!!) {
                    if (Cheats!![i].cheat_code == mCheatListCode!![j]) {
                        CheatAdapter!!.setItemChecked(i, true)
                    }
                }
            }
        }
        return 0
    }

    override fun NewItem(gameid: String?, desc: String?, value: String?) {
        val item = Cheat(gameid, desc, value)
        item.save()
        CheatAdapter!!.add(item)
    }

    override fun UpdateItem(index: Int, gameid: String?, desc: String?, value: String?) {
        val item = Cheats!![index]
        item.gameid = gameid
        item.description = desc
        item.cheat_code = value
        item.save()
        CheatAdapter!!.notifyDataSetChanged()
    }

    override fun Remove(index: Int) {
        Cheats!![index].delete()
        LoadData(mGameCode)
        mListView!!.adapter = CheatAdapter
    }
}