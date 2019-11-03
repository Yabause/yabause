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

import android.app.UiModeManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.os.Bundle;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.appcompat.app.AppCompatActivity;

import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import org.uoyabause.uranus.R;

import static java.lang.Thread.sleep;


/*
  1. Find Local Address
  2. Try to access 9212  http://????:9212/ping
  3. send start http://????:9212/start
  4. get status until 'READY_TO_DOWNLOAD' http://????:9212/status
  5. Download cue http://????:9212/download?ext=cue
  6. Donwload img http://????:9212/download?ext=img
  When a error is occured, Invite user to Youtube
 */

public class IsoDownload extends AppCompatActivity {

    final String TAG = "IsoDownload";

    String errormsg_;
    String basepath_ = "/mnt/sdcard/yabause/games/";
    TextView message_view_;
    TextView download_status_;
    ProgressBar download_progress_;

    final int STATE_IDLE = 0;
    final int STATE_SERCHING_SERVER = 1;
    final int STATE_REQUEST_GENERATE_ISO = 2;
    final int STATE_DOWNLOADING = 3;
    final int STATE_CHECKING_FILE = 4;
    final int STATE_FINISHED = 5;

    int state_ = STATE_IDLE;


    private Intent mServiceIntent;
    DownloadStateReceiver mDownloadStateReceiver;

    //----------------------------------------------------------------------------------------------
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_iso_download);

        Intent intent = getIntent();
        String sharedText = intent.getStringExtra("save_path");
        if( sharedText!= null ){
            basepath_ = sharedText;
        }

        message_view_ = (TextView)findViewById(R.id.textView_message);
        download_status_ = (TextView)findViewById(R.id.textViewProgress);
        download_progress_ = (ProgressBar)findViewById(R.id.progressBarDownload);

        if( savedInstanceState != null ){
/*
            CheckBox cb = (CheckBox)findViewById(R.id.checkBoxFindServer);
            cb.setChecked( savedInstanceState.getBoolean("checkBoxFindServer") );

            cb = (CheckBox)findViewById(R.id.checkBox_generate_iso);
            cb.setChecked( savedInstanceState.getBoolean("checkBox_generate_iso") );

            cb = (CheckBox)findViewById(R.id.checkBox_downloaded);
            cb.setChecked( savedInstanceState.getBoolean("checkBox_downloaded") );

            cb = (CheckBox)findViewById(R.id.checkBox_checked);
            cb.setChecked( savedInstanceState.getBoolean("checkBox_checked") );
*/
            message_view_.setText(savedInstanceState.getString("message_view_"));
        }


        Button cancel = (Button)findViewById(R.id.button_cancel);
        cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent localIntent = new Intent(Constants.BROADCAST_STOP);
                LocalBroadcastManager.getInstance(IsoDownload.this).sendBroadcast(localIntent);
                IsoDownload.this.finish();
            }
        });

        //thread_ = new DownloadThread();
        //thread_.execute("init");

        mServiceIntent = new Intent(this, MediaDownloadService.class);
        mServiceIntent.putExtra("save_path", basepath_);
        startService(mServiceIntent);

        IntentFilter statusIntentFilter = new IntentFilter(Constants.BROADCAST_ACTION);
        mDownloadStateReceiver = new DownloadStateReceiver();
        LocalBroadcastManager.getInstance(this).registerReceiver(
                mDownloadStateReceiver,
                statusIntentFilter);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
/*
        CheckBox cb = (CheckBox)findViewById(R.id.checkBoxFindServer);
        outState.putBoolean("checkBoxFindServer", cb.isChecked() );

        cb = (CheckBox)findViewById(R.id.checkBox_generate_iso);
        outState.putBoolean("checkBox_generate_iso", cb.isChecked() );

        cb = (CheckBox)findViewById(R.id.checkBox_downloaded);
        outState.putBoolean("checkBox_downloaded", cb.isChecked() );

        cb = (CheckBox)findViewById(R.id.checkBox_checked);
        outState.putBoolean("checkBox_checked", cb.isChecked() );
*/
        outState.putString("message_view_", message_view_.getText().toString());
    }

    //----------------------------------------------------------------------------------------------
    @Override
    public void onDestroy()
    {
       Log.v(TAG, "this is the end...");
       //thread_.cancel(true);
       super.onDestroy();
    }

    private class DownloadStateReceiver extends BroadcastReceiver {
        private DownloadStateReceiver() {
        }
        @Override
        public void onReceive(Context context, Intent intent) {

            int type = intent.getIntExtra(Constants.EXTENDED_DATA_STATUS,-1);
            if( type == Constants.TYPE_UPDATE_STATE ){
                int newstate = intent.getIntExtra(Constants.NEW_STATE,-1);
                if( newstate != -1 ){
                    UpdateState(newstate );

                    if( STATE_FINISHED == state_ ){
                        int result = intent.getIntExtra(Constants.FINISH_STATUS,-1);
                        if( result == -1 ){
                            errormsg_ = intent.getStringExtra(Constants.ERROR_MESSAGE);
                            showAlert();
                        }else {
                            Intent intent_result = IsoDownload.this.getIntent();
                            IsoDownload.this.setResult(result, intent_result);
                            IsoDownload.this.finish();
                        }
                    }

                    return;
                }
            }else if( type == Constants.TYPE_UPDATE_MESSAGE ) {

                String message = intent.getStringExtra(Constants.MESSAGE);
                if( message != null ){
                    UpdateMessage( message );
                }

            }else if( type == Constants.TYPE_UPDATE_DOWNLOAD ) {
                long current = intent.getLongExtra(Constants.CURRENT,-1);
                long max = intent.getLongExtra(Constants.MAX,-1);
                double bps = intent.getDoubleExtra(Constants.BPS,-1.0);
                if( current != -1 && max != -1 && bps != -1.0 ){
                    UpdateDownloadState(current,max,bps);
                }
            }
        }
    }


    //----------------------------------------------------------------------------------------------
    void UpdateDownloadState( final long current,  final long max, final double mbps  ){
        runOnUiThread(new Runnable() {
            long current_ = current;
            long max_ = max;
            @Override
            public void run() {
                final long base = 1024*1024;
                final String bps_str = String.format("%1$.3f Mbps", mbps);
                String textStatus = (current_/base) + "MByte /" + (max_/base) +"MByte " + bps_str;
                download_status_.setText(textStatus);
                double progress = (double)(current_)/(double)(max_) * 100.0;
                download_progress_.setProgress((int)progress);
            }
        });
    }
    //----------------------------------------------------------------------------------------------
    void UpdateState( int newstate ){

/*
        switch(state_){
            case STATE_SERCHING_SERVER:
                if( newstate == STATE_REQUEST_GENERATE_ISO ){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            CheckBox cb = (CheckBox)findViewById(R.id.checkBoxFindServer);
                            cb.setChecked(true);
                        }
                    });
                }
                break;
            case STATE_REQUEST_GENERATE_ISO:
                if( newstate == STATE_DOWNLOADING ){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            CheckBox cb = (CheckBox)findViewById(R.id.checkBox_generate_iso);
                            cb.setChecked(true);
                        }
                    });
                }
                break;
            case STATE_DOWNLOADING:
                if( newstate == STATE_CHECKING_FILE ){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            CheckBox cb = (CheckBox)findViewById(R.id.checkBox_downloaded);
                            cb.setChecked(true);
                        }
                    });
                }
                break;
            case STATE_CHECKING_FILE:
                if( newstate == STATE_FINISHED ){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            CheckBox cb = (CheckBox)findViewById(R.id.checkBox_checked);
                            cb.setChecked(true);
                        }
                    });
                }
                break;

        }
*/
        state_ = newstate;
    }

    //----------------------------------------------------------------------------------------------
    void UpdateMessage( final String message ){
       runOnUiThread(new Runnable() {
           private String message_ = message;
           @Override
           public void run() {
               message_view_.setText(message_);
           }
       });
    }

    //----------------------------------------------------------------------------------------------
    void showAlert( ){

        // Just for Android TV
        UiModeManager uiModeManager = (UiModeManager) getApplicationContext().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            Toast.makeText(this, "Download failed \n" + errormsg_, Toast.LENGTH_LONG).show();
        }

        if( this.isDestroyed() ) return;
        if( this.isFinishing() ) return;
        Intent intent = IsoDownload.this.getIntent();
        IsoDownload.this.setResult(-1, intent);
        IsoDownload.this.finish();

/*
        if( this.isDestroyed() ) return;
        if( this.isFinishing() ) return;

        new AlertDialog.Builder(this)
                .setTitle("Download Failed")
                .setMessage( errormsg_ + "\nDo you want to watch the tutorial?")
                .setPositiveButton("YES", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String videoId = "HqC9nUJV9zM";
                        Intent intent = new Intent(Intent.ACTION_VIEW);
                        intent.setData(Uri.parse("vnd.youtube:" + videoId));
                        intent.putExtra("VIDEO_ID", videoId);
                        startActivity(intent);
                    }
                })
                .setNegativeButton("NO", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Intent intent = IsoDownload.this.getIntent();
                        IsoDownload.this.setResult(-1, intent);
                        IsoDownload.this.finish();
                    }
                })
                .setOnKeyListener(new Dialog.OnKeyListener() {

                    @Override
                    public boolean onKey(DialogInterface arg0, int keyCode,
                                         KeyEvent event) {
                        // TODO Auto-generated method stub
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            Intent intent = IsoDownload.this.getIntent();
                            IsoDownload.this.setResult(-1, intent);
                            IsoDownload.this.finish();
                        }
                        return true;
                    }
                })
                .show();
*/

    }

}
