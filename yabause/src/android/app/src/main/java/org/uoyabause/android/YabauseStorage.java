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


package org.uoyabause.android;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;

import android.app.Activity;
import android.content.Context;

import java.io.FilenameFilter;
import java.io.IOException;
import java.net.Authenticator;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.PasswordAuthentication;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.activeandroid.query.Delete;
import com.activeandroid.query.Select;

import org.apache.commons.io.FilenameUtils;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.apache.commons.io.IOUtils;
import org.apache.commons.io.FileUtils;
import org.apache.commons.io.filefilter.*;
import org.apache.commons.io.IOCase;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;

import io.reactivex.ObservableEmitter;

class BiosFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".bin")) return true;
        if (filename.endsWith(".rom")) return true;
        return false;
    }
}

class GameFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
    	if (filename.endsWith(".img")) return true;
    	if (filename.endsWith(".IMG")) return true;
        if (filename.endsWith(".bin")) return true;
        if (filename.endsWith(".cue")) return true;
        if (filename.endsWith(".CCD")) return true;
        if (filename.endsWith(".ccd")) return true;
        if (filename.endsWith(".iso")) return true;
        if (filename.endsWith(".mds")) return true;
        if (filename.endsWith(".BIN")) return true;
        if (filename.endsWith(".CUE")) return true;
        if (filename.endsWith(".ISO")) return true;
        if (filename.endsWith(".MDS")) return true;
        if (filename.endsWith(".CHD")) return true;
        if (filename.endsWith(".chd")) return true;
        return false;
    }
}

class MemoryFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".ram")) return true;
        return false;
    }
}

public class YabauseStorage {
    static private YabauseStorage instance = null;

    private File bios;
    private File games;
    private File memory;
    private File cartridge;
    private File state;
    private File screenshots;
    private File record;
    private File external = null;

    private ObservableEmitter<String> progress_emitter = null;

    void setProcessEmmiter( ObservableEmitter<String> emitter ){
        progress_emitter = emitter;
    }

    private YabauseStorage() {
        File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");

        if (! yabroot.exists()) yabroot.mkdir();

        bios = new File(yabroot, "bios");
        if (! bios.exists()) bios.mkdir();

        games = new File(yabroot, "games");
        if (! games.exists()) games.mkdir();

        memory = new File(yabroot, "memory");
        if (! memory.exists()) memory.mkdir();

        cartridge = new File(yabroot, "cartridge");
        if (! cartridge.exists()) cartridge.mkdir();
        
        state = new File(yabroot, "state");
        if (! state.exists()) state.mkdir();
        
        screenshots = new File(yabroot, "screenshots");
        if (! screenshots.exists()) screenshots.mkdir();

        record = new File(yabroot, "record");
        if (! record.exists()) record.mkdir();

    }

    static public YabauseStorage getStorage() {
        if (instance == null) {
            instance = new YabauseStorage();
        }

        return instance;
    }

    public String[] getBiosFiles() {
        String[] biosfiles = bios.list(new BiosFilter());
        return biosfiles;
    }

    public String getBiosPath(String biosfile) {
        return bios + File.separator + biosfile;
    }

    public String[] getGameFiles( String other_dir_string ) {
        String[] gamefiles = games.list(new GameFilter());
        
        Arrays.sort(gamefiles, new Comparator<String>() {
        	  @Override
        	    public int compare(String obj0, String obj1) {
        	        return obj0.compareTo(obj1);
        	    }
        	});
        
        String[] selfiles  = new String[]{other_dir_string}; 
        String[] allLists = new String[selfiles.length + gamefiles.length];
        System.arraycopy(selfiles, 0, allLists, 0, selfiles.length);
        System.arraycopy(gamefiles, 0, allLists, selfiles.length, gamefiles.length);



        return allLists;
    }

    public String getGamePath(String gamefile) {
        return games + File.separator + gamefile;
    }

    public String getGamePath() {
        return games + File.separator;
    }

    public void setExternalStoragePath( String expath ){
        File yabroot = new File(expath, "yabause");
        if (! yabroot.exists()) {
            if( yabroot.mkdirs() == false ){
                int a=0;
            }
        }
        external = new File(yabroot, "games");
        if (! external.exists()) {
            external.mkdirs();
        }
    }

    public boolean hasExternalSD() {
        if( external != null ){
            return true;
        }
        return false;
    }

    public String getExternalGamePath() {
        if( external == null ){
            return null;
        }
        return external + File.separator;
    }
    
    public String[] getMemoryFiles() {
        String[] memoryfiles = memory.list(new MemoryFilter());
        return memoryfiles;
    }

    public String getMemoryPath(String memoryfile) {
        return memory + File.separator + memoryfile;
    }

    public String getCartridgePath(String cartridgefile) {
        return cartridge + File.separator + cartridgefile;
    }
    
    public String getStateSavePath() {
        return state + File.separator;
    }

    public String getRecordPath() {
        return record + File.separator;
    }

    public String getScreenshotPath() {
        return screenshots + File.separator;
    }

    class BasicAuthenticator extends Authenticator {
        private String password;
        private String user;

        public BasicAuthenticator(String user, String password){
            this.password = password;
            this.user = user;
        }

        protected PasswordAuthentication getPasswordAuthentication(){
            return new PasswordAuthentication(user, password.toCharArray());
        }
    }

    int updateAllGameStatus(){
        HttpURLConnection con = null;
        String urlstr;

        Date lastupdate = GameStatus.getLastUpdate();
        if( lastupdate == null){
            urlstr = "https://www.uoyabause.org/api/games/get_status_from/?date=20010101";
        }else{
            SimpleDateFormat f = new SimpleDateFormat("yyyy/MM/dd'T'HH:mm:ss");
            String date_string = f.format(lastupdate);
            urlstr = "https://www.uoyabause.org/api/games/get_status_from/?date=" + date_string;
        }

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

            StringBuilder viewStrBuilder = new StringBuilder();
            JSONArray ar = new JSONArray(new String(responseArray.toByteArray()));
            for( int i=0; i< ar.length(); i++ ) {
                JSONObject jsonObj = ar.getJSONObject(i);
                GameStatus status = null;
                try {
                    status = new Select()
                            .from(GameStatus.class)
                            .where("product_number = ?", jsonObj.getString("product_number"))
                            .executeSingle();
                }catch( Exception e) {
                    status = new GameStatus();
                }
                if( status== null ){
                    status = new GameStatus();
                }

                status.product_number = jsonObj.getString("product_number");
                status.image_url = jsonObj.getString("image_url");
                String dateStr = jsonObj.getString("updated_at");
                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
                status.update_at = sdf.parse(dateStr);
                status.rating = jsonObj.getInt("rating");
                status.save();
            }

        } catch (MalformedURLException e) {
            e.printStackTrace();
            return -1;
        } catch (IOException e) {
            e.printStackTrace();
            return -1;
        } catch (JSONException e) {
            e.printStackTrace();
            return -1;
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }finally{
        }

        return 0;
    }

    void generateGameListFromDirectory( String dir ){
        String[] extensions = new String[] {"img", "bin", "ccd","CCD", "cue","mds","iso","IMG","BIN", "CUE","MDS","ISO","CHD","chd" };
        IOFileFilter filter = new SuffixFileFilter(extensions, IOCase.INSENSITIVE);
        boolean recursive = true;

        File gamedir = new File(dir);
        if( !gamedir.exists() ) return;
        if( !gamedir.isDirectory() ) return;

        Iterator<File> iter = FileUtils.iterateFiles(gamedir, extensions, recursive);

        int i=0;
        while(iter.hasNext()){
            File  gamefile = iter.next();
            String gamefile_name = gamefile.getAbsolutePath();

            Log.d("generateGameDB",gamefile_name);

            GameInfo gameinfo = null;
            if( gamefile_name.endsWith("CUE") || gamefile_name.endsWith("cue") ){
                GameInfo tmp = GameInfo.getFromFileName( gamefile_name );
                if( tmp == null ) {
                    gameinfo = GameInfo.genGameInfoFromCUE( gamefile_name);
                }

            }else if( gamefile_name.endsWith("MDS") || gamefile_name.endsWith("mds") ){
                GameInfo tmp = GameInfo.getFromFileName( gamefile_name);
                if( tmp == null ) {
                    gameinfo = GameInfo.genGameInfoFromMDS(gamefile_name);
                }
            }else if( gamefile_name.endsWith("CCD") || gamefile_name.endsWith("ccd") ) {
                GameInfo tmp = GameInfo.getFromFileName(gamefile_name);
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromCCD(gamefile_name);
                }
            }else if( gamefile_name.endsWith("CHD") || gamefile_name.endsWith("chd") ) {
                GameInfo tmp = GameInfo.getFromFileName(gamefile_name);
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromCHD(gamefile_name);
                }
            }
            if( gameinfo != null ) {
                gameinfo.updateState();
                gameinfo.save();
                if( progress_emitter != null ){
                    progress_emitter.onNext(gameinfo.game_title);
                }
            }
        }

        iter =  FileUtils.iterateFiles(gamedir, extensions, recursive);
        while(iter.hasNext()) {
            File gamefile = iter.next();
            String gamefile_name = gamefile.getAbsolutePath();

            if( gamefile_name.endsWith("BIN") || gamefile_name.endsWith("bin") ||
                    gamefile_name.endsWith("ISO") || gamefile_name.endsWith("iso") ||
                    gamefile_name.endsWith("IMG") || gamefile_name.endsWith("img") ) {
                GameInfo tmp = GameInfo.getFromInDirectFileName(gamefile_name);
                if (tmp == null) {
                    GameInfo gameinfo = GameInfo.genGameInfoFromIso(gamefile_name);
                    if (gameinfo != null) {
                        gameinfo.updateState();
                        gameinfo.save();
                    }
                }
            }
        }

    }

    public final static int REFRESH_LEVEL_STATUS_ONLY = 0;
    public final static int REFRESH_LEVEL_REBUILD = 3;


    public void generateGameDB( int level ){

        int rtn = updateAllGameStatus();
        if( level == 0 && rtn == -1 ) return;

        if( level >=3 ) {
            GameInfo.deleteAll();
        }

        Context ctx = YabauseApplication.getAppContext();
        ArrayList<String> list;
        list = new ArrayList<String>();
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx);
        String data = sharedPref.getString("pref_game_directory","err");
        if( data.equals("err")){
            list.add(getGamePath());
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putString("pref_game_directory", getGamePath());
            if( hasExternalSD() == true ) {
                editor.putString("pref_game_directory", getGamePath() + ";" + getExternalGamePath() );
                list.add(getExternalGamePath());
            }
            editor.apply();
        }else {
            String[] paths = data.split(";", 0);
            for( int i=0; i<paths.length; i++ ){
                list.add(paths[i]);
            }
            if( hasExternalSD() == true ) {
                list.add(getExternalGamePath());
             }
        }

        Set<String> set = new HashSet<String>();
        set.addAll(list);
        List<String> uniqueList = new ArrayList<String>();
        uniqueList.addAll(set);


        for( int i=0; i< uniqueList.size(); i++ ){
            generateGameListFromDirectory( uniqueList.get(i) );
        }

/*
        // inDirect Format
        for( i=0; i< gamefiles.length; i++ ){
            GameInfo gameinfo = null;
            if( gamefiles[i].endsWith("CUE") || gamefiles[i].endsWith("cue") ){
                if( gamefiles[i].indexOf("3D") >= 0){
                    Log.d("Yabause",gamefiles[i]);
                }
                GameInfo tmp = GameInfo.getFromFileName( getGamePath() + gamefiles[i]);
                if( tmp == null ) {
                    gameinfo = GameInfo.genGameInfoFromCUE( getGamePath() + gamefiles[i]);
                }
            }else if( gamefiles[i].endsWith("MDS") || gamefiles[i].endsWith("mds") ){
                GameInfo tmp = GameInfo.getFromFileName( getGamePath() + gamefiles[i]);
                if( tmp == null ) {
                    gameinfo = GameInfo.genGameInfoFromMDS(getGamePath() + gamefiles[i]);
                }
            }else if( gamefiles[i].endsWith("CCD") || gamefiles[i].endsWith("ccd") ) {
                GameInfo tmp = GameInfo.getFromFileName(getGamePath() + gamefiles[i]);
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromMDS(getGamePath() + gamefiles[i]);
                }
            }
            if( gameinfo != null ) {
                gameinfo.updateState();
                gameinfo.save();
            }

        }

        // Direct Format
        for( i=0; i< gamefiles.length; i++ ){

            if( gamefiles[i].endsWith("BIN") || gamefiles[i].endsWith("bin") ||
                    gamefiles[i].endsWith("ISO") || gamefiles[i].endsWith("iso") ||
                    gamefiles[i].endsWith("IMG") || gamefiles[i].endsWith("img") ) {
                GameInfo tmp = GameInfo.getFromInDirectFileName(getGamePath() + gamefiles[i]);
                if (tmp == null) {
                    GameInfo gameinfo = GameInfo.genGameInfoFromIso(getGamePath() + gamefiles[i]);
                    if (gameinfo != null) {
                        gameinfo.updateState();
                        gameinfo.save();
                    }
                }
            }
        }
*/
    }
}

