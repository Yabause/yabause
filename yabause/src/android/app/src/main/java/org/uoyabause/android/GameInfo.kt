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

import android.util.Log
import com.activeandroid.Model
import com.activeandroid.annotation.Column
import com.activeandroid.annotation.Table
import com.activeandroid.query.Delete
import com.activeandroid.query.Select
import okhttp3.Credentials
import okhttp3.MediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import org.devmiyax.yabasanshiro.BuildConfig
import org.uoyabause.android.YabauseStorage.Companion.storage
import org.uoyabause.android.YabauseApplication.Companion.appContext
import org.devmiyax.yabasanshiro.R
import org.json.JSONObject
import org.json.JSONException
import java.io.BufferedInputStream
import java.io.BufferedReader
import java.io.ByteArrayOutputStream
import java.io.DataInputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileReader
import java.io.IOException
import java.lang.Exception
import java.lang.StringBuilder
import java.net.Authenticator
import java.net.HttpURLConnection
import java.net.MalformedURLException
import java.net.PasswordAuthentication
import java.net.URL
import java.nio.charset.Charset
import java.text.SimpleDateFormat
import java.util.ArrayList
import java.util.Date
import java.util.concurrent.TimeUnit
import java.util.regex.Pattern

/**
 * Created by shinya on 2015/12/30.
 */
@Table(name = "GameInfo")
class GameInfo : Model() {
    companion object {
        fun getFromFileName(file_path: String?): GameInfo {
            Log.d("GameInfo :", file_path!!)
            return Select()
                .from(GameInfo::class.java)
                .where("file_path = ?", file_path)
                .executeSingle()
        }

        fun getFromInDirectFileName(file_path: String): GameInfo {
            var file_path = file_path
            Log.d("GameInfo direct:", file_path)
            file_path = file_path.toUpperCase()
            return Select()
                .from(GameInfo::class.java)
                .where("iso_file_path = ?", file_path)
                .executeSingle()
        }

        fun deleteAll() {
            Delete().from(GameInfo::class.java).execute<Model>()
        }

        fun genGameInfoFromCUE(file_path: String?): GameInfo? {
            if( file_path == null ) return null
            val file = File(file_path)
            val dirName = file.parent
            var iso_file_name = ""
            var tmp: GameInfo? = null
            try {
                val filereader = FileReader(file)
                val br = BufferedReader(filereader)
                var str = br.readLine()
                while (str != null) {
                    //System.out.println(str);
                    val p = Pattern.compile("FILE \"(.*)\"")
                    val m = p.matcher(str)
                    if (m.find()) {
                        iso_file_name = m.group(1)
                        break
                    }
                    str = br.readLine()
                }
                br.close()
                if (iso_file_name == "") {
                    return null // bad file format;
                }
            } catch (e: FileNotFoundException) {
                println(e)
                return null
            } catch (e: IOException) {
                println(e)
                return null
            }
            iso_file_name = dirName + File.separator + iso_file_name
            tmp = genGameInfoFromIso(iso_file_name)
            if (tmp == null) return null
            tmp.file_path = file_path
            tmp.iso_file_path = iso_file_name.toUpperCase()
            return tmp
        }

        fun genGameInfoFromMDS(file_path: String?): GameInfo? {
            if( file_path == null ) return null
            val iso_file_name = file_path.replace(".mds", ".mdf")
            var tmp: GameInfo? = null
            tmp = genGameInfoFromIso(iso_file_name)
            if (tmp == null) return null
            tmp.file_path = file_path
            tmp.iso_file_path = iso_file_name.toUpperCase()

            // read mdf
            return tmp
        }

        fun genGameInfoFromCCD(file_path: String?): GameInfo? {
            if( file_path == null ) return null
            val iso_file_name = file_path.replace(".ccd", ".img")
            var tmp: GameInfo? = null
            tmp = genGameInfoFromIso(iso_file_name)
            if (tmp == null) return null
            tmp.file_path = file_path
            tmp.iso_file_path = iso_file_name.toUpperCase()
            return tmp
        }

        fun getGimeInfoFromBuf(file_path: String?, header: ByteArray): GameInfo? {
            if( file_path == null ) return null
            var tmp: GameInfo? = null
            var startindex = -1
            val check_str =
                byteArrayOf('S'.toByte(), 'E'.toByte(), 'G'.toByte(), 'A'.toByte(), ' '.toByte())
            for (i in 0 until header.size - check_str.size) {
                if (header[i + 0] == check_str[0] && header[i + 1] == check_str[1] && header[i + 2] == check_str[2] && header[i + 3] == check_str[3] && header[i + 4] == check_str[4]) {
                    startindex = i
                    break
                }
            }
            if (startindex == -1) return null
            try {
                tmp = GameInfo()
                val charaset = Charset.forName("MS932")
                tmp.file_path = file_path
                tmp.iso_file_path = file_path.toUpperCase()
                tmp.maker_id = String(header, startindex + 0x10, 0x10, )
                tmp.maker_id = tmp.maker_id.trim { it <= ' ' }
                tmp.product_number = String(header, startindex + 0x20, 0xA, charaset)
                tmp.product_number = tmp.product_number.trim { it <= ' ' }
                tmp.version = String(header, startindex + 0x2A, 0x10, charaset)
                tmp.version = tmp.version.trim { it <= ' ' }
                tmp.release_date = String(header, startindex + 0x30, 0x8, charaset)
                tmp.release_date = tmp.release_date.trim { it <= ' ' }
                tmp.area = String(header, startindex + 0x40, 0xA, charaset)
                tmp.area = tmp.area.trim { it <= ' ' }
                tmp.input_device = String(header, startindex + 0x50, 0x10, charaset)
                tmp.input_device = tmp.input_device.trim { it <= ' ' }
                tmp.device_infomation = String(header, startindex + 0x38, 0x8, charaset)
                tmp.device_infomation = tmp.device_infomation.trim { it <= ' ' }
                tmp.game_title = String(header, startindex + 0x60, 0x70, charaset)
                tmp.game_title = tmp.game_title.trim { it <= ' ' }
            } catch (e: Exception) {
                Log.e("GameInfo", e.localizedMessage)
                return null
            }
            return tmp
        }

        fun genGameInfoFromCHD(file_path: String?): GameInfo? {
            if( file_path == null ) return null
            Log.d("yabause", file_path)
            val header = YabauseRunnable.getGameinfoFromChd(file_path) ?: return null
            return getGimeInfoFromBuf(file_path, header)
        }

        @JvmStatic
        fun genGameInfoFromIso(file_path: String?): GameInfo? {
            if( file_path == null ) return null
            return try {
                val buff = ByteArray(0xFF)
                val dataInStream = DataInputStream(
                    BufferedInputStream(FileInputStream(file_path)))
                dataInStream.read(buff, 0x0, 0xFF)
                dataInStream.close()
                getGimeInfoFromBuf(file_path, buff)
            } catch (e: FileNotFoundException) {
                println(e)
                null
            } catch (e: IOException) {
                println(e)
                null
            }
        }

        init {
            System.loadLibrary("yabause_native")
        }
    }

    @JvmField
    @Column(name = "file_path", index = true)
    var file_path = ""

    @Column(name = "iso_file_path", index = true)
    var iso_file_path = ""

    @JvmField
    @Column(name = "game_title")
    var game_title = ""

    @JvmField
    @Column(name = "maker_id")
    var maker_id = ""

    @Column(name = "product_number", index = true)
    var product_number = ""

    @Column(name = "version")
    var version = ""

    @Column(name = "release_date")
    var release_date = ""

    @JvmField
    @Column(name = "device_infomation")
    var device_infomation = ""

    @Column(name = "area")
    var area = ""

    @Column(name = "input_device")
    var input_device = ""

    @Column(name = "last_playdate")
    var last_playdate: Date? = null

    @Column(name = "update_at")
    var update_at: Date? = Date()

    @JvmField
    @Column(name = "image_url")
    var image_url: String? = ""

    @JvmField
    @Column(name = "rating")
    var rating = 0

    @JvmField
    @Column(name = "lastplay_date")
    var lastplay_date: Date? = null
    fun removeInstance() {
        val fname = file_path.toUpperCase()
        if (fname.endsWith("CHD")) {
            val file = File(file_path)
            if (file.exists()) {
                file.delete()
            }
            this.delete()
        } else if (fname.endsWith("CCD") || fname.endsWith("MDS")) {
            val file = File(file_path)
            val dir = file.parentFile
            var searchName = file.name
            searchName = searchName.replace(".(?i)ccd".toRegex(), "")
            searchName = searchName.replace(".(?i)mds".toRegex(), "")
            val searchNamef = searchName
            val matchingFiles = dir.listFiles { dir, name -> name.startsWith(searchNamef) }
            for (removefile in matchingFiles) {
                if (removefile.exists()) {
                    removefile.delete()
                }
            }
            this.delete()
        } else if (fname.endsWith("CUE")) {
            val delete_files: MutableList<String> = ArrayList()
            try {
                val filereader = FileReader(file_path)
                val br = BufferedReader(filereader)
                var str = br.readLine()
                while (str != null) {
                    //System.out.println(str);
                    val p = Pattern.compile("FILE \"(.*)\"")
                    val m = p.matcher(str)
                    if (m.find()) {
                        delete_files.add(m.group(1))
                    }
                    str = br.readLine()
                }
                br.close()
                val file = File(file_path)
                for (removefile in delete_files) {
                    val delname = file.parentFile.absolutePath + "/" + removefile
                    val f = File(delname)
                    if (f.exists()) {
                        f.delete()
                    }
                }
                file.delete()
                this.delete()
            } catch (e: FileNotFoundException) {
            } catch (e: IOException) {
            }
        }
    }

    internal inner class BasicAuthenticator(
        private val user: String,
        private val password: String
    ) : Authenticator() {
        override fun getPasswordAuthentication(): PasswordAuthentication {
            return PasswordAuthentication(user, password.toCharArray())
        }
    }

    fun updateState(): Int {
        var status: GameStatus? = null
        if (product_number != "") {
            status = try {
                Select()
                    .from(GameStatus::class.java)
                    .where("product_number = ?", product_number)
                    .executeSingle()
            } catch (e: Exception) {
                null
            }
        }
        if (status == null) {
            image_url = ""
            rating = 0
            try {
                val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss")
                update_at = sdf.parse("2001-01-01 00:00:00")
            } catch (e: Exception) {
                e.printStackTrace()
            }
        } else {
            image_url = "" //status.image_url;
            rating = status.rating
            update_at = status.update_at
        }
        val save_path = storage.screenshotPath
        val screen_shot_save_path = "$save_path$product_number.png"
        var fp: File? = File(screen_shot_save_path)
        if (fp != null && fp.exists()) {
            image_url = screen_shot_save_path
            fp = null
        } else {
            //image_url = status.image_url;
        }
        if (product_number == "") return -1
        var con: HttpURLConnection? = null
        try {
            var encoded_product_id = product_number
            encoded_product_id = encoded_product_id.replace(".", "%2E")
            encoded_product_id = encoded_product_id.replace("-", "%2D")
            encoded_product_id = encoded_product_id.replace(" ", "%20")
            var urlstr = "https://www.uoyabause.org/api/games/$encoded_product_id/getstatus"
            val ctx = appContext
            val user = ctx.getString(R.string.basic_user)
            val password = ctx.getString(R.string.basic_password)
            val url = URL(urlstr)
            con = url.openConnection() as HttpURLConnection
            val authenticator: Authenticator = BasicAuthenticator(user, password)
            Authenticator.setDefault(authenticator)
            con.requestMethod = "GET"
            con!!.instanceFollowRedirects = false
            con.connect()
            val responseCode = con.responseCode
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
            val jsonObj = JSONObject(String(responseArray.toByteArray()))
            try {
                if (jsonObj.getBoolean("result") == false) {
                    Log.i("GameInfo",
                        product_number + "( " + game_title + " ) is not found " + responseCode)

                    // automatic update
                    if (BuildConfig.DEBUG /*&& responseCode == 500*/) {
                        //String.format( "{game:{maker_id:\"%s\",product_number:\"%s\",version:\"%s\","release_date:\"%s\",\"device_infomation\":\"%s\","
                        //        "area:\"%s\",game_title:\"%s\",input_device:\"%s\"}}",
                        //        cdip->company,cdip->itemnum,cdip->version,cdip->date,cdip->cdinfo,cdip->region, cdip->gamename, cdip->peripheral);
                        val job = JSONObject()
                        job.put("game", JSONObject()
                            .put("maker_id", maker_id)
                            .put("product_number", product_number)
                            .put("version", version)
                            .put("release_date", release_date)
                            .put("device_infomation", device_infomation)
                            .put("area", area)
                            .put("game_title", game_title)
                            .put("input_device", input_device)
                        )
                        urlstr = "https://www.uoyabause.org/api/games/"
                        val MIMEType = MediaType.parse("application/json; charset=utf-8")
                        val requestBody = RequestBody.create(MIMEType, job.toString())
                        val request = Request.Builder().url(urlstr).post(requestBody).build()
                        var client: OkHttpClient? = null
                        client = OkHttpClient.Builder()
                            .connectTimeout(10, TimeUnit.SECONDS)
                            .writeTimeout(10, TimeUnit.SECONDS)
                            .readTimeout(30, TimeUnit.SECONDS)
                            .authenticator { route, response ->
                                val ctx = appContext
                                //if (responseCount(response) >= 3) {
                                //    return null; // If we've failed 3 times, give up. - in real life, never give up!!
                                //}
                                val credential =
                                    Credentials.basic(ctx.getString(R.string.basic_user),
                                        ctx.getString(R.string.basic_password))
                                response.request().newBuilder().header("Authorization", credential)
                                    .build()
                            }
                            .build()
                        val response = client.newCall(request).execute()
                        if (response.isSuccessful) {
                            val rootObject = JSONObject(response.body()!!.string())
                            if (rootObject.getBoolean("result") != true) {
                                Log.i("GameInfo",
                                    product_number + "( " + game_title + " ) can not be added")
                            }
                        } else {
                            Log.i("GameInfo",
                                product_number + "( " + game_title + " ) can not be added by " + response.message())
                        }
                    }
                    return -1
                }
            } catch (e: JSONException) {
            }
            if (game_title == "FINALIST") {
                Log.d("debugg", "FINALIST")
            }

            // JSONをパース
            image_url = try {
                jsonObj.getString("image_url")
            } catch (e: JSONException) {
                null
            }
            rating = try {
                jsonObj.getInt("rating")
            } catch (e: JSONException) {
                1
            }
            update_at = try {
                val dateStr = jsonObj.getString("updated_at")
                val sdf = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss")
                sdf.parse(dateStr)
            } catch (e: Exception) {
                val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss")
                sdf.parse("2001-01-01 00:00:00")
            }
        } catch (e: MalformedURLException) {
            e.printStackTrace()
        } catch (e: IOException) {
            e.printStackTrace()
            Log.e("GameInfo", product_number + "( " + game_title + " ) " + e.localizedMessage)
        } catch (e: JSONException) {
            e.printStackTrace()
            Log.e("GameInfo", product_number + "( " + game_title + " ) " + e.localizedMessage)
        } catch (e: Exception) {
            e.printStackTrace()
            Log.e("GameInfo", product_number + "( " + game_title + " ) " + e.localizedMessage)
        } finally {
        }
        return 0
    }
}