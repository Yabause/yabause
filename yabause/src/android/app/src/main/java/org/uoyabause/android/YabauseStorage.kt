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

import android.os.Environment
import android.os.StatFs
import android.util.Log
import androidx.preference.PreferenceManager
import com.activeandroid.ActiveAndroid
import com.activeandroid.query.Select
import io.reactivex.ObservableEmitter
import net.nend.android.a.g.o
import okhttp3.Call
import okhttp3.Callback
import okhttp3.Credentials
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import org.apache.commons.io.FileUtils
import org.apache.commons.io.IOCase
import org.apache.commons.io.filefilter.IOFileFilter
import org.apache.commons.io.filefilter.SuffixFileFilter
import org.devmiyax.yabasanshiro.R
import org.json.JSONArray
import org.json.JSONException
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FilenameFilter
import java.io.IOException
import java.net.Authenticator
import java.net.HttpURLConnection
import java.net.MalformedURLException
import java.net.PasswordAuthentication
import java.net.URL
import java.text.SimpleDateFormat
import java.util.Arrays
import kotlin.system.measureTimeMillis


internal class BiosFilter : FilenameFilter {
    override fun accept(dir: File, filename: String): Boolean {
        if (filename.endsWith(".bin")) return true
        return if (filename.endsWith(".rom")) true else false
    }
}

internal class GameFilter : FilenameFilter {
    override fun accept(dir: File, filename: String): Boolean {
        if (filename.endsWith(".img")) return true
        if (filename.endsWith(".IMG")) return true
        if (filename.endsWith(".bin")) return true
        if (filename.endsWith(".cue")) return true
        if (filename.endsWith(".CCD")) return true
        if (filename.endsWith(".ccd")) return true
        if (filename.endsWith(".iso")) return true
        if (filename.endsWith(".mds")) return true
        if (filename.endsWith(".BIN")) return true
        if (filename.endsWith(".CUE")) return true
        if (filename.endsWith(".ISO")) return true
        if (filename.endsWith(".MDS")) return true
        if (filename.endsWith(".CHD")) return true
        return if (filename.endsWith(".chd")) true else false
    }
}

internal class MemoryFilter : FilenameFilter {
    override fun accept(dir: File, filename: String): Boolean {
        return if (filename.endsWith(".ram")) true else false
    }
}

class YabauseStorage private constructor() {
    private var webcdUrl: String = ""
    private val bios: File
    private val games: File
    private val memory: File
    private val cartridge: File
    private val state: File
    private val screenshots: File
    private val record: File
    private val root: File
    private var external: File? = null
    private var progress_emitter: ObservableEmitter<String>? = null
    fun setProcessEmmiter(emitter: ObservableEmitter<String>?) {
        progress_emitter = emitter
    }

    val biosFiles: Array<String?>?
        get() = bios.list(BiosFilter())

    fun getBiosPath(biosfile: String): String {
        return bios.toString() + File.separator + biosfile
    }

    fun setWebCdUrl(url: String){
        webcdUrl = url
    }

    fun getGameFiles(other_dir_string: String): Array<String?> {
        val gameFiles = games.list(GameFilter())
        Arrays.sort(gameFiles) { obj0, obj1 -> obj0.compareTo(obj1) }
        val selfiles = arrayOf(other_dir_string)
        val allLists = arrayOfNulls<String>(selfiles.size + gameFiles.size)
        System.arraycopy(selfiles, 0, allLists, 0, selfiles.size)
        System.arraycopy(gameFiles, 0, allLists, selfiles.size, gameFiles.size)
        return allLists
    }

    fun getGamePath(gamefile: String): String {
        return games.toString() + File.separator + gamefile
    }

    val gamePath: String
        get() = games.toString() + File.separator

    fun setExternalStoragePath(expath: String?) {
        external = File(expath)
    }

    fun hasExternalSD(): Boolean {
        return if (external != null) {
            true
        } else false
    }

    val externalGamePath: String?
        get() = if (external == null) {
            null
        } else external.toString() + File.separator

    val memoryFiles: Array<String>
        get() = memory.list(MemoryFilter())

    fun getMemoryPath(memoryfile: String): String {
        return memory.toString() + File.separator + memoryfile
    }

    fun getCartridgePath(cartridgefile: String): String {
        return cartridge.toString() + File.separator + cartridgefile
    }

    val stateSavePath: String
        get() = state.toString() + File.separator

    val recordPath: String
        get() = record.toString() + File.separator

    val screenshotPath: String
        get() = screenshots.toString() + File.separator

    val rootPath: String
        get() = root.toString() + File.separator

    internal inner class BasicAuthenticator(private val user: String, private val password: String) : Authenticator() {
        override fun getPasswordAuthentication(): PasswordAuthentication {
            return PasswordAuthentication(user, password.toCharArray())
        }
    }

    fun updateAllGameStatus(): Int {
        var con: HttpURLConnection? = null
        val urlstr: String
        val lastupdate = GameStatus.getLastUpdate()
        urlstr = if (lastupdate == null) {
            "https://www.uoyabause.org/api/games/get_status_from/?date=20010101"
        } else {
            val f = SimpleDateFormat("yyyy/MM/dd'T'HH:mm:ss")
            val date_string = f.format(lastupdate)
            "https://www.uoyabause.org/api/games/get_status_from/?date=$date_string"
        }
        val ctx = YabauseApplication.appContext
        val user = ctx!!.getString(R.string.basic_user)
        val password = ctx!!.getString(R.string.basic_password)
        try {
            val url = URL(urlstr)
            con = url.openConnection() as HttpURLConnection
            val authenticator: Authenticator = BasicAuthenticator(user, password)
            Authenticator.setDefault(authenticator)
            con!!.requestMethod = "GET"
            con.instanceFollowRedirects = false
            con.connect()
            if (con.responseCode != 200) {
                return -1
            }
            val inputStream = BufferedInputStream(con.inputStream)
            val responseArray = ByteArrayOutputStream()
            val buff = ByteArray(1024)
            var length: Int
            while (inputStream.read(buff).also { length = it } != -1) {
                if (length > 0) {
                    responseArray.write(buff, 0, length)
                }
            }
            val viewStrBuilder = StringBuilder()
            val ar = JSONArray(String(responseArray.toByteArray()))

            ActiveAndroid.beginTransaction()
            for (i in 0 until ar.length()) {
                var status: GameStatus? = null

                val time = measureTimeMillis {
                    val jsonObj = ar.getJSONObject(i)
                    if (lastupdate == null) {
                        status = GameStatus()
                    } else {
                        status = try {
                            Select()
                                .from(GameStatus::class.java)
                                .where("product_number = ?", jsonObj.getString("product_number"))
                                .executeSingle()
                        } catch (e: Exception) {
                            GameStatus()
                        }
                        if (status == null) {
                            status = GameStatus()
                        }
                    }
                    status!!.product_number = jsonObj.getString("product_number")
                    status!!.image_url = jsonObj.getString("image_url")
                    val dateStr = jsonObj.getString("updated_at")
                    val sdf = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss")
                    status!!.update_at = sdf.parse(dateStr)
                    status!!.rating = jsonObj.getInt("rating")
                    status!!.save()
                }
                //Log.i("YabauseStorage", "updateing ${status?.product_number} : ${time}")
                status?.product_number?.let { progress_emitter!!.onNext(it) }
            }
            ActiveAndroid.endTransaction()

        } catch (e: MalformedURLException) {
            e.printStackTrace()
            return -1
        } catch (e: IOException) {
            e.printStackTrace()
            return -1
        } catch (e: JSONException) {
            e.printStackTrace()
            return -1
        } catch (e: Exception) {
            e.printStackTrace()
            return -1
        } finally {
        }
        return 0
    }
/*
    fun generateGameListFromWebServer(dir: String?) {

        val client = OkHttpClient()


        val ctx = YabauseApplication.appContext
        val user: String = ctx!!.getString(R.string.basic_user)
        val password: String = ctx!!.getString(R.string.basic_password)

        progress_emitter!!.onNext("game status")

        val requestStatus = Request.Builder()
            .url("https://www.uoyabause.org//api/games/get_status_from/?date=1970/01/01T14:34:57" )
            .header("Authorization",
                Credentials.basic(
                    user,
                    password))
            .build()

        val stateRsponse = client.newCall(requestStatus).execute()
        if (!stateRsponse.isSuccessful) throw IOException("Unexpected code $stateRsponse")
        val gameStatus = JSONArray(stateRsponse.body()?.string())

        var gamesJ = JSONObject()

        try {


            for (index in 0 until gameStatus.length()) {

                val obj = gameStatus.getJSONObject(index)
                val number = obj?.getString("product_number")
                if (obj != null && number != null) {
                    gamesJ.put(number, obj)
                }

            }
        }catch( e: JSONException ){
            Log.e("YabauseStorage", e.localizedMessage)

        }


        progress_emitter!!.onNext("game list")

        val request = Request.Builder()
            .url(webcdUrl + "/games/list" )
            .build()
        client.newCall(request).enqueue(object : Callback {
            override fun onResponse(call: Call, response: Response) {
                val responseBody = response.body()?.string().orEmpty()
            }

            override fun onFailure(call: Call, e: IOException) {
                Log.e("Error", e.toString())
            }
        })
        val response = client.newCall(request).execute()
        if (!response.isSuccessful) throw IOException("Unexpected code $response")
        //println(response.body()?.string())

        val gameList = JSONArray(response.body()?.string())


        for (i in 0 until gameList.length()) {

            try {
                var g = GameInfo()
                val jobj = gameList.getJSONObject(i)
                g.file_path =
                    "{ \"baseurl\":\"${webcdUrl}\", \"gameid\":\"${jobj.getString("id")}\" }"
                g.iso_file_path = jobj.getString("id")
                g.maker_id = jobj.getString("maker_id")
                g.product_number = jobj.getString("product_number")
                g.version = jobj.getString("version")
                g.release_date = jobj.getString("release_date")
                g.area = jobj.getString("area")
                g.input_device = jobj.getString("input_device")
                g.device_infomation = jobj.getString("device_infomation")
                g.game_title = jobj.getString("game_title")
                //g.updateState()

                try {
                    val obj = gamesJ.getJSONObject(g.product_number)
                    if (obj != null) {
                        g.image_url = obj.optString("image_url", "")
                        g.rating = obj.optInt("rating", -1)
                    }
                }catch(e: JSONException){
                    Log.e("YabauseStorage", e.localizedMessage)
                }
                g.save()
                if (progress_emitter != null) {
                    progress_emitter!!.onNext(g.game_title)
                }
            }catch(e: Exception){
                Log.e("YabauseStorage", e.localizedMessage)
            }
        }
    }
 */

    fun generateGameListFromDirectory(dir: String?) {
        val extensions = arrayOf("img",
            "bin",
            "ccd",
            "CCD",
            "cue",
            "mds",
            "iso",
            "IMG",
            "BIN",
            "CUE",
            "MDS",
            "ISO",
            "CHD",
            "chd")
        val filter: IOFileFilter = SuffixFileFilter(extensions, IOCase.INSENSITIVE)
        val recursive = true
        val gamedir = File(dir)
        if (!gamedir.exists()) return
        if (!gamedir.isDirectory) return
        var iter = FileUtils.iterateFiles(gamedir, extensions, recursive)
        val i = 0
        while (iter.hasNext()) {
            val gamefile = iter.next()
            val gamefile_name = gamefile.absolutePath
            Log.d("generateGameDB", gamefile_name)
            var gameinfo: GameInfo? = null
            if (gamefile_name.endsWith("CUE") || gamefile_name.endsWith("cue")) {
                val tmp = GameInfo.getFromFileName(gamefile_name)
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromCUE(gamefile_name)
                }
            } else if (gamefile_name.endsWith("MDS") || gamefile_name.endsWith("mds")) {
                val tmp = GameInfo.getFromFileName(gamefile_name)
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromMDS(gamefile_name)
                }
            } else if (gamefile_name.endsWith("CCD") || gamefile_name.endsWith("ccd")) {
                val tmp = GameInfo.getFromFileName(gamefile_name)
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromCCD(gamefile_name)
                }
            } else if (gamefile_name.endsWith("CHD") || gamefile_name.endsWith("chd")) {
                val tmp = GameInfo.getFromFileName(gamefile_name)
                if (tmp == null) {
                    gameinfo = GameInfo.genGameInfoFromCHD(gamefile_name)
                }
            }
            if (gameinfo != null) {
                gameinfo.updateState()
                gameinfo.save()
                if (progress_emitter != null) {
                    progress_emitter!!.onNext(gameinfo.game_title)
                }
            }
        }
        iter = FileUtils.iterateFiles(gamedir, extensions, recursive)
        while (iter.hasNext()) {
            val gamefile = iter.next()
            val gamefile_name = gamefile.absolutePath
            if (gamefile_name.endsWith("BIN") || gamefile_name.endsWith("bin") ||
                    gamefile_name.endsWith("ISO") || gamefile_name.endsWith("iso") ||
                    gamefile_name.endsWith("IMG") || gamefile_name.endsWith("img")) {
                val tmp = GameInfo.getFromInDirectFileName(gamefile_name)
                if (tmp == null) {
                    val gameinfo = GameInfo.genGameInfoFromIso(gamefile_name)
                    if (gameinfo != null) {
                        gameinfo.updateState()
                        gameinfo.save()
                    }
                }
            }
        }
    }

    fun generateGameDB(level: Int) {
        val rtn = updateAllGameStatus()
        if (level == 0 && rtn == -1) return
        if (level >= 3) {
            GameInfo.deleteAll()
        }
        val ctx = YabauseApplication.appContext
        var list: ArrayList<String?> = ArrayList()
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx)
        val data = sharedPref.getString("pref_game_directory", "err")
        if (data == "err") {
            list.add(gamePath)
            val editor = sharedPref.edit()
            editor.putString("pref_game_directory", gamePath)
            if (hasExternalSD() == true) {
                editor.putString("pref_game_directory", "$gamePath;$externalGamePath")
                list.add(externalGamePath)
            }
            editor.apply()
        } else {
            var listtmp: ArrayList<String?> = ArrayList()
            val paths = data!!.split(";".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
            for (i in paths.indices) {
                listtmp.add(paths[i])
            }
            listtmp.add(gamePath)
            if (hasExternalSD() == true) {
                listtmp.add(externalGamePath)
            }
            list = ArrayList<String?>(LinkedHashSet(listtmp))
        }
        // val set: MutableList<String> = ArrayList()
        // set.addAll(list)
        // val uniqueList: MutableList<String> = ArrayList()
        // uniqueList.addAll(list)
        val ulist = list.distinct()
        for (i in ulist.indices) {
            generateGameListFromDirectory(ulist[i])
        }

        //generateGameListFromWebServer("http://dddd")

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

    fun getInstallDir() : File {
        val ctx = YabauseApplication.appContext
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(ctx)
        val path = sharedPref.getString("pref_install_location","0")
        if(path=="0" || hasExternalSD() == false ){
            return games
        }else {
            return external!!
        }
    }

    private object HOLDER {
        val INSTANCE = YabauseStorage()
    }

    companion object {

        @JvmStatic
        val storage: YabauseStorage by lazy { HOLDER.INSTANCE }

        const val REFRESH_LEVEL_STATUS_ONLY = 0
        const val REFRESH_LEVEL_REBUILD = 3
    }



    init {
        var yabroot = File(YabauseApplication.appContext!!.getExternalFilesDir(null), "yabause")

        // Above version 10
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
            val oldyabroot = File(Environment.getExternalStorageDirectory(), "yabause")
            // if (!yabroot.exists() && oldyabroot.exists()) {
            //    Files.move(oldyabroot.toPath(), yabroot.toPath(), StandardCopyOption.REPLACE_EXISTING)
            // }
        } else {
            yabroot = File(Environment.getExternalStorageDirectory(), "yabause")
        }

        root = yabroot

        if (!yabroot.exists()) yabroot.mkdir()
        bios = File(yabroot, "bios")
        if (!bios.exists()) bios.mkdir()
        games = File(yabroot, "games")
        if (!games.exists()) games.mkdir()
        memory = File(yabroot, "memory")
        if (!memory.exists()) memory.mkdir()
        cartridge = File(yabroot, "cartridge")
        if (!cartridge.exists()) cartridge.mkdir()
        state = File(yabroot, "state")
        if (!state.exists()) state.mkdir()
        screenshots = File(yabroot, "screenshots")
        if (!screenshots.exists()) screenshots.mkdir()
        record = File(yabroot, "record")
        if (!record.exists()) record.mkdir()

    }

    fun externalMemoryAvailable(): Boolean {
        return Environment.getExternalStorageState() ==
                Environment.MEDIA_MOUNTED
    }

    fun getAvailableInternalMemorySize(): String? {
        val stat = StatFs(gamePath)
        val blockSize = stat.blockSizeLong
        val availableBlocks = stat.availableBlocksLong
        return formatSize(availableBlocks * blockSize)
    }

    fun getTotalInternalMemorySize(): String? {
        val stat = StatFs(gamePath)
        val blockSize = stat.blockSizeLong
        val totalBlocks = stat.blockCountLong
        return formatSize(totalBlocks * blockSize)
    }

    fun getAvailableExternalMemorySize(): String? {
        return if (externalMemoryAvailable()) {
            val stat = StatFs(externalGamePath)
            val blockSize = stat.blockSizeLong
            val availableBlocks = stat.availableBlocksLong
            formatSize(availableBlocks * blockSize)
        } else {
            "ERROR"
        }
    }

    fun getTotalExternalMemorySize(): String? {
        return if (externalMemoryAvailable()) {
            val stat = StatFs(externalGamePath)
            val blockSize = stat.blockSizeLong
            val totalBlocks = stat.blockCountLong
            formatSize(totalBlocks * blockSize)
        } else {
            "ERROR"
        }
    }

    fun formatSize(size: Long): String? {
        var size = size
        var suffix: String? = null
        if (size >= 1024) {
            suffix = "KB"
            size /= 1024
            if (size >= 1024) {
                suffix = "MB"
                size /= 1024
            }
            if (size >= 1024) {
                suffix = "GB"
                size /= 1024
            }
        }
        val resultBuffer = java.lang.StringBuilder(java.lang.Long.toString(size))
        var commaOffset = resultBuffer.length - 3
        while (commaOffset > 0) {
            resultBuffer.insert(commaOffset, ',')
            commaOffset -= 3
        }
        if (suffix != null) resultBuffer.append(suffix)
        return resultBuffer.toString()
    }
}
