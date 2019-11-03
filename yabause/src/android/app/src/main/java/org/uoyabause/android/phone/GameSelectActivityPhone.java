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

package org.uoyabause.android.phone;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.app.FragmentTransaction;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.ViewCompat;

import android.util.Log;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Toast;

import net.nend.android.NendAdListener;
import net.nend.android.NendAdView;

import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;

public class GameSelectActivityPhone extends AppCompatActivity implements NendAdListener {

    private static final int CONTENT_VIEW_ID = 10101010;
    GameSelectFragmentPhone frg_;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        FrameLayout frame = new FrameLayout(this);
        frame.setId(CONTENT_VIEW_ID);
        setContentView(frame, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        if (savedInstanceState == null) {
            frg_ = new GameSelectFragmentPhone();
            FragmentTransaction ft = getFragmentManager().beginTransaction();
            ft.add(CONTENT_VIEW_ID, frg_).commit();
        }else{
            frg_ = (GameSelectFragmentPhone)getFragmentManager().findFragmentById(CONTENT_VIEW_ID);
        }

        if( !BuildConfig.BUILD_TYPE.equals("pro") ) {
            SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
            Boolean hasDonated = prefs.getBoolean("donated", false);
            if (hasDonated == false) {
                try {
                    NendAdView nendAdView = new NendAdView(this, Integer.parseInt(getString(R.string.nend_spoid)), getString(R.string.nend_apikey));
                    nendAdView.setListener(this);
                    FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT);
                    params.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                    frame.addView(nendAdView, params);
                    nendAdView.bringToFront();
                    nendAdView.invalidate();
                    ViewCompat.setTranslationZ(nendAdView, 90);
                    nendAdView.loadAd();
                }catch(Exception e){

                }
            }
        }
/*
        GameSelectFragmentPhone frg = new GameSelectFragmentPhone();
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        //transaction.replace(R.id.fragment_container, newFragment);
        transaction.add(frg,"mainfrag");
        transaction.addToBackStack(null);
        transaction.commit();
*/
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if(frg_!=null){
            frg_.onConfigurationChanged(newConfig);
        }
        //mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Pass the event to ActionBarDrawerToggle, if it returns
        // true, then it has handled the app icon touch event
        //if (mDrawerToggle.onOptionsItemSelected(item)) {
        //    return true;
        //}
        if(frg_!=null){
            boolean rtn = frg_.onOptionsItemSelected(item);
            if(rtn==true){
                return true;
            }
        }
        return super.onOptionsItemSelected(item);
    }

    public void onFailedToReceiveAd(NendAdView nendAdView) {
        //Toast.makeText(getApplicationContext(), "onFailedToReceiveAd", Toast.LENGTH_LONG).show();
        NendAdView.NendError nendError = nendAdView.getNendError();
        switch (nendError) {
            case INVALID_RESPONSE_TYPE:
                break;
            case FAILED_AD_DOWNLOAD:
                break;
            case FAILED_AD_REQUEST:
                break;
            case AD_SIZE_TOO_LARGE:
                break;
            case AD_SIZE_DIFFERENCES:
                break;
        }
        Log.e("nend", nendError.getMessage());

    }

    public void onReceiveAd(NendAdView nendAdView) {
        //Toast.makeText(getApplicationContext(), "onReceiveAd", Toast.LENGTH_LONG).show();
        nendAdView.bringToFront();
        nendAdView.invalidate();
    }

    public void onClick(NendAdView nendAdView) {
        //Toast.makeText(getApplicationContext(), "onClick", Toast.LENGTH_LONG).show();
    }

    public void onDismissScreen(NendAdView nendAdView) {
        //Toast.makeText(getApplicationContext(), "onDismissScreen", Toast.LENGTH_LONG).show();
    }

}
