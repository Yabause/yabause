package org.uoyabause.android;

import com.activeandroid.Model;
import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;
import com.activeandroid.query.Select;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by shinya on 2015/12/30.
 */
@Table(name = "GameInfo")
public class GameInfo extends Model {

    @Column(name = "file_path", index = true)
    public String file_path;

    @Column(name = "iso_file_path", index = true)
    public String iso_file_path;


    @Column(name = "game_title")
    public String game_title;

    @Column(name = "maker_id")
    public String maker_id;

    @Column(name = "product_number", index = true)
    public String product_number;

    @Column(name = "version")
    public String version;

    @Column(name = "release_date")
    public String release_date;

    @Column(name = "device_infomation")
    public String device_infomation;

    @Column(name = "area")
    public String area;

    @Column(name = "input_device")
    public String input_device;

    static GameInfo getFromFileName(String file_path){
        return new Select()
                .from(GameInfo.class)
                .where("iso_file_path = ?", file_path )
                .executeSingle();
    }

    static GameInfo  getFromInDirectFileName(String file_path){
        return new Select()
                .from(GameInfo.class)
                .where("file_path = ?", file_path )
                .executeSingle();
    }

    static GameInfo genGameInfoFromCUE( String file_path ){
        File file = new File(file_path);
        String dirName = file.getParent();
        String iso_file_name = "";
        GameInfo tmp = null;
        try {
            FileReader filereader = new FileReader(file);
            BufferedReader br = new BufferedReader(filereader);

            String str = br.readLine();
            while(str != null){
                //System.out.println(str);
                Pattern p = Pattern.compile("FILE \"(.*)\"");
                Matcher m = p.matcher(str);
                if( m.find() ){
                    iso_file_name = m.group(1);
                    break;
                }
                str = br.readLine();
            }
            br.close();

            if( iso_file_name.equals("") ){
                return null; // bad file format;
            }

        }catch(FileNotFoundException e){
            System.out.println(e);
            return null;
        }catch(IOException e){
            System.out.println(e);
            return null;
        }

        iso_file_name = dirName + File.separator + iso_file_name;

        try {

            byte[] buff = new byte[0xFF];
            DataInputStream dataInStream = new DataInputStream(
                    new BufferedInputStream( new FileInputStream(iso_file_name)));
            dataInStream.skipBytes(0x10);
            dataInStream.read(buff, 0x0, 0xFF);
            dataInStream.close();

            String header = new String(buff);
            tmp = new GameInfo();
            tmp.file_path = file_path;
            tmp.iso_file_path = iso_file_name;
            tmp.maker_id = header.substring( 0x10,0x20 );
            tmp.maker_id = tmp.maker_id.trim();
            tmp.product_number = header.substring( 0x20,0x2A );
            tmp.product_number = tmp.product_number.trim();
            tmp.version = header.substring( 0x2A,0x30 );
            tmp.version = tmp.version.trim();
            tmp.release_date = header.substring( 0x30,0x38 );
            tmp.release_date = tmp.release_date.trim();
            tmp.area = header.substring( 0x40,0x4A );
            tmp.area = tmp.area.trim();
            tmp.input_device =  header.substring( 0x50,0x60 );
            tmp.input_device = tmp.input_device.trim();
            tmp.device_infomation =  header.substring( 0x38,0x40 );
            tmp.device_infomation = tmp.device_infomation.trim();
            tmp.game_title = header.substring( 0x60,0xD0 );
            tmp.game_title = tmp.game_title.trim();

        }catch(FileNotFoundException e){
            System.out.println(e);
            return null;
        }catch(IOException e){
            System.out.println(e);
            return null;
        }
        return tmp;
    }

    static GameInfo genGameInfoFromMDS( String file_path ){
        return null;
    }

    static GameInfo genGameInfoFromCCD( String file_path ){
        return null;
    }

    static GameInfo genGameInfoFromIso( String file_path ){
        return null;
    }
}
