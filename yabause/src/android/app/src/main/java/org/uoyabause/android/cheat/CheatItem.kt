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

import com.google.firebase.database.Exclude
import com.google.firebase.database.IgnoreExtraProperties

@IgnoreExtraProperties
data class CheatItem(
    var key: String = "",
    var gameid: String = "",
    var description: String = "",
    var cheat_code: String = "",
    var star_count: Double = 0.0,
) {
    var enable: Boolean = false
    var sharedKey = ""

    @Exclude
    fun toMap(): Map<String, Any?> {
        return mapOf(
            "gameid" to gameid,
            "description" to description,
            "cheat_code" to cheat_code,
            "star_count" to star_count,
            "key" to key
        )
/*
        val result = HashMap<String, Any>()
        result["gameid"] = this.gameid!!
        result["description"] = description!!
        result["cheat_code"] = cheat_code!!
        result["star_count"] = star_count!!
        result["key"] = this.key!!
        return result
 */

    }


}