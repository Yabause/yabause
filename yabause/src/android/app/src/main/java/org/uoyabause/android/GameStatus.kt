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

import com.activeandroid.Model
import com.activeandroid.annotation.Column
import com.activeandroid.annotation.Table
import com.activeandroid.query.Select
import java.util.Date

/**
 * Created by shinya on 2016/01/04.
 */
@Table(name = "GameStatus")
class GameStatus : Model {
    @Column(name = "product_number", index = true)
    var product_number = ""

    @Column(name = "update_at")
    var update_at: Date? = null

    @Column(name = "image_url")
    var image_url = ""

    @Column(name = "rating")
    var rating = 0

    constructor() : super() {
        product_number = ""
        image_url = ""
        rating = -1
    }

    constructor(
        product_number: String,
        update_at: Date?,
        image_url: String,
        rating: Int
    ) : super() {
        this.product_number = product_number
        this.update_at = update_at
        this.image_url = image_url
        this.rating = rating
    }

    companion object {
        val lastUpdate: Date?
            get() {
                val tmp = Select()
                    .from(GameStatus::class.java)
                    .orderBy("update_at DESC")
                    .executeSingle<GameStatus>() ?: return null
                return tmp.update_at
            }
    }
}
