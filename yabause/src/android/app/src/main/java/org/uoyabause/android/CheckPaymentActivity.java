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

import android.app.ProgressDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import android.text.util.Linkify;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.json.JSONObject;
import org.uoyabause.uranus.R;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import okhttp3.Authenticator;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.Credentials;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.Route;

import static java.lang.Thread.sleep;

public class CheckPaymentActivity extends AppCompatActivity {

    OkHttpClient client_ = null;
    private final String LOG_TAG = "CheckPaymentActivity";
    private final String sip_ = "www.uoyabause.org";
    private String errormsg_;
    private int result_code_ = RESULT_CANCELED;
    private ProgressDialog mProgressDialog = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_check_payment);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);

        findViewById(R.id.editText_1).requestFocus();

        Button btn = (Button)findViewById(R.id.button2);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                onSubmmit(v);
            }
        });

        UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
            TextView t = (TextView) findViewById(R.id.textView_message);
            Linkify.addLinks(t, Linkify.ALL);
        }
        errormsg_ = "";

    }

    String run(String url) throws IOException {
        Request request = new Request.Builder()
                .url(url)
                .build();
        Response response = client_.newCall(request).execute();
        return response.body().string();
    }

    int send(){
        Log.d(LOG_TAG,"onSubmmit");

        String gpa = "GPA.";

        TextView tv = (TextView)findViewById(R.id.editText_1);
        if( tv == null || tv.getText().length() != 4 ){
            errormsg_ = "input";
            tv.requestFocus();
            return -1;
        }
        gpa += tv.getText() + "-";

        tv = (TextView)findViewById(R.id.editText_2);
        if( tv == null || tv.getText().length() != 4 ){
            errormsg_ = "input";
            tv.requestFocus();
            return -1;
        }
        gpa += tv.getText() + "-";

        tv = (TextView)findViewById(R.id.editText_3);
        if( tv == null || tv.getText().length() != 4 ){
            errormsg_ = "input";
            tv.requestFocus();
            return -1;
        }
        gpa += tv.getText() + "-";

        tv = (TextView)findViewById(R.id.editText_4);
        if( tv == null || tv.getText().length() != 5 ){
            errormsg_ = "input";
            tv.requestFocus();
            return -1;
        }
        gpa += tv.getText();

        showDialog();
        client_ = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(60*3, TimeUnit.SECONDS)  // needs 3 minutes for generating md5
                .authenticator(new Authenticator() {
                    public Request authenticate(Route route, Response response) throws IOException {
                        String credential = Credentials.basic(getString(R.string.basic_user), getString(R.string.basic_password));
                        return response.request().newBuilder().header("Authorization", credential).build();
                    }
                })
                .build();

        String url = "http://" + sip_ + "/api/orders/activate";
        try {
            MediaType MIMEType= MediaType.parse("application/json; charset=utf-8");
            RequestBody requestBody = RequestBody.create (MIMEType,"{ \"gpa\": \"" + gpa + "\" }");
            Request request = new Request.Builder().url(url).post(requestBody).build();

            client_.newCall(request).enqueue(new Callback() {
                @Override
                public void onFailure(final Call call, IOException e) {
                    // Error

                    errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
                    dismissDialog();
                }

                @Override
                public void onResponse(Call call, final Response response) throws IOException {
                    String res = response.body().string();
                    try {
                        JSONObject Jobject = new JSONObject(res);
                        if (Jobject.getBoolean("result") == false) {
                            errormsg_ = Jobject.getString("msg");
                            if( errormsg_.equals("not found")){
                                errormsg_ = getString(R.string.order_number_not_found);
                            }
                            if( errormsg_.equals("too much")){
                                errormsg_ = getString(R.string.order_number_used);
                            }

                        }else {
                            result_code_ = RESULT_OK;
                        }
                        dismissDialog();
                        return;

                    }catch(Exception e){
                        errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
                        Log.e(LOG_TAG, "message", e);
                        dismissDialog();
                        return;
                    }
                }
            });


        }catch( Exception e ){
            errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
            Log.e(LOG_TAG, "message", e);
            dismissDialog();
            return -1;
        }
        return 0;
    }

    public void showDialog() {
        if( mProgressDialog == null  ) {
            mProgressDialog = new ProgressDialog(this);
            mProgressDialog.setMessage("Sending...");
            mProgressDialog.show();
        }
    }
    public void dismissDialog() {

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if( mProgressDialog != null ) {
                    if( mProgressDialog.isShowing() ) {
                        mProgressDialog.dismiss();
                    }
                    mProgressDialog = null;
                }

                Intent data = new Intent();
                String text = errormsg_;
                data.putExtra("msg",errormsg_);
                setResult(result_code_, data);
                finish();
                return;
            }
        });

    }

    public void onSubmmit(View view)
    {
        result_code_ = RESULT_CANCELED;
        send();
    }

}
