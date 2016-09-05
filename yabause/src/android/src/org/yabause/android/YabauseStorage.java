package org.yabause.android;

import java.io.File;
import java.io.FilenameFilter;

import android.os.Environment;
import android.util.Log;

class BiosFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".bin")) return true;
        if (filename.endsWith(".rom")) return true;
        return false;
    }
}

class GameFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".bin")) return true;
        if (filename.endsWith(".cue")) return true;
        if (filename.endsWith(".iso")) return true;
        if (filename.endsWith(".mds")) return true;
        return false;
    }
}

class MemoryFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".ram")) return true;
        return false;
    }
}

class SaveFilter implements FilenameFilter {
    public boolean accept(File dir, String filename) {
        if (filename.endsWith(".yss")) return true;
        return false;
    }
}

class GameSavesFilter implements FilenameFilter {
    private String itemnum;

    GameSavesFilter(String itemnum) {
        super();
        this.itemnum = itemnum;
    }

    public boolean accept(File dir, String filename) {
        if (filename.startsWith(this.itemnum) && filename.endsWith(".yss")) return true;
        return false;
    }
}

public class YabauseStorage {
    static private YabauseStorage instance = null;

    private File bios;
    private File games;
    private File memory;
    private File cartridge;
    private File saves;

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

        saves = new File(yabroot, "saves");
        if (! saves.exists()) saves.mkdir();
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

    public String[] getGameFiles() {
        String[] gamefiles = games.list(new GameFilter());
        if (gamefiles == null) {
            /* We're on Android >= 6 and we don't have permission
            to access external storage */
            gamefiles = new String[0];
        }
        return gamefiles;
    }

    public String getGamePath(String gamefile) {
        return games + File.separator + gamefile;
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

    public String getSavesPath() {
        return saves.getPath();
    }

    public String[] getGameSaves(String itemnum) {
        return saves.list(new GameSavesFilter(itemnum));
    }

    public void deleteGameSaves(String[] gamesaves) {
        for(String save : gamesaves) {
            File f = new File(saves.getPath() + File.separator + save);
            f.delete();
        }
    }
}
