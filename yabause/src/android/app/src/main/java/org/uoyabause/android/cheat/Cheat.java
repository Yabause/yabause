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

import com.activeandroid.Model;
import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;

/**
 * Created by shinya on 2017/03/04.
 */

@Table(name = "Cheat")
public class Cheat extends Model {
    public String key;
    public boolean local = true;

    @Column(name = "gameid")
    public String gameid;

    @Column(name = "description")
    public String description;

    @Column(name = "cheat_code")
    public String cheat_code;
    public boolean enable = false;

    public Cheat(){

    }
    public Cheat( String gameid, String description, String cheat_code){
        this.gameid = gameid;
        this.description = description;
        this.cheat_code = cheat_code;
        this.enable = false;
    }
}