/*  Copyright 2017 devMiyax(smiyaxdev@gmail.com)

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

package org.uoyabause.android;

import android.content.Context;
import android.util.Log;

import com.activeandroid.Model;
import com.activeandroid.annotation.Column;
import com.activeandroid.annotation.Table;
import com.activeandroid.query.Delete;
import com.activeandroid.query.Select;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.net.Authenticator;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.PasswordAuthentication;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by shinya on 2015/12/30.
 */
@Table(name = "GameInfo")
public class GameInfo extends Model {

    static {
        System.loadLibrary("yabause_native");
    }

    public GameInfo() {
        super();
    }

    @Column(name = "file_path", index = true)
    public String file_path ="";

    @Column(name = "iso_file_path", index = true)
    public String iso_file_path ="";


    @Column(name = "game_title")
    public String game_title ="";

    @Column(name = "maker_id")
    public String maker_id ="";

    @Column(name = "product_number", index = true)
    public String product_number ="";

    @Column(name = "version")
    public String version ="";

    @Column(name = "release_date")
    public String release_date ="";

    @Column(name = "device_infomation")
    public String device_infomation ="";

    @Column(name = "area")
    public String area ="";

    @Column(name = "input_device")
    public String input_device ="";

    @Column(name = "last_playdate")
    public Date last_playdate;

    @Column(name = "update_at")
    public Date update_at;

    @Column(name = "image_url")
    public String image_url ="";

    @Column(name = "rating")
    public int rating = 0;

    @Column(name = "lastplay_date")
    public Date lastplay_date;

    static public GameInfo getFromFileName(String file_path) {
        Log.d("GameInfo :", file_path);
        return new Select()
                .from(GameInfo.class)
                .where("file_path = ?", file_path)
                .executeSingle();
    }

    static public  GameInfo getFromInDirectFileName(String file_path) {
        Log.d("GameInfo direct:", file_path);
        file_path = file_path.toUpperCase();
        return new Select()
                .from(GameInfo.class)
                .where("iso_file_path = ?", file_path)
                .executeSingle();
    }

    static public void deleteAll(){
        new Delete().from(GameInfo.class).execute();
    }

    static public  GameInfo genGameInfoFromCUE(String file_path) {
        File file = new File(file_path);
        String dirName = file.getParent();
        String iso_file_name = "";
        GameInfo tmp = null;
        try {
            FileReader filereader = new FileReader(file);
            BufferedReader br = new BufferedReader(filereader);

            String str = br.readLine();
            while (str != null) {
                //System.out.println(str);
                Pattern p = Pattern.compile("FILE \"(.*)\"");
                Matcher m = p.matcher(str);
                if (m.find()) {
                    iso_file_name = m.group(1);
                    break;
                }
                str = br.readLine();
            }
            br.close();

            if (iso_file_name.equals("")) {
                return null; // bad file format;
            }

        } catch (FileNotFoundException e) {
            System.out.println(e);
            return null;
        } catch (IOException e) {
            System.out.println(e);
            return null;
        }

        iso_file_name = dirName + File.separator + iso_file_name;

        tmp = genGameInfoFromIso(iso_file_name);
        if( tmp == null )
            return null;

        tmp.file_path = file_path;
        tmp.iso_file_path = iso_file_name.toUpperCase();

        return tmp;
    }

    static  public  GameInfo genGameInfoFromMDS(String file_path) {

        String iso_file_name = file_path.replace(".mds",".mdf");
        GameInfo tmp = null;
        tmp = genGameInfoFromIso(iso_file_name);
        if( tmp == null )
            return null;

        tmp.file_path = file_path;
        tmp.iso_file_path = iso_file_name.toUpperCase();

        // read mdf
        return tmp;
    }

    static  public GameInfo genGameInfoFromCCD(String file_path) {

        String iso_file_name = file_path.replace(".ccd",".img");
        GameInfo tmp = null;
        tmp = genGameInfoFromIso(iso_file_name);
        if( tmp == null )
            return null;

        tmp.file_path = file_path;
        tmp.iso_file_path = iso_file_name.toUpperCase();

        return tmp;
    }

    static GameInfo getGimeInfoFromBuf( String file_path, String header ){
        GameInfo tmp = null;
        int startindex = header.indexOf("SEGA SEGASATURN");
        if( startindex == -1) return null;

        if( startindex != 0 )
            header = header.substring(startindex);

        tmp = new GameInfo();
        tmp.file_path = file_path;
        tmp.iso_file_path = file_path.toUpperCase();
        tmp.maker_id = header.substring(0x10, 0x20);
        tmp.maker_id = tmp.maker_id.trim();
        tmp.product_number = header.substring(0x20, 0x2A);
        tmp.product_number = tmp.product_number.trim();
        tmp.version = header.substring(0x2A, 0x30);
        tmp.version = tmp.version.trim();
        tmp.release_date = header.substring(0x30, 0x38);
        tmp.release_date = tmp.release_date.trim();
        tmp.area = header.substring(0x40, 0x4A);
        tmp.area = tmp.area.trim();
        tmp.input_device = header.substring(0x50, 0x60);
        tmp.input_device = tmp.input_device.trim();
        tmp.device_infomation = header.substring(0x38, 0x40);
        tmp.device_infomation = tmp.device_infomation.trim();
        tmp.game_title = header.substring(0x60, 0xD0);
        tmp.game_title = tmp.game_title.trim();

        return tmp;

    }

    static public  GameInfo genGameInfoFromCHD(String file_path) {
        String header = YabauseRunnable.getGameinfoFromChd(file_path);
        return getGimeInfoFromBuf(file_path,header);
    }

    static public GameInfo genGameInfoFromIso(String file_path) {
        try {

            byte[] buff = new byte[0xFF];
            DataInputStream dataInStream = new DataInputStream(
                    new BufferedInputStream(new FileInputStream(file_path)));
            dataInStream.read(buff, 0x0, 0xFF);
            dataInStream.close();

            String header = new String(buff);
            return getGimeInfoFromBuf(file_path,header);

        } catch (FileNotFoundException e) {
            System.out.println(e);
            return null;
        } catch (IOException e) {
            System.out.println(e);
            return null;
        }
    }



    public int updateState(){

        GameStatus status = null;
        if( !product_number.equals("")) {
            try {
                status = new Select()
                        .from(GameStatus.class)
                        .where("product_number = ?", product_number)
                        .executeSingle();
            } catch (Exception e) {
                status = null;
            }
        }
        if( status == null ){
            image_url = "";
            rating = 0;
            try {
                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                update_at = sdf.parse("2001-01-01 00:00:00");
            }catch (Exception e) {
                e.printStackTrace();
            }
        }else{
            image_url = status.image_url;
            rating = status.rating;
            update_at = status.update_at;
        }

/*
        if( product_number.equals("")) return -1;

        HttpURLConnection con = null;
        String urlstr = "http://www.uoyabause.org/api/games/" + this.product_number +"/getstatus";
        Context ctx = YabauseApplication.getAppContext();
        String user = ctx.getString(R.string.basic_user);
        String password = ctx.getString(R.string.basic_password);
        try {
            URL url = new URL(urlstr);
            con = (HttpURLConnection) url.openConnection();
            Authenticator authenticator = new BasicAuthenticator(user, password);
            Authenticator.setDefault(authenticator);
            con.setRequestMethod("GET");
            con.setInstanceFollowRedirects(false);
            con.connect();

            if(con.getResponseCode() != 200) {
                return -1;
            }

            BufferedInputStream inputStream = new BufferedInputStream(con.getInputStream());
            ByteArrayOutputStream responseArray = new ByteArrayOutputStream();
            byte[] buff = new byte[1024];

            int length;
            while((length = inputStream.read(buff)) != -1) {
                if(length > 0) {
                    responseArray.write(buff, 0, length);
                }
            }

            // JSONをパース
            StringBuilder viewStrBuilder = new StringBuilder();
            JSONObject jsonObj = new JSONObject(new String(responseArray.toByteArray()));

            image_url = jsonObj.getString("image_url");

            String dateStr = jsonObj.getString("updated_at");
            SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
            update_at = sdf.parse(dateStr);

            rating = jsonObj.getInt("rating");

        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }finally{
        }
*/
        return 0;
    }
}