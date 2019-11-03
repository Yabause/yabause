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

import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.auth.AuthScope;
import org.apache.http.auth.Credentials;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONObject;
import org.uoyabause.uranus.R;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.net.URI;

 class AsyncDownload extends AsyncTask<String, Integer, Bitmap> {

    private HttpClient hClient;
    private HttpGet hGetMethod;
    private Yabause mainActivity;

    // コンストラクター
    public AsyncDownload(Yabause activity){
        mainActivity = activity;
        hClient = new DefaultHttpClient();
        hGetMethod = new HttpGet();
    }

    // バックグラウンドで処理する（重い処理）
    @Override
    protected Bitmap doInBackground(String... params) {
        String uri = params[0];
        return downloadImage(uri);
    }

    // バックグラウンド処理が終了した後にメインスレッドに渡す処理
    @Override
    protected void onPostExecute(Bitmap result) {

    }

    // 画像をダウンロード
    private Bitmap downloadImage(String uri) {
        Log.d("downloadImage", "uri=" + uri);
        try {
            hGetMethod.setURI(new URI(uri));
            HttpResponse resp = hClient.execute(hGetMethod);
            if (resp.getStatusLine().getStatusCode() < 400) {
                Log.d("downloadImage","try");

                InputStream is = resp.getEntity().getContent();
                Bitmap bit = BitmapFactory.decodeStream(is);
                return bit;
            }
        } catch (Exception e) {
            Log.d("downloadImage","error");
            e.printStackTrace();
        }
        return null;
    }
}

public class AsyncReport extends AsyncTask<String, Integer, Integer> {

    private HttpClient hClient;
    private HttpGet hGetMethod;
    private Yabause mainActivity;

    // コンストラクター
    public AsyncReport(Yabause activity){
        mainActivity = activity;
        hClient = new DefaultHttpClient();
        hGetMethod = new HttpGet();
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
    }

     @Override
    protected Integer doInBackground(String... params) {
        String uri = params[0];
        String product_id = params[1];
        return sendReport(uri, product_id);
    }

    @Override
    protected void onPostExecute(Integer result) {
        mainActivity.dismissDialog();
    }

    private Integer sendReport(String uri, String product_id) {

        Log.d("sendReport", "================ sendReport start ================");

        mainActivity._report_status = Yabause.REPORT_STATE_FAIL_CONNECTION;

        DefaultHttpClient client = new DefaultHttpClient();
        try {
            Credentials credentials = new UsernamePasswordCredentials(mainActivity.getString(R.string.basic_user), mainActivity.getString(R.string.basic_password));
            AuthScope scope = new AuthScope(null, -1);
            client.getCredentialsProvider().setCredentials(scope, credentials);
        }catch (Exception e){
            Log.d("sendReport","error " + e.getMessage());
            e.printStackTrace();
            client.getConnectionManager().shutdown();
            return 0;
        }

        String device_id = Settings.Secure.getString(mainActivity.getContentResolver(), Settings.Secure.ANDROID_ID);
        try {
            long id = -1;
            Log.d("sendReport", "uri=" + uri+ "games/" + product_id);
            HttpGet httpGet = new HttpGet(new URI(uri + "games/" + product_id));
            HttpResponse resp = client.execute(httpGet);
            int status = resp.getStatusLine().getStatusCode();
            Log.d("sendReport", "get status=" + status);
            if (HttpStatus.SC_OK == status) {
                try {
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    resp.getEntity().writeTo(outputStream);
                    String data;
                    data = outputStream.toString(); // JSONデータ
                    JSONObject rootObject = new JSONObject(data);
                    Log.d("sendReport", "get id respose=" + data);
                    if( rootObject.getBoolean("result") == true ){
                        id = rootObject.getLong("id");
                    }
                } catch (Exception e) {
                    Log.d("sendReport","error");
                    e.printStackTrace();
                }
            } else {
            }
            if(  id == -1 ) {

                HttpPost httpPost = new HttpPost(new URI(uri + "games/"));
                StringEntity se = new StringEntity(mainActivity.current_game_info.toString());
                httpPost.setEntity(se);
                httpPost.setHeader("Accept", "application/json");
                httpPost.setHeader("Content-Type", "application/json");
                resp = client.execute(httpPost);
                status = resp.getStatusLine().getStatusCode();
                Log.d("sendReport", "post stats=" + status);
                if (HttpStatus.SC_CREATED == status || HttpStatus.SC_OK == status) {
                    try {
                        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                        resp.getEntity().writeTo(outputStream);
                        String data;
                        data = outputStream.toString(); // JSONデータ
                        Log.d("sendReport", "post respose=" + data);
                        JSONObject rootObject = new JSONObject(data);
                        if (rootObject.getBoolean("result") == true) {
                            id = rootObject.getLong("id");
                        }
                    } catch (Exception e) {
                        Log.d("sendReport","error");
                        e.printStackTrace();
                    }
                } else {
                }

            }
            Log.d("sendReport", "ID=" + id);
            if( id == -1 ){
                client.getConnectionManager().shutdown();
                return -1;
            }

            SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(mainActivity);
            String cputype = sharedPref.getString("pref_cpu", "2");
            String gputype = sharedPref.getString("pref_video", "1");
            JSONObject reportJson = new JSONObject();
            reportJson.put("rating",mainActivity.current_report._rating);
            if( mainActivity.current_report._message != null ) {
                reportJson.put("message", mainActivity.current_report._message);
            }
            reportJson.put("emulator_version",Home.getVersionName(mainActivity));
            reportJson.put("device",android.os.Build.MODEL);
            reportJson.put("user_id",1);
            reportJson.put("device_id",device_id);
            reportJson.put("game_id",id);
            reportJson.put("cpu_type",cputype);
            reportJson.put("video_type",gputype);
            JSONObject sendJson = new JSONObject();
            sendJson.put("report",reportJson);
            if( mainActivity.current_report._screenshot ){
                JSONObject jsonObjimg = new JSONObject();
                jsonObjimg.put("data", mainActivity.current_report._screenshot_base64);
                jsonObjimg.put("filename", mainActivity.current_report._screenshot_save_path);
                jsonObjimg.put("content_type", "image/png");
                JSONObject jsonObjgame = sendJson.getJSONObject("report");
                jsonObjgame.put("screenshot", jsonObjimg);
                File file = new File( mainActivity.current_report._screenshot_save_path );
                file.delete();
            }
            Log.d("sendReport", reportJson.toString());
            HttpPost httpPost = new HttpPost(new URI(uri + "reports/"));
            StringEntity se = new StringEntity(sendJson.toString());
            httpPost.setEntity(se);
            httpPost.setHeader("Accept", "application/json");
            httpPost.setHeader("Content-Type", "application/json");
            resp = client.execute(httpPost);
            status = resp.getStatusLine().getStatusCode();
            if (HttpStatus.SC_CREATED == status || HttpStatus.SC_OK == status) {
                try {
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    resp.getEntity().writeTo(outputStream);
                    String data;
                    data = outputStream.toString(); // JSONデータ
                    JSONObject rootObject = new JSONObject(data);
                    Log.d("sendReport", "get respose=" + data);
                    if( rootObject.getBoolean("result") == true ){
                        mainActivity._report_status = Yabause.REPORT_STATE_SUCCESS;
                    }else{
                        int return_code = rootObject.getInt("code");
                        mainActivity._report_status = return_code;
                    }
                } catch (Exception e) {
                    Log.d("sendReport","error");
                    e.printStackTrace();
                }
                client.getConnectionManager().shutdown();
                return 0;
            }
            client.getConnectionManager().shutdown();
            return -1;

        } catch (Exception e) {
            Log.d("sendReport","error:" + e.getMessage() );
            e.printStackTrace();
        }finally {
            client.getConnectionManager().shutdown();
        }
        return -1;
    }
}
