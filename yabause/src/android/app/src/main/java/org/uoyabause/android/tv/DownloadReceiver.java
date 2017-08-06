package org.uoyabause.android.tv;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

import org.uoyabause.android.tv.GameSelectFragment;

/**
 * Created by shinya on 2017/04/02.
 */

public class DownloadReceiver extends BroadcastReceiver {
    public static final String DOWNLOAD_COMPLETE_NOTIFICATION = "org.uoyabause.download.COMPLETE";
    public static final String DOWNLOAD_FAIL_NOTIFICATION = "org.uoyabause.download.FAILED";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        switch (action) {
            case DOWNLOAD_COMPLETE_NOTIFICATION:
                Toast.makeText(context, intent.getStringExtra("filename") + " download is completed.", Toast.LENGTH_LONG).show();
                GameSelectFragment.refresh_level = 3;

                if( GameSelectFragment.isForeground != null ){
                    GameSelectFragment.isForeground.updateGameList();
                }

                break;
            case DOWNLOAD_FAIL_NOTIFICATION:
                Toast.makeText(context, intent.getStringExtra("message") + " download is failed.", Toast.LENGTH_LONG).show();
                break;
        }
    }
}
