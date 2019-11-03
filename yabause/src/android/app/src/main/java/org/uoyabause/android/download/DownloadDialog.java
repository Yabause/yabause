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

package org.uoyabause.android.download;

import android.app.DialogFragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.uoyabause.uranus.R;

public class DownloadDialog extends DialogFragment {

    final String TAG = "DownloadDialog";

    String errormsg_;
    String basepath_ = "/mnt/sdcard/yabause/games/";
    TextView message_view_;

    private Intent mServiceIntent;
    DownloadStateReceiver mDownloadStateReceiver;

    void setBasePath( String path ){
        basepath_ = path;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        int style = DialogFragment.STYLE_NO_TITLE;
        int theme  = android.R.style.Theme_Material_Dialog;
        setStyle(style, theme);

        mServiceIntent = new Intent(getActivity(), MediaDownloadService.class);
        mServiceIntent.putExtra("svae_path", basepath_);
        getActivity().startService(mServiceIntent);

        IntentFilter statusIntentFilter = new IntentFilter(Constants.BROADCAST_ACTION);
        mDownloadStateReceiver = new DownloadDialog.DownloadStateReceiver();
        LocalBroadcastManager.getInstance(getActivity()).registerReceiver(
                mDownloadStateReceiver,
                statusIntentFilter);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.download_dialog, container, false);
        message_view_ = (TextView)v.findViewById(R.id.textView_message);
        return v;
    }

    private class DownloadStateReceiver extends BroadcastReceiver {
        private DownloadStateReceiver() {
        }
        @Override
        public void onReceive(Context context, Intent intent) {

            int type = intent.getIntExtra(Constants.EXTENDED_DATA_STATUS,-1);
            if( type == Constants.TYPE_UPDATE_STATE ){
                int newstate = intent.getIntExtra(Constants.NEW_STATE,-1);

                if( newstate == Constants.STATE_FINISHED ) {

                }

                if( newstate != -1 ){

                    //if(newstate == Constants.STATE_FINISHED) {
                    //    int result = intent.getIntExtra(Constants.FINISH_STATUS, -1);
                    //}

                    return;
                }

            }else if( type == Constants.TYPE_UPDATE_MESSAGE ) {

                String message = intent.getStringExtra(Constants.MESSAGE);
                if( message != null ){
                    UpdateMessage( message );
                }

            }else if( type == Constants.TYPE_UPDATE_DOWNLOAD ) {
                //long current = intent.getLongExtra(Constants.CURRENT,-1);
                //long max = intent.getLongExtra(Constants.MAX,-1);
                //double bps = intent.getDoubleExtra(Constants.BPS,-1.0);
                //if( current != -1 && max != -1 && bps != -1.0 ){
                //    UpdateDownloadState(current,max,bps);
                //}
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    void UpdateMessage( final String message ){
        message_view_.setText(message);
    }

}
