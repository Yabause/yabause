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

import android.app.Notification
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage
import android.content.Intent
import android.app.PendingIntent
import android.media.RingtoneManager
import org.devmiyax.yabasanshiro.R
import android.app.UiModeManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.app.NotificationManager
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.net.Uri
import android.util.Log
import androidx.core.app.NotificationCompat
import org.uoyabause.android.tv.GameSelectActivity
import java.lang.Exception

/**
 * Created by devMiyax on 2016/05/28.
 */
class Notification : FirebaseMessagingService() {
    override fun onNewToken(token: String) {}
    fun showVersionUpNOtification(remoteMessage: RemoteMessage) {

        //https://play.google.com/store/apps/details?id=org.uoyabause.android
        val `val` = remoteMessage.data
        val googlePlayIntent = Intent(Intent.ACTION_VIEW)
        googlePlayIntent.data = Uri.parse("market://details?id=org.uoyabause.uranus")
        val pendingIntent = PendingIntent.getActivity(this, 0, googlePlayIntent,
            PendingIntent.FLAG_ONE_SHOT)
        val message = `val`["message"]
        val defaultSoundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)
        val mBuilder = NotificationCompat.Builder(this)
            .setSmallIcon(R.drawable.ic_stat_ss_one)
            .setContentTitle(getString(R.string.new_version_available))
            .setStyle(NotificationCompat.BigTextStyle().bigText(message))
            .setContentText(message)
            .setSound(defaultSoundUri)
            .setContentIntent(pendingIntent)
            .setAutoCancel(false) //.setPriority(android.app.Notification.PRIORITY_MAX)
            .addAction(android.R.drawable.ic_media_play, "Install", pendingIntent)
        var notification: Notification? = null
        val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
        notification =
            if (uiModeManager.currentModeType == Configuration.UI_MODE_TYPE_TELEVISION) {
                val r = resources
                val image = BitmapFactory.decodeResource(r, R.drawable.banner)
                mBuilder.setCategory(Notification.CATEGORY_RECOMMENDATION)
                    .setLargeIcon(image)
                    .setLocalOnly(true)
                    .setOngoing(true)
                NotificationCompat.BigPictureStyle(mBuilder).build()
            } else {
                mBuilder.build()
            }
        val notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(0 /* ID of notification */, notification)
    }

    override fun onMessageReceived(remoteMessage: RemoteMessage) {
        super.onMessageReceived(remoteMessage)
        Log.d(TAG, "From: " + remoteMessage.from)
        //Log.d(TAG, "Notification Message Body: " + remoteMessage.getNotification().getBody());
        val `val` = remoteMessage.data
        val pm = this.packageManager
        var sentversion: String? = null
        var CurrentVersion = -1
        try {
            val packageInfo = pm.getPackageInfo(this.packageName, 0)
            CurrentVersion = packageInfo.versionCode
        } catch (e: Exception) {
        }
        sentversion = `val`["version"]
        // Version up Information
        if (sentversion != null) {
            showVersionUpNOtification(remoteMessage)
            return
        }
        val intent = Intent(this, GameSelectActivity::class.java)
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
        val pendingIntent = PendingIntent.getActivity(this, 0 /* Request code */, intent,
            PendingIntent.FLAG_ONE_SHOT)
        val defaultSoundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)
        val mBuilder = NotificationCompat.Builder(this)
            .setSmallIcon(R.drawable.ic_stat_ss_one)
            .setContentTitle("uoYabause")
            .setContentText(remoteMessage.notification!!.body)
            .setAutoCancel(true)
            .setSound(defaultSoundUri)
            .setContentIntent(pendingIntent)
        var notification: Notification? = null
        val uiModeManager = getSystemService(UI_MODE_SERVICE) as UiModeManager
        notification = if (uiModeManager.currentModeType == Configuration.UI_MODE_TYPE_TELEVISION) {
            val r = resources
            val image = BitmapFactory.decodeResource(r, R.drawable.banner)
            mBuilder.setCategory("recommendation")
                .setLargeIcon(image)
                .setLocalOnly(true)
                .setCategory(Notification.CATEGORY_RECOMMENDATION)
                .setOngoing(true)
            NotificationCompat.BigPictureStyle(mBuilder).build()
        } else {
            mBuilder.build()
        }
        val notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(0 /* ID of notification */, notification)
    }

    companion object {
        private const val TAG = "uoyabause.Notification"
    }
}