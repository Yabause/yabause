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
/* Copyright (c) 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.uoyabause.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.text.util.Linkify;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.google.firebase.remoteconfig.FirebaseRemoteConfig;

import org.json.JSONObject;
import org.uoyabause.uranus.R;
import org.uoyabause.util.IabHelper;
import org.uoyabause.util.IabResult;
import org.uoyabause.util.Inventory;
import org.uoyabause.util.Purchase;
import org.uoyabause.util.SkuDetails;

import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
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

import static com.activeandroid.Cache.getContext;


/**
 * Example game using in-app billing version 3.
 *
 * Before attempting to run this sample, please read the README file. It
 * contains important information on how to set up this project.
 *
 * All the game-specific logic is implemented here in MainActivity, while the
 * general-purpose boilerplate that can be reused in any app is provided in the
 * classes in the util/ subdirectory. When implementing your own application,
 * you can copy over util/*.java to make use of those utility classes.
 *
 * This game is a simple "driving" game where the player can buy gas
 * and drive. The car has a tank which stores gas. When the player purchases
 * gas, the tank fills up (1/4 tank at a time). When the player drives, the gas
 * in the tank diminishes (also 1/4 tank at a time).
 *
 * The user can also purchase a "premium upgrade" that gives them a red car
 * instead of the standard blue one (exciting!).
 *
 * The user can also purchase a subscription ("infinite gas") that allows them
 * to drive without using up any gas while that subscription is active.
 *
 * It's important to note the consumption mechanics for each item.
 *
 * PREMIUM: the item is purchased and NEVER consumed. So, after the original
 * purchase, the player will always own that item. The application knows to
 * display the red car instead of the blue one because it queries whether
 * the premium "item" is owned or not.
 *
 * INFINITE GAS: this is a subscription, and subscriptions can't be consumed.
 *
 * GAS: when gas is purchased, the "gas" item is then owned. We consume it
 * when we apply that item's effects to our app's world, which to us means
 * filling up 1/4 of the tank. This happens immediately after purchase!
 * It's at this point (and not when the user drives) that the "gas"
 * item is CONSUMED. Consumption should always happen when your game
 * world was safely updated to apply the effect of the purchase. So,
 * in an example scenario:
 *
 * BEFORE:      tank at 1/2
 * ON PURCHASE: tank at 1/2, "gas" item is owned
 * IMMEDIATELY: "gas" is consumed, tank goes to 3/4
 * AFTER:       tank at 3/4, "gas" item NOT owned any more
 *
 * Another important point to notice is that it may so happen that
 * the application crashed (or anything else happened) after the user
 * purchased the "gas" item, but before it was consumed. That's why,
 * on startup, we check if we own the "gas" item, and, if so,
 * we have to apply its effects to our world and consume it. This
 * is also very important!
 *
 * @author Bruno Oliveira (Google)
 */
public class DonateActivity extends Activity {
    // Debug tag, for logging
    static final String TAG = "DonateActivity";

    // SKUS
    public static final String SKU_DONATE_SMALL = "remove_ad_once";
    public static final String SKU_DONATE_MEDIUM = "remove_ad_5";
    public static final String SKU_DONATE_LARGE = "remove_ad_forever";
    public static final String SKU_DONATE_EXTRA_LARGE = "donation_extra_large";
    final int MAX_SKU_COUNT = 3;
    final String skus[] = { SKU_DONATE_SMALL, SKU_DONATE_MEDIUM, SKU_DONATE_LARGE };

    // (arbitrary) request code for the purchase flow
    static final int RC_REQUEST = 10001;

    // The helper object
    IabHelper mHelper;

    private Spinner  selectSpinner;
    private Handler _handler = null;
    private final int STATE_IDLE = 0;
    private final int STATE_QUERYING = 1;
    private final int STATE_QUERYFINIED = 2;
    private int _state = STATE_IDLE;

    private static final String ORDER_USER = "order_user";
    private static final String ORDER_KEY = "order_key";
    private FirebaseRemoteConfig mFirebaseRemoteConfig;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.donation);
        _handler = new Handler();
        UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
            TextView t = (TextView) findViewById(R.id.test1);
            Linkify.addLinks(t, Linkify.ALL);
        }
        mFirebaseRemoteConfig = FirebaseRemoteConfig.getInstance();

        // load game data
        loadData();

        /* base64EncodedPublicKey should be YOUR APPLICATION'S PUBLIC KEY
         * (that you got from the Google Play developer console). This is not your
         * developer public key, it's the *app-specific* public key.
         *
         * Instead of just storing the entire literal string here embedded in the
         * program,  construct the key at runtime from pieces or
         * use bit manipulation (for example, XOR with some other string) to hide
         * the actual key.  The key itself is not secret information, but we don't
         * want to make it easy for an attacker to replace the public key with one
         * of their own and then fake messages from the server.
         */
        String base64EncodedPublicKey = getString(R.string.donation_publickey);

        // Some sanity checks to see if the developer (that's you!) really followed the
        // instructions to run this sample (don't put these checks on your app!)
        if (base64EncodedPublicKey.contains("CONSTRUCT_YOUR")) {
            throw new RuntimeException("Please put your app's public key in MainActivity.java. See README.");
        }
        if (getPackageName().startsWith("com.example")) {
            throw new RuntimeException("Please change the sample's package name! See README.");
        }

        // Create the helper, passing it our context and the public key to verify signatures with
        Log.d(TAG, "Creating IAB helper.");
        mHelper = new IabHelper(this, base64EncodedPublicKey);

        // enable debug logging (for a production application, you should set this to false).
        mHelper.enableDebugLogging(false);

/*
        ArrayAdapter adapter = new ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item);
        adapter.add(getString(R.string.donate_3doller));
        adapter.add(getString(R.string.donate_5doller));
        adapter.add(getString(R.string.donate_10doller));
        adapter.add(getString(R.string.donate_30doller));
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        selectSpinner = (Spinner) findViewById(R.id.Spinner01);
        selectSpinner.setAdapter(adapter);
        h = new Handler();
        // Spawn a thread that triggers the Spinner to open after 5 seconds...
        new Thread(new Runnable() {
            public void run() {
                // DO NOT ATTEMPT TO DIRECTLY UPDATE THE UI HERE, IT WON'T WORK!
                // YOU MUST POST THE WORK TO THE UI THREAD'S HANDLER
                h.postDelayed(new Runnable() {
                    public void run() {
                        // Open the Spinner...
                        selectSpinner.performClick();
                    }
                }, 1000);
            }
        }).start();
*/
        // Start setup. This is asynchronous and the specified listener
        // will be called once setup completes.
        Log.d(TAG, "Starting setup.");
        _state = STATE_QUERYING;
        mHelper.startSetup(new IabHelper.OnIabSetupFinishedListener() {
            public void onIabSetupFinished(IabResult result) {
                Log.d(TAG, "Setup finished.");

                if (!result.isSuccess()) {
                    // Oh noes, there was a problem.
                    complain("Problem setting up in-app billing: " + result);
                    return;
                }

                // Have we been disposed of in the meantime? If so, quit.
                if (mHelper == null) return;

                List<String> listsku = new ArrayList<String>();
                listsku.add(SKU_DONATE_SMALL);
                listsku.add(SKU_DONATE_MEDIUM);
                listsku.add(SKU_DONATE_LARGE);
                //listsku.add(SKU_DONATE_EXTRA_LARGE);


                // IAB is fully set up. Now, let's get an inventory of stuff we own.
                //Log.d(TAG, "Setup successful. Querying inventory.");
                mHelper.queryInventoryAsync(true,listsku,mGotInventoryListener);
            }
        });
    }

    // Listener that's called when we finish querying the items and subscriptions we own

    IabHelper.QueryInventoryFinishedListener mGotInventoryListener = new IabHelper.QueryInventoryFinishedListener() {
        public void onQueryInventoryFinished(IabResult result, Inventory inventory) {
            Log.d(TAG, "Query inventory finished.");
            _state = STATE_QUERYFINIED;

            // Have we been disposed of in the meantime? If so, quit.
            if (mHelper == null) return;

            // Is it a failure?
            if (result.isFailure()) {
                complain("Failed to query inventory: " + result);
                return;
            }

            Log.d(TAG, "Query inventory was successful.");


            //SkuDetails s0 = inventory.getSkuDetails(SKU_DONATE_SMALL);
            //if( rb != null && s0 != null ) {
            //    rb.setText( s0.getPrice() + ": Remove Ad and this message for a month" );
           //}

            ArrayAdapter adapter = new ArrayAdapter(DonateActivity.this, android.R.layout.simple_spinner_dropdown_item);

            for( int i=0; i<MAX_SKU_COUNT; i++ ) {

                SkuDetails s0 = inventory.getSkuDetails(skus[i]);
                Purchase gasPurchase = inventory.getPurchase(skus[i]);
                if( gasPurchase != null && verifyDeveloperPayload(gasPurchase) ){

                }else {

                    if (s0 != null) {
                        String name = s0.getPrice() + " : " + s0.getDescription();
                        switch(i){
                            case 0: {
                                RadioButton rv = (RadioButton) findViewById(R.id.donate_0);
                                rv.setText(name);
                            }
                            break;
                            case 1: {
                                RadioButton rv = (RadioButton) findViewById(R.id.donate_1);
                                rv.setText(name);
                            }
                            break;
                            case 2: {
                                RadioButton rv = (RadioButton) findViewById(R.id.donate_2);
                                rv.setText(name);
                            }
                            break;
                        }
                        //adapter.add(name);
                    }
                }

                //Purchase gasPurchase = inventory.getPurchase(skus[i]);
                //if( gasPurchase != null && verifyDeveloperPayload(gasPurchase) ){
                //    mHelper.consumeAsync(gasPurchase, mConsumeFinishedListener);
                //}
            }
/*
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            selectSpinner = (Spinner) findViewById(R.id.Spinner01);
            selectSpinner.setAdapter(adapter);
            if( _handler != null ) {
                _handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        selectSpinner.performClick();
                    }
                }, 1000);
            }
*/
/*
            h = new Handler();
            // Spawn a thread that triggers the Spinner to open after 5 seconds...
            new Thread(new Runnable() {
                public void run() {
                    // DO NOT ATTEMPT TO DIRECTLY UPDATE THE UI HERE, IT WON'T WORK!
                    // YOU MUST POST THE WORK TO THE UI THREAD'S HANDLER
                    h.postDelayed(new Runnable() {
                        public void run() {
                            // Open the Spinner...
                            selectSpinner.performClick();
                        }
                    }, 1000);
                }
            }).start();
*/
            RadioGroup radioGroup = (RadioGroup) findViewById(R.id.donate_radio_group);
            radioGroup.check(R.id.donate_1);

            updateUi();
            setWaitScreen(false);
            Log.d(TAG, "Initial inventory query finished; enabling main UI.");
        }
    };


    private String payload_ = null;
    public void onDonate(View view)
    {
        //int pos = selectSpinner.getSelectedItemPosition();
        //if( pos < 0 || pos >= MAX_SKU_COUNT ) return;
        int pos = -1;

        RadioGroup radioGroup = (RadioGroup) findViewById(R.id.donate_radio_group);
        int checkid = radioGroup.getCheckedRadioButtonId();
        switch(checkid){
            case R.id.donate_0:
                pos = 0;
                break;
            case R.id.donate_1:
                pos = 1;
                break;
            case R.id.donate_2:
                pos = 2;
                break;
        }
        if( pos == -1 ){
            return;
        }

        setWaitScreen(true);

        String android_id = Settings.Secure.getString(getContext().getContentResolver(),
                Settings.Secure.ANDROID_ID);
        String date = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss").format(new Date());

        android_id = android_id + "xxx" + date;

        MessageDigest md = null;

        try {
            md = MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException e) {
            return;
        }
        md.update(android_id.getBytes());
        StringBuilder sb = new StringBuilder();
        for (byte b : md.digest()) {
            String hex = String.format("%02x", b);
            sb.append(hex);
        }


        payload_ = sb.toString();
        mHelper.launchPurchaseFlow(this, skus[pos], RC_REQUEST, mPurchaseFinishedListener, payload_);

    }

    public void onDoNotDonate(View view)
    {
        finish();
    }

    final int CHECK_PAYMENT = 32;
    public void onIhavedonated(View view)
    {

        Intent intent = new Intent(this, CheckPaymentActivity.class );
        startActivityForResult(intent, CHECK_PAYMENT);

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult(" + requestCode + "," + resultCode + "," + data);
        if( CHECK_PAYMENT == requestCode ){
            super.onActivityResult(requestCode, resultCode, data);
            if( resultCode == RESULT_OK){

                SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean("donated", true);
                editor.putBoolean("uoyabause_donation", true);
                editor.apply();

                Toast.makeText(DonateActivity.this, getString(R.string.thank_you), Toast.LENGTH_LONG).show();
                finish();

            }else{

                String msg = "";
                if( data != null ) {
                    msg = data.getStringExtra("msg");
                    new androidx.appcompat.app.AlertDialog.Builder(this)
                            .setTitle("Failed")
                            .setMessage(msg)
                            .setPositiveButton("OK", null)
                            .show();
                }

            }

        }else {
            if (mHelper == null) return;

            // Pass on the activity result to the helper for handling
            if (!mHelper.handleActivityResult(requestCode, resultCode, data)) {
                // not handled, so handle it ourselves (here's where you'd
                // perform any handling of activity results not related to in-app
                // billing...
                super.onActivityResult(requestCode, resultCode, data);
            } else {
                Log.d(TAG, "onActivityResult handled by IABUtil.");
            }
        }
    }

    /** Verifies the developer payload of a purchase. */
    boolean verifyDeveloperPayload(Purchase p) {
        String payload = p.getDeveloperPayload();

        if( payload_ == null ){
            return false;
        }

        if( !payload.equals(payload_) ){
            return false;
        }

        /*
         * TODO: verify that the developer payload of the purchase is correct. It will be
         * the same one that you sent when initiating the purchase.
         *
         * WARNING: Locally generating a random string when starting a purchase and
         * verifying it here might seem like a good approach, but this will fail in the
         * case where the user purchases an item on one device and then uses your app on
         * a different device, because on the other device you will not have access to the
         * random string you originally generated.
         *
         * So a good developer payload has these characteristics:
         *
         * 1. If two different users purchase an item, the payload is different between them,
         *    so that one user's purchase can't be replayed to another user.
         *
         * 2. The payload must be such that you can verify it even when the app wasn't the
         *    one who initiated the purchase flow (so that items purchased by the user on
         *    one device work on other devices owned by the user).
         *
         * Using your own server to store and verify developer payloads across app
         * installations is recommended.
         */

        return true;
    }

    OkHttpClient client_ = null;
    //private final String sip_ = "www.uoyabause.org";
    private String sip_ = "192.168.0.8:3000";
    private String errormsg_;
    private final int AS_IDLE = -1;
    private final int AS_ACTIVATING = 0;

    private int activating_status_ = AS_IDLE;

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
    boolean ActivateItem( Purchase purchase, String type ){
        Log.d(TAG, "ActivateItem , purchase: " + purchase);

        SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putString("donate_payload", purchase.getDeveloperPayload());
        editor.putString( "donate_item", purchase.getSku());
        editor.apply();

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

        sip_ = getString(R.string.check_order_url);
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
                    errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
                    Log.d(TAG, "ActivateItem failed: " + errormsg_);
                    activating_status_ = AS_IDLE;
                    setWaitScreen(false);
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
                                errormsg_ = getString(R.string.order_number_not_found);
                            }
                            if( errormsg_.equals("too much")){
                                errormsg_ = getString(R.string.order_number_used);
                            }
                            if( errormsg_.equals("bad token")){
                                errormsg_ = getString(R.string.order_number_used);
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        Toast.makeText(DonateActivity.this, "Something wrong in your payment. Please Contact Developer", Toast.LENGTH_LONG).show();
                                        finish();
                                    }
                                });
                            }
                        }else {
                            int count = Jobject.getInt("count");
                            SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
                            SharedPreferences.Editor editor = prefs.edit();
                            editor.putInt("donate_activate_count", count);
                            editor.apply();
                        }
                        activating_status_ = AS_IDLE;
                        setWaitScreen(false);

                    }catch(Exception e) {
                        errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
                        activating_status_ = AS_IDLE;
                        setWaitScreen(false);
                    }

                    SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = prefs.edit();
                    editor.putBoolean("donated", true);
                    editor.putBoolean("uoyabause_donation", false);
                    editor.apply();
                    Log.d(TAG, "Purchase successful.");

                    Log.d(TAG, "ActivateItem result: " + errormsg_);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Consumption successful. Provisioning.");
                            Toast.makeText(DonateActivity.this, getString(R.string.thank_you), Toast.LENGTH_LONG).show();
                            finish();
                        }
                    });

                }
            });
        }catch( Exception e ){
            errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
            Log.d(TAG, "ActivateItem failed: " + errormsg_);
            activating_status_ = AS_IDLE;
            setWaitScreen(false);
            return false;
        }
         return true;
    }

    // Callback for when a purchase is finished
    IabHelper.OnIabPurchaseFinishedListener mPurchaseFinishedListener = new IabHelper.OnIabPurchaseFinishedListener() {
        public void onIabPurchaseFinished(IabResult result, Purchase purchase) {
            Log.d(TAG, "Purchase finished: " + result + ", purchase: " + purchase);

            // if we were disposed of in the meantime, quit.
            if (mHelper == null) return;

            if (result.isFailure()) {
                complain("Error purchasing: " + result);
                setWaitScreen(false);
                return;
            }
            if (!verifyDeveloperPayload(purchase)) {
                complain("Error purchasing. Authenticity verification failed.");
                setWaitScreen(false);
                return;
            }


            if (purchase.getSku().equals(SKU_DONATE_SMALL)) {
                ActivateItem( purchase, "small" );
            }
            if (purchase.getSku().equals(SKU_DONATE_MEDIUM)) {
                ActivateItem( purchase, "midium" );
            }

            if (purchase.getSku().equals(SKU_DONATE_LARGE) || purchase.getSku().equals(SKU_DONATE_EXTRA_LARGE) ) {
                ActivateItem( purchase, "large" );
            }
            //Log.d(TAG, "Purchase successful.");
        }
    };

    // Called when consumption is complete
    IabHelper.OnConsumeFinishedListener mConsumeFinishedListener = new IabHelper.OnConsumeFinishedListener() {
        public void onConsumeFinished(Purchase purchase, IabResult result) {
            Log.d(TAG, "Consumption finished. Purchase: " + purchase + ", result: " + result);

            // if we were disposed of in the meantime, quit.
            if (mHelper == null) return;

            //check which SKU is consumed here and then proceed.
            if (result.isSuccess()) {
                Log.d(TAG, "Consumption successful. Provisioning.");
                Toast.makeText(DonateActivity.this, getString(R.string.thank_you), Toast.LENGTH_LONG).show();

                SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean("donated", true);
                editor.putBoolean("uoyabause_donation", false);
                editor.apply();

            } else {
                Toast.makeText(DonateActivity.this, getString(R.string.error_consume) + result, Toast.LENGTH_LONG).show();

            }

            Log.d(TAG, "End consumption flow.");
            finish();

        }

    };


    // We're being destroyed. It's important to dispose of the helper here!
    @Override
    public void onDestroy() {

        if( _state == STATE_QUERYING ){
            int timeout_cnt = 0;
            while(_state == STATE_QUERYING){
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                if( timeout_cnt++ >= 10 ){
                    break;
                }
            }
        }

        if( activating_status_ == AS_ACTIVATING ){
            int timeout_cnt = 0;
            while(activating_status_ == AS_ACTIVATING){
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                if( timeout_cnt++ >= 10 ){
                    break;
                }
            }
        }

        if( _handler != null ) {
            _handler.removeCallbacksAndMessages(null);
        }

        // very important:
        Log.d(TAG, "Destroying helper.");
        if (mHelper != null) {
            mHelper.dispose();
            mHelper = null;
        }

        super.onDestroy();
    }

    // updates UI to reflect model
    public void updateUi() {

    }

    // Enables or disables the "please wait" screen.
    void setWaitScreen(final boolean set) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                findViewById(R.id.screen_main).setVisibility(set ? View.GONE : View.VISIBLE);
                findViewById(R.id.screen_wait).setVisibility(set ? View.VISIBLE : View.GONE);
            }
        });

    }

    void complain(String message) {
        Log.e(TAG, "**** TrivialDrive Error: " + message);
        alert("Error: " + message);
    }

    void alert(String message) {
        AlertDialog.Builder bld = new AlertDialog.Builder(this);
        bld.setMessage(message);
        bld.setNeutralButton("OK", null);
        Log.d(TAG, "Showing alert dialog: " + message);
        bld.create().show();
    }

    void saveData() {

    }

    void loadData() {

    }
}