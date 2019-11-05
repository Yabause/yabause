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
import android.os.AsyncTask;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONObject;
import org.uoyabause.uranus.R;
import java.io.File;
import java.io.IOException;
import java.net.Proxy;
import java.net.URI;
import java.net.URL;
import java.util.List;
import java.util.concurrent.TimeUnit;

import okhttp3.Authenticator;
import okhttp3.Challenge;
import okhttp3.Credentials;
import okhttp3.Headers;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.Route;
import okio.Buffer;

/**
 * Created by shinya on 2017/10/07.
 */

public class AsyncReportv2  extends AsyncTask<String, Integer, Integer> {

    private static String LOG_TAG="AsyncReportv2";
    private static final String IMGUR_CLIENT_ID = "...";
    private static final MediaType MEDIA_TYPE_PNG = MediaType.parse("image/png");
    private static final MediaType MEDIA_TYPE_ZIP = MediaType.parse("application/x-zip-compressed");
    private OkHttpClient client = null;
    private Yabause mainActivity;

    private int responseCount(Response response) {
        int result = 1;
        while ((response = response.priorResponse()) != null) {
            result++;
        }
        return result;
    }

    // コンストラクター
    public AsyncReportv2(Yabause activity){
        mainActivity = activity;
        client = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .authenticator(new Authenticator() {
                    @Override
                    public Request authenticate(Route route, Response response) {
                        if (responseCount(response) >= 3) {
                                return null; // If we've failed 3 times, give up. - in real life, never give up!!
                        }
                        String credential = Credentials.basic(mainActivity.getString(R.string.basic_user), mainActivity.getString(R.string.basic_password));
                        return response.request().newBuilder().header("Authorization", credential).build();
                    }
                })
                .build();
    }

    //----------------------------------------------------------------------------------------------
    long getgameid( String baseurl, String product_id ){

        long id = -1;
        String url = baseurl +"/games/" + product_id;
        try {
            Request request = new Request.Builder().url(url).build();
            Response response = client.newCall(request).execute();
            if( response.isSuccessful() ) {
                JSONObject rootObject = new JSONObject(response.body().string());
                if( rootObject.getBoolean("result") == true ){
                    id = rootObject.getLong("id");
                }
            // new data
            }else{

            }

            // if failed create new data
            if( id == -1 ){
                if( mainActivity.current_game_info == null ){
                    return -1;
                }
                url = baseurl +"/games/";
                MediaType MIMEType = MediaType.parse("application/json; charset=utf-8");
                RequestBody requestBody = RequestBody.create(MIMEType, mainActivity.current_game_info.toString());
                request = new Request.Builder().url(url).post(requestBody).build();
                response = client.newCall(request).execute();
                if( response.isSuccessful() ) {
                    JSONObject rootObject = new JSONObject(response.body().string());
                    if( rootObject.getBoolean("result") == true ){
                        id = rootObject.getLong("id");
                    }
                }
            }
        }catch( Exception e){
            Log.e(LOG_TAG, "message", e);
            return -1;
        }



        return id;
    }

    @Override
    protected Integer doInBackground(String... params) {

        String uri = params[0];
        String product_id = params[1];
        String device_id = Settings.Secure.getString(mainActivity.getContentResolver(), Settings.Secure.ANDROID_ID);

        mainActivity._report_status = Yabause.REPORT_STATE_FAIL_CONNECTION;

        long id = getgameid(uri,product_id);
        if( id == -1 ){
            mainActivity._report_status = 400;
            return -1;
        }

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(mainActivity);
        String cputype = sharedPref.getString("pref_cpu", "2");
        String gputype = sharedPref.getString("pref_video", "1");
        JSONObject reportJson = new JSONObject();
        JSONObject sendJson = new JSONObject();
        try {
            reportJson.put("rating", mainActivity.current_report._rating);
            if (mainActivity.current_report._message != null) {
                reportJson.put("message", mainActivity.current_report._message);
            }
            reportJson.put("emulator_version", Home.getVersionName(mainActivity));
            reportJson.put("device", android.os.Build.MODEL);
            reportJson.put("user_id", 1);
            reportJson.put("device_id", device_id);
            reportJson.put("game_id", id);
            reportJson.put("cpu_type", cputype);
            reportJson.put("video_type", gputype);
            sendJson.put("report", reportJson);
        }catch( Exception e ){
            Log.e(LOG_TAG, "message", e);
            return -1;
        }

        MediaType MIMEType = MediaType.parse("application/json; charset=utf-8");

        final String BOUNDARY = String.valueOf(System.currentTimeMillis());

        // Use the imgur image upload API as documented at https://api.imgur.com/endpoints/image
        RequestBody requestBody = new MultipartBody.Builder(BOUNDARY).setType(MultipartBody.FORM)
                .addPart(
                        Headers.of("Content-Disposition", "form-data; name=\"report\""),
                        RequestBody.create(MIMEType, reportJson.toString() ))
                .addFormDataPart("screenshot",mainActivity.getCurrentScreenshotfilename(),
                        RequestBody.create(MEDIA_TYPE_PNG, new File( mainActivity.getCurrentScreenshotfilename() )))
                .addFormDataPart("zip",mainActivity.getCurrentReportfilename(),
                        RequestBody.create(MEDIA_TYPE_ZIP, new File( mainActivity.getCurrentReportfilename() )))
                .build();

        Buffer buffer = new Buffer();
        String CONTENT_LENGTH;
        try {
            requestBody.writeTo(buffer);
            Log.d(LOG_TAG,buffer.toString());
            CONTENT_LENGTH = String.valueOf(buffer.size());
        } catch (IOException e) {
            e.printStackTrace();
            CONTENT_LENGTH = "-1";
        } finally {
            buffer.close();
        }

        try {
            Request request = new Request.Builder()
                    .addHeader("Content-Length", CONTENT_LENGTH)
                    .url(uri + "v2/reports/")
                    .post(requestBody)
                    .build();

            Response response = client.newCall(request).execute();
            if (!response.isSuccessful()) {
                throw new IOException("Unexpected code " + response);
            }else{
                JSONObject rootObject = new JSONObject(response.body().string());
                if( rootObject.getBoolean("result") == true ){
                    mainActivity._report_status = Yabause.REPORT_STATE_SUCCESS;
                }else{
                    int return_code = rootObject.getInt("code");
                    mainActivity._report_status = return_code;
                }
            }

            File tmp = new File( mainActivity.getCurrentScreenshotfilename() );
            tmp.delete();
            tmp = new File( mainActivity.getCurrentReportfilename() );
            tmp.delete();

        }catch(Exception e){
            Log.e(LOG_TAG, "message", e);
            return -1;
        }
        //System.out.println(response.body().string());
        return null;
    }

    @Override
    protected void onPostExecute(Integer result) {
        mainActivity.dismissDialog();
    }
}
