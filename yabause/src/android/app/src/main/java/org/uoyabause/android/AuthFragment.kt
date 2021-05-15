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

import android.app.Activity
import android.content.Intent
import android.util.Log
import androidx.fragment.app.Fragment
import com.firebase.ui.auth.ErrorCodes
import com.firebase.ui.auth.IdpResponse
import com.google.android.material.snackbar.Snackbar
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseAuth.AuthStateListener
import com.google.firebase.database.FirebaseDatabase
import org.devmiyax.yabasanshiro.R

/**
 * Created by i77553 on 2017/10/27.
 */
open class AuthFragment : Fragment(), AuthStateListener {
    private fun showSnackbar(id: Int) {
        //Toast.makeText(getActivity(), getString(id), Toast.LENGTH_SHORT).show();
        Snackbar
            .make(this.view!!, getString(id), Snackbar.LENGTH_SHORT)
            .show()
    }

    protected open fun OnAuthAccepted() {}
    override fun onAuthStateChanged(firebaseAuth: FirebaseAuth) {
        val nuser = firebaseAuth.currentUser
        if (nuser != null) {
            val baseref = FirebaseDatabase.getInstance().reference
            val baseurl = "/user-posts/" + nuser.uid
            if (nuser.displayName != null) baseref.child(baseurl).child("name")
                .setValue(nuser.displayName)
            if (nuser.email != null) baseref.child(baseurl).child("email").setValue(nuser.email)
            OnAuthAccepted()
            Log.i("login", nuser.displayName!!)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            RC_SIGN_IN -> {
                val response = IdpResponse.fromResultIntent(data)

                // Successfully signed in
                if (resultCode == Activity.RESULT_OK) {
                    response!!.idpToken
                    val token = response.idpToken
                    //FirebaseAuth auth = FirebaseAuth.getInstance();
                    //if( auth.getCurrentUser() == null ){
                    //    Log.d("AuthAragment","auth.getCurrentUser() == null");
                    //}
                    return
                } else {
                    // Sign in failed
                    if (response == null) {
                        // User pressed back button
                        showSnackbar(R.string.sign_in_cancelled)
                        return
                    }
                    if (response.error!!.errorCode == ErrorCodes.NO_NETWORK) {
                        showSnackbar(R.string.no_internet_connection)
                        return
                    }
                    if (response.error!!.errorCode == ErrorCodes.UNKNOWN_ERROR) {
                        showSnackbar(R.string.unknown_error)
                        return
                    }
                }
                showSnackbar(R.string.unknown_sign_in_response)
            }
        }
    }

    protected fun checkAuth(): FirebaseAuth {
        return FirebaseAuth.getInstance()
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
                                            .setProviders( Arrays.asList(/ *new AuthUI.IdpConfig.Builder(AuthUI.TWITTER_PROVIDER).build(),
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

    companion object {
        private const val RC_SIGN_IN = 123
    }
}