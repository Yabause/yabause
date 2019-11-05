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
