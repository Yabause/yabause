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
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.RingtoneManager;
import android.net.Uri;
import androidx.core.app.NotificationCompat;
import android.util.Log;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import org.uoyabause.android.tv.GameSelectActivity;
import org.uoyabause.uranus.R;

import java.util.Map;

/**
 * Created by devMiyax on 2016/05/28.
 */
public class Notification extends FirebaseMessagingService{

    private static final String TAG = "uoyabause.Notification";

    @Override
    public void onNewToken(String token) {

    }

    public void showVersionUpNOtification(RemoteMessage remoteMessage){

        //https://play.google.com/store/apps/details?id=org.uoyabause.android

        Map<String,String> val = remoteMessage.getData();

        Intent googlePlayIntent = new Intent(Intent.ACTION_VIEW);
        googlePlayIntent.setData(Uri.parse("market://details?id=org.uoyabause.uranus"));
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0 , googlePlayIntent,
                PendingIntent.FLAG_ONE_SHOT);

        String message = val.get("message");

        Uri defaultSoundUri= RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
                        .setSmallIcon(R.drawable.ic_stat_ss_one)
                        .setContentTitle(getString(R.string.new_version_available))
                        .setStyle(new NotificationCompat.BigTextStyle().bigText(message))
                        .setContentText(message)
                        .setSound(defaultSoundUri)
                        .setContentIntent(pendingIntent)
                        .setAutoCancel(false)
                        //.setPriority(android.app.Notification.PRIORITY_MAX)
                        .addAction(android.R.drawable.ic_media_play, "Install", pendingIntent) ;
        android.app.Notification notification =null;
        UiModeManager uiModeManager = (UiModeManager) getSystemService(UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            Resources r = getResources();
            Bitmap image = BitmapFactory.decodeResource(r, R.drawable.banner);
            mBuilder.setCategory(android.app.Notification.CATEGORY_RECOMMENDATION)
                    .setLargeIcon(image)
                    .setLocalOnly(true)
                    .setOngoing(true);
            notification = new NotificationCompat.BigPictureStyle(mBuilder).build();
        } else {
            notification = mBuilder.build();
        }


        NotificationManager notificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        notificationManager.notify(0 /* ID of notification */, notification);
    }

    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        super.onMessageReceived(remoteMessage);

        Log.d(TAG, "From: " + remoteMessage.getFrom());
        //Log.d(TAG, "Notification Message Body: " + remoteMessage.getNotification().getBody());

        Map<String,String> val = remoteMessage.getData();

        PackageManager pm = this.getPackageManager();
        String sentversion = null;
        int CurrentVersion = -1;
        try {
            PackageInfo packageInfo = pm.getPackageInfo(this.getPackageName(), 0);
            CurrentVersion = packageInfo.versionCode;
        }catch( Exception e){

        }

        sentversion = val.get("version");
        // Version up Information
        if( sentversion != null  ){
            showVersionUpNOtification(remoteMessage);
            return;
        }

        Intent intent = new Intent(this, GameSelectActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0 /* Request code */, intent,
                PendingIntent.FLAG_ONE_SHOT);

        Uri defaultSoundUri= RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
                        .setSmallIcon(R.drawable.ic_stat_ss_one)
                        .setContentTitle("uoYabause")
                        .setContentText( remoteMessage.getNotification().getBody() )
                        .setAutoCancel(true)
                        .setSound(defaultSoundUri)
                        .setContentIntent(pendingIntent);
        android.app.Notification notification =null;
        UiModeManager uiModeManager = (UiModeManager) getSystemService(UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            Resources r = getResources();
            Bitmap image = BitmapFactory.decodeResource(r,R.drawable.banner);
            mBuilder.setCategory("recommendation")
                    .setLargeIcon(image)
                    .setLocalOnly(true)
                    .setCategory(android.app.Notification.CATEGORY_RECOMMENDATION)
                    .setOngoing(true);
            notification = new NotificationCompat.BigPictureStyle(mBuilder).build();
        } else {
            notification = mBuilder.build();
        }

        NotificationManager notificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        notificationManager.notify(0 /* ID of notification */, notification);

    }
}
