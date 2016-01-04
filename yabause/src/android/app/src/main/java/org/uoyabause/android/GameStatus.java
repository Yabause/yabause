package org.uoyabause.android;

import com.activeandroid.Model;
import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;
import com.activeandroid.query.Select;

import java.util.Date;

/**
 * Created by shinya on 2016/01/04.
 */
@Table(name = "GameStatus")
public class GameStatus extends Model {

    @Column(name = "product_number", index = true)
    public String product_number ="";

    @Column(name = "update_at")
    public Date update_at;

    @Column(name = "image_url")
    public String image_url ="";

    @Column(name = "rating")
    public int rating = 0;

    public GameStatus(  ){
        super();
        this.product_number ="";
        this.image_url ="";
        this.rating = -1;
    }

    static Date getLastUpdate(){
        GameStatus tmp =  new Select()
                .from(GameStatus.class)
                .orderBy("update_at DESC")
                .executeSingle();
        if( tmp == null)
            return null;
        return tmp.update_at;
    }

    public GameStatus( String product_number, Date update_at,String image_url, int rating  ){
        super();
        this.product_number = product_number;
        this.update_at = update_at;
        this.image_url = image_url;
        this.rating = rating;
    }

}
