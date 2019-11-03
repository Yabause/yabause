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

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import com.google.android.material.snackbar.Snackbar;
import androidx.fragment.app.Fragment;
import android.util.Log;

import com.firebase.ui.auth.ErrorCodes;
import com.firebase.ui.auth.IdpResponse;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;

import org.uoyabause.uranus.R;

/**
 * Created by i77553 on 2017/10/27.
 */

public class AuthFragment extends Fragment implements FirebaseAuth.AuthStateListener {

    private static final int RC_SIGN_IN = 123;

    private void showSnackbar( int id ) {
        //Toast.makeText(getActivity(), getString(id), Toast.LENGTH_SHORT).show();
        Snackbar
                .make(this.getView(), getString(id), Snackbar.LENGTH_SHORT)
                .show();
    }

    protected void OnAuthAccepted(){

    }

    @Override
    public void onAuthStateChanged(@NonNull FirebaseAuth firebaseAuth) {
        FirebaseUser nuser = firebaseAuth.getCurrentUser();

        if (nuser != null) {
            DatabaseReference baseref = FirebaseDatabase.getInstance().getReference();
            String baseurl = "/user-posts/" + nuser.getUid();
            if (nuser.getDisplayName() != null)
                baseref.child(baseurl).child("name").setValue(nuser.getDisplayName());
            if (nuser.getEmail() != null)
                baseref.child(baseurl).child("email").setValue(nuser.getEmail());
            OnAuthAccepted();

            Log.i("login", nuser.getDisplayName());
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        //Bundle bundle = data.getExtras();
        switch (requestCode) {
            case RC_SIGN_IN: {

                IdpResponse response = IdpResponse.fromResultIntent(data);

                // Successfully signed in
                if (resultCode == Activity.RESULT_OK ) {
                    response.getIdpToken();
                    String token = response.getIdpToken();
                    //FirebaseAuth auth = FirebaseAuth.getInstance();
                    //if( auth.getCurrentUser() == null ){
                    //    Log.d("AuthAragment","auth.getCurrentUser() == null");
                    //}
                    return;
                } else {
                    // Sign in failed
                    if (response == null) {
                        // User pressed back button
                        showSnackbar(R.string.sign_in_cancelled);
                        return;
                    }

                    if (response.getError().getErrorCode() == ErrorCodes.NO_NETWORK) {
                        showSnackbar(R.string.no_internet_connection);
                        return;
                    }

                    if (response.getError().getErrorCode() == ErrorCodes.UNKNOWN_ERROR) {
                        showSnackbar(R.string.unknown_error);
                        return;
                    }
                }
                showSnackbar(R.string.unknown_sign_in_response);

            }
            break;
        }
    }

    protected FirebaseAuth  checkAuth(){
        FirebaseAuth auth = FirebaseAuth.getInstance();
        return auth;
/*
        if (auth.getCurrentUser() == null) {
            auth.addAuthStateListener(this);

            startActivityForResult(
                    AuthUI.getInstance()
                            .createSignInIntentBuilder()
                            .setPrivacyPolicyUrl("http://uoyabause.org")
                            .setProviders(
                                    new AuthUI.IdpConfig.Builder(AuthUI.GOOGLE_PROVIDER).build()))
                            .build(),
                    RC_SIGN_IN);

            new android.app.AlertDialog.Builder(getActivity())
                    .setMessage(getString(R.string.needs_login))
                    .setPositiveButton(getString(R.string.accept), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            startActivityForResult(
                                    AuthUI.getInstance()
                                            .createSignInIntentBuilder()
                                            .setProviders( Arrays.asList(/*new AuthUI.IdpConfig.Builder(AuthUI.TWITTER_PROVIDER).build(),
                                            new AuthUI.IdpConfig.Builder(AuthUI.FACEBOOK_PROVIDER).build(),*
                                                    new AuthUI.IdpConfig.Builder(AuthUI.GOOGLE_PROVIDER).build()))
                                            .build(),
                                    RC_SIGN_IN);
                        }
                    })
                    .setNegativeButton(getString(R.string.decline), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            // Do Nothing
                        }
                    })
                    .show();

            return null;
        }
        return auth;
*/
    }


}
