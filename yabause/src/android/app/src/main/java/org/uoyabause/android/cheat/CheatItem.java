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

import com.google.firebase.database.Exclude;
import com.google.firebase.database.IgnoreExtraProperties;

import java.util.HashMap;
import java.util.Map;

@IgnoreExtraProperties
class CheatItem {
    public String key = "";
    public String gameid = "";
    public String description = "";
    public String cheat_code = "";
    public double star_count = 0;
    public boolean enable = false;
    public String shared_key = "";

    public CheatItem(){

    }

    public CheatItem( String gameid, String description, String cheat_code){
        this.gameid = gameid;
        this.description = description;
        this.cheat_code = cheat_code;
        this.enable = false;
    }

    public String getKey() { return key; }
    public String getGameid() { return gameid; }
    public String getDescription() { return description; }
    public String getCheatCode() { return cheat_code; }
    public String getSharedKey( ){ return shared_key; }

    public boolean getEnable() { return enable; }
    public double getStarCount() { return star_count; }
    public void setEnable( boolean v){ enable = v; }
    public void setSharedKey( String v){ shared_key = v; }


    @Exclude
    public Map<String, Object> toMap() {
        HashMap<String, Object> result = new HashMap<>();
        result.put("gameid", gameid);
        result.put("description", description);
        result.put("cheat_code", cheat_code);
        result.put("star_count", star_count);
        result.put("key", key);
        return result;
    }

}
