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

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Handler;
import androidx.annotation.NonNull;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.remoteconfig.FirebaseRemoteConfig;
import com.google.firebase.remoteconfig.FirebaseRemoteConfigSettings;

import org.json.JSONObject;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;
import org.uoyabause.util.IabHelper;
import org.uoyabause.util.IabResult;
import org.uoyabause.util.Purchase;

import java.io.IOException;
import java.security.cert.CertificateException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

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

public class CheckPSerivce extends JobService {

    private static final String TAG = "CheckPSerivce";
    private CheckPTask mCheckpTask;

    @Override
    public boolean onStartJob(final JobParameters jobParameters) {
        Log.d(TAG, "Starting channel creation job");
        mCheckpTask =
                new CheckPTask(getApplicationContext()) {
                    @Override
                    protected void onPostExecute(Boolean success) {
                        super.onPostExecute(success);
                        jobFinished(jobParameters, !success);
                    }
                };
        mCheckpTask.execute();
        return true;
    }

    @Override
    public boolean onStopJob(JobParameters jobParameters) {
        return false;
    }

    private static class CheckPTask extends AsyncTask<Void, Void, Boolean> {

        private final Context mContext;

        CheckPTask(Context context) {
            this.mContext = context;
            handler = new Handler();
            mFirebaseRemoteConfig = FirebaseRemoteConfig.getInstance();
            FirebaseRemoteConfigSettings configSettings = new FirebaseRemoteConfigSettings.Builder()
                    .setDeveloperModeEnabled(BuildConfig.DEBUG)
                    .build();
            mFirebaseRemoteConfig.setConfigSettings(configSettings);
            mFirebaseRemoteConfig.setDefaults(R.xml.config);
            long cacheExpiration = 3600; // 1 hour in seconds.
            // If your app is using developer mode, cacheExpiration is set to 0, so each fetch will
            // retrieve values from the service.
            if (mFirebaseRemoteConfig.getInfo().getConfigSettings().isDeveloperModeEnabled()) {
                cacheExpiration = 0;
            }
            mFirebaseRemoteConfig.fetch(cacheExpiration)
                    .addOnCompleteListener( new OnCompleteListener<Void>() {
                        @Override
                        public void onComplete(@NonNull Task<Void> task) {
                            if (task.isSuccessful()) {
                                mFirebaseRemoteConfig.activateFetched();
                            } else {
                            }
                        }
                    });

        }

        Handler handler;
        IabHelper mHelper;
        private void runOnUiThread(Runnable runnable) {
            handler.post(runnable);
        }

        private static final String ORDER_USER = "order_user";
        private static final String ORDER_KEY = "order_key";
        private FirebaseRemoteConfig mFirebaseRemoteConfig;


        OkHttpClient client_ = null;
        private String sip_ = "192.168.0.8:3000";
        private String errormsg_;
        private final int AS_IDLE = -1;
        private final int AS_ACTIVATING = 0;
        private int itemcount_ = 0;
        ArrayList<Purchase> purchaseList_;
        int current_checkitem_ = 0;

        private int activating_status_ = AS_IDLE;

        private static final int CONTENT_VIEW_ID = 10101010;

        // Called when consumption is complete
        IabHelper.OnConsumeFinishedListener mConsumeFinishedListener = new IabHelper.OnConsumeFinishedListener() {
            public void onConsumeFinished(Purchase purchase, IabResult result) {
                Log.d(TAG, "Consumption finished. Purchase: " + purchase + ", result: " + result);

                // if we were disposed of in the meantime, quit.
                if (mHelper == null) return;

                //check which SKU is consumed here and then proceed.
                if (result.isSuccess()) {
                    Log.d(TAG, "Consumption successful by max installationg.");
                    Toast.makeText(mContext, "You've reached max installation count.", Toast.LENGTH_LONG).show();
                } else {
                    Toast.makeText(mContext, mContext.getString(R.string.error_consume) + result, Toast.LENGTH_LONG).show();

                }
                Log.d(TAG, "End consumption flow.");

            }

        };

        static class MyX509TrustManager implements X509TrustManager {
            public void checkClientTrusted(java.security.cert.X509Certificate[] chain, String authType) throws CertificateException { }
            public void checkServerTrusted(java.security.cert.X509Certificate[] chain, String authType) throws CertificateException { }
            public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                return new java.security.cert.X509Certificate[]{};
            }
        }

        private static  OkHttpClient.Builder getUnsafeOkHttpClientbuilder() {
            try {
                // Create a trust manager that does not validate certificate chains
                final TrustManager[] trustAllCerts = new TrustManager[] {
                        new X509TrustManager() {
                            @Override
                            public void checkClientTrusted(java.security.cert.X509Certificate[] chain, String authType) throws CertificateException {
                            }

                            @Override
                            public void checkServerTrusted(java.security.cert.X509Certificate[] chain, String authType) throws CertificateException {
                            }

                            @Override
                            public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                                return new java.security.cert.X509Certificate[]{};
                            }
                        }
                };

                // Install the all-trusting trust manager
                final SSLContext sslContext = SSLContext.getInstance("SSL");
                sslContext.init(null, trustAllCerts, new java.security.SecureRandom());
                // Create an ssl socket factory with our all-trusting manager
                final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

                OkHttpClient.Builder builder = new OkHttpClient.Builder();
                builder.sslSocketFactory(sslSocketFactory, (X509TrustManager)trustAllCerts[0]);
                builder.hostnameVerifier(new HostnameVerifier() {
                    @Override
                    public boolean verify(String hostname, SSLSession session) {
                        return true;
                    }
                });
                return builder;
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }

        boolean ActivateItem( Purchase purchase ){
            Log.d(TAG, "ActivateItem , purchase: " + purchase);
            String type = "small";
            if (purchase.getSku().equals(DonateActivity.SKU_DONATE_SMALL)) {
                type = "small";
            }
            if (purchase.getSku().equals(DonateActivity.SKU_DONATE_MEDIUM)) {
                type = "midium";
            }

            if (purchase.getSku().equals(DonateActivity.SKU_DONATE_LARGE) || purchase.getSku().equals(DonateActivity.SKU_DONATE_EXTRA_LARGE) ) {
                type ="large";
            }

            activating_status_ = AS_ACTIVATING;
            client_ = getUnsafeOkHttpClientbuilder()
                    .connectTimeout(10, TimeUnit.SECONDS)
                    .writeTimeout(10, TimeUnit.SECONDS)
                    .readTimeout(60*3, TimeUnit.SECONDS)  // needs 3 minutes for generating md5
                    .authenticator(new Authenticator() {
                        public Request authenticate(Route route, Response response) throws IOException {
                            String credential = Credentials.basic(mFirebaseRemoteConfig.getString(ORDER_USER), mFirebaseRemoteConfig.getString(ORDER_KEY));
                            return response.request().newBuilder().header("Authorization", credential).build();
                        }
                    })
                    .build();

            sip_ = mContext.getString(R.string.check_order_url);
            String url = sip_ + "/api/orders/activate";
            try {
                MediaType MIMEType= MediaType.parse("application/json; charset=utf-8");

                String req = "{ ";
                req += "\"gpa\": \"" + purchase.getDeveloperPayload() + "\",";
                req += "\"type\": \"" + type + "\",";
                req += "\"sku\": \"" + purchase.getSku() + "\",";
                req += "\"token\": \"" + purchase.getToken() + "\",";
                req += "\"gorder\": \"" + purchase.getOrderId() + "\" }";

                RequestBody requestBody = RequestBody.create (MIMEType,req);
                Request request = new Request.Builder().url(url).post(requestBody).build();

                client_.newCall(request).enqueue(new Callback() {
                    @Override
                    public void onFailure(final Call call, IOException e) {
                        errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                        Log.d(TAG, "ActivateItem failed: " + errormsg_);
                        activating_status_ = AS_IDLE;
                        return;
                    }

                    @Override
                    public void onResponse(Call call, final Response response) throws IOException {
                        String res = response.body().string();
                        try {
                            JSONObject Jobject = new JSONObject(res);
                            if (Jobject.getBoolean("result") == false) {
                                errormsg_ = Jobject.getString("msg");
                                if( errormsg_.equals("not found")){
                                    errormsg_ = mContext.getString(R.string.order_number_not_found);
                                }
                                if( errormsg_.equals("too much")){
                                    errormsg_ = mContext.getString(R.string.order_number_used);
                                }
                                if( errormsg_.equals("bad token")){
                                    errormsg_ = mContext.getString(R.string.order_number_used);
                                    runOnUiThread(new Runnable() {
                                       @Override
                                       public void run() {
                                            Toast.makeText(mContext, "Something wrong in your payment. Please contact smiyaxdev@gmail.com", Toast.LENGTH_LONG).show();
                                        }
                                    });
                                }
                            }else {

                            }
                            activating_status_ = AS_IDLE;

                        }catch(Exception e) {
                            errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                            activating_status_ = AS_IDLE;
                        }
                        Log.d(TAG, "ActivateItem result: " + errormsg_);
                    }
                });
            }catch( Exception e ){
                errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                Log.d(TAG, "ActivateItem failed: " + errormsg_);
                activating_status_ = AS_IDLE;
                return false;
            }
            return true;
        }

        int CheckItemStatus(final Purchase purchase, int id ) {
            if( activating_status_ != AS_IDLE) return -1;

            SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putString("donate_payload", purchase.getDeveloperPayload());
            editor.putString( "donate_item", purchase.getSku());
            editor.apply();

            activating_status_ = AS_ACTIVATING;
            OkHttpClient.Builder clientBuilder = getUnsafeOkHttpClientbuilder()
                    .connectTimeout(10, TimeUnit.SECONDS)
                    .writeTimeout(10, TimeUnit.SECONDS)
                    .readTimeout(60*3, TimeUnit.SECONDS)  // needs 3 minutes for generating md5
                    .authenticator(new Authenticator() {
                        public Request authenticate(Route route, Response response) throws IOException {
                            String credential = Credentials.basic(mFirebaseRemoteConfig.getString(ORDER_USER), mFirebaseRemoteConfig.getString(ORDER_KEY));
                            return response.request().newBuilder().header("Authorization", credential).build();
                        }
                    });

            client_ = clientBuilder.build();

            sip_ = mContext.getString(R.string.check_order_url);
            String url = sip_ + "/api/orders/getstatus";
            try {
                MediaType MIMEType = MediaType.parse("application/json; charset=utf-8");

                String req = "{ ";
                req += "\"gpa\": \"" + purchase.getDeveloperPayload() + "\",";
                req += "\"sku\": \"" + purchase.getSku() + "\",";
                req += "\"token\": \"" + purchase.getToken() + "\",";
                req += "\"gorder\": \"" + purchase.getOrderId() + "\" }";

                Log.d(TAG, "reqbody :" + req);
                RequestBody requestBody = RequestBody.create(MIMEType, req);
                Request request = new Request.Builder().url(url).post(requestBody).build();
                client_.newCall(request).enqueue(new Callback() {
                    @Override
                    public void onFailure(final Call call, IOException e) {
                        SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                        SharedPreferences.Editor editor = prefs.edit();
                        editor.putBoolean("donated", true);
                        editor.putBoolean("uoyabause_donation", false);
                        editor.apply();
                        errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                        Log.d(TAG, "CheckItem failed but accepted :" + errormsg_);
                        activating_status_ = AS_IDLE;
                    }
                    @Override
                    public void onResponse(Call call, final Response response) throws IOException {
                        String res = response.body().string();
                        try {
                            activating_status_ = AS_IDLE;
                            JSONObject Jobject = new JSONObject(res);
                            SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                            SharedPreferences.Editor editor = prefs.edit();
                            if (Jobject.getBoolean("result") == false) {
                                errormsg_ = Jobject.getString("msg");
                                if( errormsg_.equals("bad token") || errormsg_.equals("canceled") ){
                                    editor.putBoolean("donated", false);
                                    editor.apply();
                                    Log.d(TAG, "CheckItem failed:" + errormsg_);
                                    //current_checkitem_++;
                                    //if( current_checkitem_ < purchaseList_.size() ){
                                    //    Purchase p = purchaseList_.get(current_checkitem_);
                                    //    CheckItemStatus(p,current_checkitem_);
                                    //}
                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            mHelper.consumeAsync(purchase,mConsumeFinishedListener);
                                        }
                                    });

                                }else{
                                    editor.putBoolean("donated", true);
                                    editor.putBoolean("uoyabause_donation", false);
                                    editor.apply();
                                    Log.d(TAG, "CheckItem failed but accepted :" + errormsg_);
                                }
                            }else {
                                editor.putBoolean("donated", true);
                                editor.putBoolean("uoyabause_donation", false);
                                int count = Jobject.getInt("count");
                                editor.putInt("donate_activate_count", count);
                                editor.apply();
                            }
                        }catch(Exception e) {
                            errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                        }
                    }
                });
            }catch( Exception e ){
                errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                Log.d(TAG, "CheckItem failed:" + errormsg_);
                activating_status_ = AS_IDLE;
                return 0;
            }
            return 0;
        }
        boolean UpdateDonationStatus(final Purchase purchase ){

            SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean("donated", true);
            editor.putBoolean("uoyabause_donation", false);
            editor.putString("donate_payload", purchase.getDeveloperPayload());
            editor.putString( "donate_item", purchase.getSku());
            editor.apply();

            activating_status_ = AS_ACTIVATING;
            Log.d(TAG, "CheckItem , purchase: " + purchase);

            OkHttpClient.Builder clientBuilder = getUnsafeOkHttpClientbuilder()
                    .connectTimeout(10, TimeUnit.SECONDS)
                    .writeTimeout(10, TimeUnit.SECONDS)
                    .readTimeout(60*3, TimeUnit.SECONDS)  // needs 3 minutes for generating md5
                    .authenticator(new Authenticator() {
                        public Request authenticate(Route route, Response response) throws IOException {
                            String credential = Credentials.basic(mFirebaseRemoteConfig.getString(ORDER_USER), mFirebaseRemoteConfig.getString(ORDER_KEY));
                            return response.request().newBuilder().header("Authorization", credential).build();
                        }
                    });

            client_ = clientBuilder.build();

            sip_ = mContext.getString(R.string.check_order_url);
            String url = sip_ + "/api/orders/check";
            try {
                MediaType MIMEType= MediaType.parse("application/json; charset=utf-8");
                RequestBody requestBody = RequestBody.create (MIMEType,"{ \"gpa\": \"" + purchase.getDeveloperPayload() + "\" }");
                Request request = new Request.Builder().url(url).post(requestBody).build();

                client_.newCall(request).enqueue(new Callback() {
                    @Override
                    public void onFailure(final Call call, IOException e) {
                        errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                        Log.d(TAG, "CheckItem failed:" + errormsg_);
                        activating_status_ = AS_IDLE;
                        return;
                    }

                    @Override
                    public void onResponse(Call call, final Response response) throws IOException {
                        String res = response.body().string();
                        try {
                            JSONObject Jobject = new JSONObject(res);
                            if (Jobject.getBoolean("result") == false) {
                                errormsg_ = Jobject.getString("msg");
                                if( errormsg_.equals("canceled")){
                                    SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                                    SharedPreferences.Editor editor = prefs.edit();
                                    editor.putBoolean("donated", false);
                                    editor.putBoolean("uoyabause_donation", false);
                                    editor.apply();
                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            mHelper.consumeAsync(purchase,mConsumeFinishedListener);
                                        }
                                    });
                                }
                                else if( errormsg_.equals("not found")){
                                    errormsg_ = mContext.getString(R.string.order_number_not_found);
                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            ActivateItem(purchase);
                                        }
                                    });
                                }
                                else if( errormsg_.equals("too much")){
                                    errormsg_ = mContext.getString(R.string.order_number_used);

                                    //itemcount_--;
                                    //if( itemcount_ <= 0 ) {
                                    SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                                    SharedPreferences.Editor editor = prefs.edit();
                                    editor.putBoolean("donated", false);
                                    editor.putBoolean("uoyabause_donation", false);
                                    editor.apply();
                                    //}

                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            mHelper.consumeAsync(purchase,mConsumeFinishedListener);
                                        }
                                    });

                                }
                            }else {
                                SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                                SharedPreferences.Editor editor = prefs.edit();
                                int count = Jobject.getInt("count");
                                editor.putInt("donate_activate_count", count);
                                editor.apply();
                            }
                            Log.d(TAG, "CheckItem Finished:" + errormsg_);
                            activating_status_ = AS_IDLE;
                            return;

                        }catch(Exception e){
                            errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                            activating_status_ = AS_IDLE;
                            return;
                        }
                    }
                });
            }catch( Exception e ){
                errormsg_ = mContext.getString(R.string.network_error) + e.getLocalizedMessage();
                Log.d(TAG, "CheckItem failed:" + errormsg_);
                activating_status_ = AS_IDLE;
                return false;
            }
            return true;
        }

        void CheckDonated() {
            String base64EncodedPublicKey = mContext.getString(R.string.donation_publickey);
            mHelper = new IabHelper(mContext, base64EncodedPublicKey);
            mHelper.startSetup(new IabHelper.OnIabSetupFinishedListener() {
                public void onIabSetupFinished(IabResult result) {
                    Log.d(TAG, "Setup finished.");
                    purchaseList_ = null;
                    try {
                        purchaseList_ = mHelper.queryPurchases();
                    } catch (Exception e) {
                        return;
                    }

                    SharedPreferences prefs = mContext.getSharedPreferences("private", Context.MODE_PRIVATE);
                    Boolean hasDonated = prefs.getBoolean("donated", false);
                    if( hasDonated == false ) {
                        if (purchaseList_ == null || purchaseList_.size() == 0) {
                            return;
                        }
                        itemcount_ = purchaseList_.size();
                        for (int i = 0; i < purchaseList_.size(); i++) {
                            Purchase p = purchaseList_.get(i);
                            //if( p.getPurchaseState() == 0 ) {
                            UpdateDonationStatus(p);
                            //}
                        }

                    }else{

                        Boolean uoyabause_donation = prefs.getBoolean("uoyabause_donation", false);
                        if(uoyabause_donation){
                            return;
                        }

                        if (purchaseList_ == null || purchaseList_.size() == 0) {
                            //SharedPreferences.Editor editor = prefs.edit();
                            //editor.putBoolean("donated", false);
                            //editor.apply();
                            SharedPreferences.Editor editor = prefs.edit();
                            editor.putBoolean("uoyabause_donation", true);
                            editor.apply();
                            return;
                        }
                        current_checkitem_ = 0;
                        Purchase p = purchaseList_.get(current_checkitem_);
                        CheckItemStatus(p,current_checkitem_);
                        //for (int i = 0; i < purchaseList_.size(); i++) {
                        //    Purchase p = purchaseList_.get(i);
                        //    // 0 (purchased), 1 (canceled), or 2 (refunded).
                        //    if( p.getPurchaseState() == 0 ){
                        //        CheckItemStatus(p,i);
                        //    }
                        //}
                    }
                }
            });
        }

        @Override
        protected Boolean doInBackground(Void... voids) {
            CheckDonated();
            return true;
        }
    }

}
