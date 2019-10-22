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

import android.Manifest;
import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.FragmentActivity;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.view.ContextThemeWrapper;
import android.view.View;
import android.widget.CheckBox;

import com.crashlytics.android.Crashlytics;
import com.firebase.ui.auth.AuthUI;
import com.firebase.ui.auth.ErrorCodes;
import com.firebase.ui.auth.IdpResponse;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.analytics.FirebaseAnalytics;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;

import org.uoyabause.uranus.R;

import java.io.File;
import java.util.Arrays;
import java.util.Calendar;

import io.reactivex.Observable;
import io.reactivex.ObservableEmitter;
import io.reactivex.ObservableOnSubscribe;
import io.reactivex.Observer;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.schedulers.Schedulers;

public class GameSelectPresenter {

    private FirebaseAnalytics mFirebaseAnalytics;
    public  interface GameSelectPresenterListener {
        //void onUpdateGameList( );
        void onShowMessage( int string_id );
    }

    void updateGameDatabaseRx( Observer observer ){
        Observable.create(new ObservableOnSubscribe<String>() {
            @Override
            public void subscribe(ObservableEmitter<String> emitter){
                YabauseStorage ybs = YabauseStorage.getStorage();
                ybs.setProcessEmmiter(emitter);
                ybs.generateGameDB(refresh_level_);
                ybs.setProcessEmmiter(null);
                refresh_level_ = 0;
                emitter.onComplete();
            }
        }).observeOn(AndroidSchedulers.mainThread())
            .subscribeOn(Schedulers.io())
            .subscribe(observer);
    }


    int refresh_level_ = 0;
    android.app.Fragment target_ = null ;
    GameSelectPresenterListener listener_ = null ;

    public static final int RC_SIGN_IN = 123;

    private String username_;
    private Uri photo_url_;

    public GameSelectPresenter( android.app.Fragment target,  GameSelectPresenterListener listener ){
        target_ = target;
        listener_ = listener;
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(target_.getActivity());
    }



    public void updateGameList( int refresh_level, Observer observer ){

        refresh_level_ = refresh_level;
        Activity activity = target_.getActivity();
        if( activity == null ) return;

        if (Build.VERSION.SDK_INT >= 23) {
            // Verify that all required contact permissions have been granted.
            if (ActivityCompat.checkSelfPermission(activity, android.Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED
                    || ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                return;
            }
        }

        File[] external_AND_removable_storage_m1 = activity.getExternalFilesDirs(null);
        if( external_AND_removable_storage_m1.length > 1 && external_AND_removable_storage_m1[1] != null ){
            String path = external_AND_removable_storage_m1[1].toString();
            YabauseStorage ys = YabauseStorage.getStorage();
            ys.setExternalStoragePath(path/*external_path*/);
        }else{
            SharedPreferences sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(activity);
            SharedPreferences.Editor editor = sharedPrefwrite.edit();
            editor.putString("pref_game_download_directory", "0");
            editor.apply();
        }
        updateGameDatabaseRx( observer );
    }


    public void signIn() {
        target_.startActivityForResult(
                AuthUI.getInstance()
                        .createSignInIntentBuilder()
                        .setAvailableProviders( Arrays.asList(/*new AuthUI.IdpConfig.Builder(AuthUI.TWITTER_PROVIDER).build(),
                                            new AuthUI.IdpConfig.Builder(AuthUI.FACEBOOK_PROVIDER).build(),*/
                                new AuthUI.IdpConfig.GoogleBuilder().build()))
                        .build(),
                GameSelectPresenter.RC_SIGN_IN);
    }

    public void signOut() {
        AuthUI.getInstance()
                .signOut((FragmentActivity)target_.getActivity())
                .addOnCompleteListener(new OnCompleteListener<Void>() {
                    public void onComplete(@NonNull Task<Void> task) {
                        // user is now signed out
                        //startActivity(new Intent(MyActivity.this, SignInActivity.class));
                        //finish();
                        //listener_.onUpdateGameList();
                    }
                });
    }

    public void onSignIn( int resultCode, Intent data){
        IdpResponse response = IdpResponse.fromResultIntent(data);
        // Successfully signed in
        if (resultCode == Activity.RESULT_OK) {
            response.getIdpToken();
            String token = response.getIdpToken();
            //listener_.onUpdateGameList();
            FirebaseAuth auth = FirebaseAuth.getInstance();
            DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
            String baseurl = "/user-posts/" + auth.getCurrentUser().getUid();
            if( auth.getCurrentUser().getDisplayName() != null ){
                username_ = auth.getCurrentUser().getDisplayName();
                baseref.child(baseurl).child("name").setValue(auth.getCurrentUser().getDisplayName());
            }
            if( auth.getCurrentUser().getEmail() != null ) baseref.child(baseurl).child("email").setValue(auth.getCurrentUser().getEmail());
            if( auth.getCurrentUser().getPhotoUrl() != null ) {
//                baseref.child(baseurl).child("photo").setValue(auth.getCurrentUser().getPhotoUrl());
//                photo_url_ = auth.getCurrentUser().getPhotoUrl();
            }

            Crashlytics.setUserIdentifier( username_ + "_" + auth.getCurrentUser().getEmail());
            mFirebaseAnalytics.setUserId(username_ + "_" + auth.getCurrentUser().getEmail());
            mFirebaseAnalytics.setUserProperty ("name", username_ + "_" + auth.getCurrentUser().getEmail());

            //startActivity(SignedInActivity.createIntent(this, response));
            YabauseApplication application = (YabauseApplication)target_.getActivity().getApplication();
            if( application != null ) {
                Crashlytics crash = application.getCrashlytics();
                crash.setUserName(auth.getCurrentUser().getDisplayName());
                crash.setUserEmail(auth.getCurrentUser().getEmail());
                crash.setUserIdentifier(auth.getUid());
            }
            return;
        } else {

            username_ = null;
            photo_url_ = null;

            // Sign in failed
            if (response == null) {
                // User pressed back button
                listener_.onShowMessage(R.string.sign_in_cancelled);
                return;
            }

            if (response.getError().getErrorCode() == ErrorCodes.NO_NETWORK) {
                listener_.onShowMessage(R.string.no_internet_connection);
                return;
            }

            if (response.getError().getErrorCode() == ErrorCodes.UNKNOWN_ERROR) {
                listener_.onShowMessage(R.string.unknown_error);
                return;
            }
        }
        listener_.onShowMessage(R.string.unknown_sign_in_response);

    }

    public String getCurrentUserName(){
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() != null) {
            return auth.getCurrentUser().getDisplayName();
        }
        return null;
    }

    public Uri getCurrentUserPhoto(){
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() != null) {
            return auth.getCurrentUser().getPhotoUrl();
        }
        return null;
    }

    public void checkSignIn(){

        if( target_.getActivity() == null ){
            return; // Activity has benn detached.
        }

        SharedPreferences check_prefernce = PreferenceManager.getDefaultSharedPreferences(target_.getActivity());
        boolean do_not_ask = check_prefernce.getBoolean("pref_dont_ask_signin", false);
        if( do_not_ask == true ){
            FirebaseAuth auth = FirebaseAuth.getInstance();
            if (auth.getCurrentUser() != null) {
                Crashlytics.setUserIdentifier( auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
                mFirebaseAnalytics.setUserId(auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
                mFirebaseAnalytics.setUserProperty ("name", auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
            }
            return;
        }

        final View view = target_.getActivity().getLayoutInflater().inflate(R.layout.signin, null);

        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(new ContextThemeWrapper(target_.getActivity(), R.style.Theme_AppCompat));
            builder.setTitle(R.string.do_you_want_to_sign_in)
                    .setCancelable(false)
                    .setView(view)
                    .setPositiveButton(target_.getResources().getString(R.string.accept), new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int id) {
                            CheckBox cb = (CheckBox)view.findViewById(R.id.checkBox_never_ask);
                            if( cb != null  ) {
                                SharedPreferences sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(target_.getActivity());
                                SharedPreferences.Editor editor = sharedPrefwrite.edit();
                                editor.putBoolean("pref_dont_ask_signin", cb.isChecked() );
                                editor.apply();
                            }
                            dialog.dismiss();

                            target_.startActivityForResult(
                                    AuthUI.getInstance()
                                            .createSignInIntentBuilder()
                                            .setTheme(R.style.Theme_AppCompat)
                                            .setPrivacyPolicyUrl("http://www.uoyabause.org/static_pages/privacy_policy")
                                            .setAvailableProviders(Arrays.asList(new AuthUI.IdpConfig.GoogleBuilder().build()))
                                            .build(),
                                    RC_SIGN_IN);
                        }
                    })
                    .setNegativeButton(target_.getResources().getString(R.string.decline), new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            CheckBox cb = (CheckBox)view.findViewById(R.id.checkBox_never_ask);
                            if( cb != null  ) {
                                SharedPreferences sharedPrefwrite = PreferenceManager.getDefaultSharedPreferences(target_.getActivity());
                                SharedPreferences.Editor editor = sharedPrefwrite.edit();
                                editor.putBoolean("pref_dont_ask_signin", cb.isChecked() );
                                editor.apply();
                            }
                            dialog.cancel();
                        }
                    });

            builder.create().show();
        }else{
            Crashlytics.setUserIdentifier( auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
            mFirebaseAnalytics.setUserId(auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
            mFirebaseAnalytics.setUserProperty ("name", auth.getCurrentUser().getDisplayName() + "_" + auth.getCurrentUser().getEmail());
        }
    }

    public void fileSelected(File file) {
        String apath;
        if( file == null ){ // canceled
            return;
        }

        Activity context = target_.getActivity();
        if( context == null ){
            return;
        }

        apath = file.getAbsolutePath();

        YabauseStorage storage = YabauseStorage.getStorage();

        // save last selected dir
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("pref_last_dir", file.getParent());
        editor.apply();

        GameInfo gameinfo = GameInfo.getFromFileName(apath);
        if( gameinfo == null ) {
            if (apath.endsWith("CUE") || apath.endsWith("cue")) {
                gameinfo = GameInfo.genGameInfoFromCUE(apath);
            } else if (apath.endsWith("MDS") || apath.endsWith("mds")) {
                gameinfo = GameInfo.genGameInfoFromMDS(apath);
            } else if (apath.endsWith("CCD") || apath.endsWith("ccd")) {
                gameinfo = GameInfo.genGameInfoFromMDS(apath);
            } else {
                gameinfo = GameInfo.genGameInfoFromIso(apath);
            }
        }
        if( gameinfo != null ) {
            gameinfo.updateState();
            Calendar c = Calendar.getInstance();
            gameinfo.lastplay_date = c.getTime();
            gameinfo.save();
        }else{
            return;
        }

        /*
        Bundle bundle = new Bundle();
        bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "PLAY");
        bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, gameinfo.game_title);
        mFirebaseAnalytics.logEvent(
                FirebaseAnalytics.Event.SELECT_CONTENT, bundle
        );
        */

        Intent intent = new Intent(context, Yabause.class);
        intent.putExtra("org.uoyabause.android.FileNameEx", apath);
        context.startActivity(intent);
    }

}
