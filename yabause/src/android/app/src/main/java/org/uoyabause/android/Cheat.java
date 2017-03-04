package org.uoyabause.android;

import com.activeandroid.Model;
import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;

/**
 * Created by shinya on 2017/03/04.
 */

@Table(name = "Cheat")
public class Cheat extends Model {
    public String key;

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