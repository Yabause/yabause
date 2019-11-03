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

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.UiModeManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.StatFs;
import androidx.annotation.Nullable;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.core.app.NotificationCompat;
import android.text.TextUtils;
import android.text.format.Formatter;
import android.util.Log;

import org.json.JSONObject;
import org.uoyabause.android.GameInfo;
import org.uoyabause.uranus.R;
import org.uoyabause.android.Yabause;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Calendar;
import java.util.Enumeration;
import java.util.concurrent.TimeUnit;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okio.BufferedSource;

import static java.lang.Thread.sleep;
//import android.support.app.recommendation.ContentRecommendation;



/*
  1. Find Local Address
  2. Try to access 9212  http://????:9212/ping
  3. send start http://????:9212/start
  4. get status until 'READY_TO_DOWNLOAD' http://????:9212/status
  5. Download cue http://????:9212/download?ext=cue
  6. Donwload img http://????:9212/download?ext=img
  When a error is occured, Invite user to Youtube
 */

public class MediaDownloadService extends IntentService {

    public static final String LOG_TAG = "MediaDownloadService";
    final String TAG = "IsoDownload";
    final int port_ = 9212;

    NotificationCompat.Builder mBuilder;
    NotificationManager notificationManager_;

    String errormsg_ = "";
    String local_ip_ = "";
    String server_ip_ = "";
    String savefile_ = "";
    String md5_;
    OkHttpClient client_ = null;
    String basepath_ = "/mnt/sdcard/yabause/games/";
    StopReceiver receiver_;
    PendingIntent cancelPendingIntent_;

    private boolean stop_=false;

    final int STATE_IDLE = 0;
    final int STATE_SERCHING_SERVER = 1;
    final int STATE_REQUEST_GENERATE_ISO = 2;
    final int STATE_DOWNLOADING = 3;
    final int STATE_CHECKING_FILE = 4;
    final int STATE_FINISHED = 5;
    int state_ = STATE_IDLE;

    int qcounter_ = 0;


    public MediaDownloadService() {
        super("MediaDownloadService");
    }
    @Override
    protected void onHandleIntent(@Nullable Intent intent) {

        // Just once
        if( qcounter_ > 0 ) return;

        // Create Intent filter for Cancel
        IntentFilter filter = new IntentFilter(Constants.BROADCAST_STOP);
        filter.addCategory(Intent.CATEGORY_DEFAULT);
        receiver_ = new StopReceiver();
        LocalBroadcastManager.getInstance(this).registerReceiver(receiver_, filter);

        // Create pending intent for Cancel
        Intent plocalIntent = new Intent(Constants.BROADCAST_STOP);
        cancelPendingIntent_ = PendingIntent.getBroadcast(this, 0, plocalIntent, 0);

        // Create Notification Manager
        Context ctx = getApplicationContext();
        notificationManager_ = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        showInitNotification();


        // Get parametaer
        String sharedText = intent.getStringExtra("save_path");
        if( sharedText!= null ){
            basepath_ = sharedText;
        }

        // Start downloading
        int result = download();

        showFinishNotification(result);

        Intent localIntent =
                new Intent(Constants.BROADCAST_ACTION)
                        // Puts the status into the Intent
                        .putExtra(Constants.EXTENDED_DATA_STATUS, Constants.TYPE_UPDATE_STATE)
                        .putExtra(Constants.FINISH_STATUS, result)
                        .putExtra(Constants.ERROR_MESSAGE, errormsg_)
                        .putExtra(Constants.NEW_STATE, STATE_FINISHED);
        // Broadcasts the Intent to receivers in this app.
        LocalBroadcastManager.getInstance(this).sendBroadcast(localIntent);

        qcounter_++;
    }

    void showInitNotification(){

        mBuilder = new NotificationCompat.Builder(this)
                .setPriority(NotificationCompat.PRIORITY_MAX )
                .setContentTitle(getString(R.string.downloading_cd_rom))
                .addAction(R.drawable.ic_cancel_black_24dp, "Cancel", cancelPendingIntent_)
                .setSmallIcon(R.drawable.ic_icon_saturn_mini);
        Notification notification = mBuilder.build();
        notification.flags |= Notification.FLAG_NO_CLEAR;
        notificationManager_.notify(0, notification);
    }

    void showFinishNotification( int result ){

        mBuilder = new NotificationCompat.Builder(this)
                .setPriority(NotificationCompat.PRIORITY_MAX )
                .setContentTitle(savefile_)
                .setSmallIcon(R.drawable.ic_icon_saturn_mini);

        mBuilder.setProgress(0, 0, false);

        if( result == 0) {
            mBuilder.setContentText(getString(R.string.finished));
/*
            Target: org.uoyabause.android/org.uoyabause.android.Yabause
            Key:    org.uoyabause.android.FileNameEx
            Type:  String
            Value: absolute file path for a game CD-ROM ISO image
*/

            Intent intent = new Intent(getApplicationContext(), Yabause.class);

            String path = basepath_ + savefile_ + ".cue";
            intent.putExtra("org.uoyabause.android.FileNameEx",path);
            //intent.setFlags(
            //        Intent.FLAG_ACTIVITY_CLEAR_TOP
            //        | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED
            //        | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
            //);
            PendingIntent pendingIntent = PendingIntent.getActivity(getApplicationContext(), 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            mBuilder.setContentIntent(pendingIntent);

            path = basepath_ + savefile_ + ".img";
            GameInfo gi = GameInfo.genGameInfoFromIso(path);
            if( gi == null ){
                errormsg_ = "Fail to notify recommendation. ";
                return;
            }
            gi.updateState();
            Calendar c = Calendar.getInstance();
            gi.lastplay_date = c.getTime();
            gi.save();

            Bitmap bitmap = null;
            int cardWidth  = 320;
            int cardHeight = 240;
            if( gi.image_url != null && !gi.image_url.equals("") ) {
/*
                //Resources res = getResources();
                try {
                    bitmap = Glide.with(getApplication())
                            .load(gi.image_url)
                            .asBitmap()
                            .into(cardWidth, cardHeight) // Only use for synchronous .get()
                            .get();
                }catch( Exception e ){
                    Log.e(LOG_TAG, "message", e);
                }
*/
            }

            if( bitmap != null ) {
                NotificationCompat.BigPictureStyle style = new NotificationCompat.BigPictureStyle().bigPicture(bitmap);
                mBuilder.setStyle(style);
            }

            // In the case of Android TV, notify recommendation
            UiModeManager uiModeManager = (UiModeManager) getApplicationContext().getSystemService(Context.UI_MODE_SERVICE);
            if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
                try {


                    mBuilder.setCategory(Notification.CATEGORY_RECOMMENDATION)
                            .setSmallIcon(R.drawable.ic_stat_ss_one)
                            .setContentTitle(gi.game_title)
                            .setLocalOnly(true)
                            .setOngoing(true)
                            .setColor(getApplicationContext().getResources().getColor(R.color.fastlane_background));

                    if( bitmap != null ) {
                        mBuilder.setLargeIcon(bitmap);
                    }else{
                        bitmap = BitmapFactory.decodeResource(getApplicationContext().getResources(),R.drawable.missing);
                        NotificationCompat.BigPictureStyle style = new NotificationCompat.BigPictureStyle().bigPicture(bitmap);
                        mBuilder.setStyle(style);
                    }

                    mBuilder.setAutoCancel(true);

                    Notification notification = mBuilder.build();
                    notificationManager_.notify(gi.game_title.hashCode(), notification);
/*
                    ContentRecommendation.Builder rbuilder = new ContentRecommendation.Builder()
                            .setBadgeIcon(R.drawable.icon);

                    rbuilder.setIdTag("uoyabause0")
                            .setTitle(gi.game_title)
                            //.setDescription(getString(R.string.popular_header))
                            .setContentIntentData(
                                    ContentRecommendation.INTENT_TYPE_ACTIVITY,
                                    intent,0,null);

                    if( gi.image_url != null && !gi.image_url.equals("") ) {

                        Resources res = getResources();
                        int cardWidth  = 320;
                        int cardHeight = 240;

                        Bitmap bitmap = Glide.with(getApplication())
                                .load(gi.image_url)
                                .asBitmap()
                                .into(cardWidth, cardHeight) // Only use for synchronous .get()
                                .get();
                        rbuilder.setContentImage(bitmap);
                    }

                    ContentRecommendation rec = rbuilder.build();
                    Notification notification = rec.getNotificationObject(getApplicationContext());
                    notificationManager_.notify(1, notification);
*/
                }catch( Exception e ){
                    errormsg_ = "Fail to notify recommendation. " + e.getLocalizedMessage();
                    Log.e(LOG_TAG, "message", e);
                }

            }else {

                mBuilder.setDefaults(Notification.DEFAULT_SOUND | Notification.DEFAULT_VIBRATE);
                Notification notification = mBuilder.build();
                notification.flags |= Notification.FLAG_AUTO_CANCEL;
                notificationManager_.notify(0, notification);
            }

        }else if( result == -1 ){
            mBuilder.setContentText(getString(R.string.failed));

            NotificationCompat.BigTextStyle  bstyle = new NotificationCompat.BigTextStyle();
            bstyle.bigText(errormsg_);
            mBuilder.setStyle(bstyle);
            mBuilder.setDefaults(Notification.DEFAULT_SOUND| Notification.DEFAULT_VIBRATE);
            Notification notification = mBuilder.build();
            notification.flags = Notification.FLAG_AUTO_CANCEL;
            notificationManager_.notify(0, notification);

        }else if( result == -2 ) {
            mBuilder.setContentText(getString(R.string.canceled));
            mBuilder.setDefaults(Notification.DEFAULT_SOUND| Notification.DEFAULT_VIBRATE);
            Notification notification = mBuilder.build();
            notification.flags = Notification.FLAG_AUTO_CANCEL;
            notificationManager_.notify(0, notification);
        }


    }

    void showDownloadingNotification( final long current,  final long max, final double mbps ) {
        // Update Progress bar
        final long base = 1024*1024;
        final String bps_str = String.format("%1$.1f Mbps", mbps);
        String textStatus = (current/base) + "MByte /" + (max/base) +"MByte " + bps_str;

        mBuilder = new NotificationCompat.Builder(this)
                .setPriority(NotificationCompat.PRIORITY_MAX )
                .setContentTitle("Downloading "+ savefile_)
                .addAction(R.drawable.ic_cancel_black_24dp, getString(R.string.cancel), cancelPendingIntent_)
                .setSmallIcon(R.drawable.ic_icon_saturn_mini);

        NotificationCompat.BigTextStyle  bstyle = new NotificationCompat.BigTextStyle();
        bstyle.bigText(textStatus);
        mBuilder.setStyle(bstyle);

        double progress = (double)(current)/(double)(max) * 100.0;
        mBuilder.setProgress(100, (int)progress, false);

        Notification notification = mBuilder.build();
        notification.flags |= Notification.FLAG_NO_CLEAR;
        notificationManager_.notify(0, notification);
    }

    void showMessageNotification( String message ) {

        mBuilder = new NotificationCompat.Builder(this)
                .setPriority(NotificationCompat.PRIORITY_MAX )
                .setContentTitle("Downloading " + savefile_)
                .addAction(R.drawable.ic_cancel_black_24dp, getString(R.string.cancel), cancelPendingIntent_)
                .setSmallIcon(R.drawable.ic_icon_saturn_mini);

        NotificationCompat.BigTextStyle  bstyle = new NotificationCompat.BigTextStyle();
        bstyle.bigText(message);
        mBuilder.setStyle(bstyle);
        Notification notification = mBuilder.build();
        notification.flags |= Notification.FLAG_NO_CLEAR;
        notificationManager_.notify(0, notification);
    }

    void UpdateState( int newstate ) {
        state_ = newstate;

        Intent localIntent =
                new Intent(Constants.BROADCAST_ACTION)
                        // Puts the status into the Intent
                        .putExtra(Constants.EXTENDED_DATA_STATUS, Constants.TYPE_UPDATE_STATE)
                        .putExtra(Constants.NEW_STATE, newstate);
        // Broadcasts the Intent to receivers in this app.
        LocalBroadcastManager.getInstance(this).sendBroadcast(localIntent);
    }

    void UpdateDownloadState( final long current,  final long max, final double mbps  ){

        showDownloadingNotification(current,max,mbps);

        Intent localIntent =
                new Intent(Constants.BROADCAST_ACTION)
                        // Puts the status into the Intent
                        .putExtra(Constants.EXTENDED_DATA_STATUS, Constants.TYPE_UPDATE_DOWNLOAD)
                        .putExtra(Constants.CURRENT, current)
                        .putExtra(Constants.MAX, max)
                        .putExtra(Constants.BPS, mbps);

        // Broadcasts the Intent to receivers in this app.
        LocalBroadcastManager.getInstance(this).sendBroadcast(localIntent);
    }

    void UpdateMessage( final String message ){

        showMessageNotification(message);

        Intent localIntent =
                new Intent(Constants.BROADCAST_ACTION)
                        // Puts the status into the Intent
                        .putExtra(Constants.EXTENDED_DATA_STATUS, Constants.TYPE_UPDATE_MESSAGE)
                        .putExtra(Constants.MESSAGE, message);

        // Broadcasts the Intent to receivers in this app.
        LocalBroadcastManager.getInstance(this).sendBroadcast(localIntent);
    }

    int cleanup_cancel(){
        String path = basepath_ + savefile_ + ".cue";
        File fp = new File(path);
        if( fp != null ) fp.delete();

        path = basepath_ + savefile_ + ".img";
        fp = new File(path);
        if( fp != null ) fp.delete();
        return 0;
    }

    int download() {

        int rtn = 0;

        UpdateState( STATE_SERCHING_SERVER );
        UpdateMessage(getString(R.string.serching_for_a_server));

        if( GenerateLocalNetworkInfo() != 0 ) {
            return -1;
        }

        if( stop_ ){ return -2; }

        if( FindServer() != 0 ){
            if( stop_ ){ return -2; }
            return -1;
        }

        if( stop_ ){ return -2; }

        UpdateState(STATE_REQUEST_GENERATE_ISO);
        UpdateMessage(getString(R.string.waiting_for_reading));

        if( (rtn = startRead(server_ip_)) != 0) {
            return rtn;
        }

        UpdateState(STATE_DOWNLOADING);
        UpdateMessage(getString(R.string.downloading));

        if(  (rtn = download(  server_ip_, "cue", false )) != 0 ){
            cleanup_cancel();
            return rtn;
        }

        if(  (rtn=download(  server_ip_, "img", false )) != 0 ){
            cleanup_cancel();
            return rtn;
        }

        UpdateState(STATE_CHECKING_FILE);
        UpdateMessage(getString(R.string.checking_file));

        String path = basepath_ + savefile_ + ".img";
        if (checkMD5(md5_, new File(path)) == false) {
            errormsg_ = getString(R.string.download_file_is_broken);
            return -1;
        }

        cleanup( server_ip_ );


        return rtn;
    }

    public String getLocalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress()) {
                        String ip = Formatter.formatIpAddress(inetAddress.hashCode());
                        Log.i(TAG, "***** IP="+ ip);
                        return ip;
                    }
                }
            }
        } catch (SocketException ex) {
            Log.e(TAG, ex.toString());
        }
        return null;
    }

    //----------------------------------------------------------------------------------------------
    int GenerateLocalNetworkInfo() {

        try {
            WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
            WifiInfo w_info = wifiManager.getConnectionInfo();
            Log.i(TAG, "SSID:" + w_info.getSSID());
            Log.i(TAG, "BSSID:" + w_info.getBSSID());
            Log.i(TAG, "IP Address:" + w_info.getIpAddress());
            Log.i(TAG, "Mac Address:" + w_info.getMacAddress());
            Log.i(TAG, "Network ID:" + w_info.getNetworkId());
            Log.i(TAG, "Link Speed:" + w_info.getLinkSpeed());
            int ip_addr_i = w_info.getIpAddress();
            local_ip_ = ((ip_addr_i >> 0) & 0xFF) + "." + ((ip_addr_i >> 8) & 0xFF) + "." + ((ip_addr_i >> 16) & 0xFF) + "." + ((ip_addr_i >> 24) & 0xFF);

            // No Wifi Connection is available
            if(ip_addr_i==0){

                // no need wifi connection for android TV
                UiModeManager uiModeManager = (UiModeManager) getApplicationContext().getSystemService(Context.UI_MODE_SERVICE);
                if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION) {
                    errormsg_ = getString(R.string.you_need_wifi_connection);
                    return -1;
                }else{
                    local_ip_ = getLocalIpAddress();
                }
            }

        }catch( Exception e ){
            Log.e(LOG_TAG, "message", e);
            return -1;
        }
        return 0;
    }

    int FindServer(  )
    {
        client_ = new OkHttpClient.Builder()
                .connectTimeout(100, TimeUnit.MILLISECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .build();

        try {
            String sip = local_ip_.substring(0, local_ip_.indexOf('.', local_ip_.indexOf('.', local_ip_.indexOf('.') + 1) + 1) + 1);
            try {
                for (int it = 1; it <= 255; it++) {

                    if( stop_ ){ return -2; }

                    String checksip = sip + it;

                    UpdateMessage(getString(R.string.serching_for_a_server) + checksip );

                    if( checksip.equals(local_ip_)){
                        continue;
                    }
                    Log.i(TAG, "isReachable:" + checksip);
                    //if (InetAddress.getByName(checksip).isReachable(100)) {
                    if( checkConnection(checksip) == 0 ){
                        server_ip_ = checksip;
                        return 0;
                    }
                    //}
                }
            } catch (Exception e1) {
                errormsg_ = getString(R.string.network_error) + e1.getLocalizedMessage();
                Log.e(LOG_TAG, "message", e1);
                return -1;
            }
        }catch( Exception e){
            errormsg_ =  getString(R.string.network_error) + e.getLocalizedMessage();
            Log.e(LOG_TAG, "message", e);
            return -1;
        }

        errormsg_ = getString(R.string.server_is_not_found);
        return -1;
    }

    String run(String url) throws IOException {
        Request request = new Request.Builder()
                .url(url)
                .build();
        Response response = client_.newCall(request).execute();
        return response.body().string();
    }


    int checkConnection( String sip ){

        Log.i(TAG, "checkConnection:" + sip);

        String url = "http://" + sip +":" + port_ + "/ping";

        try {
            String result = run(url);
            if( result.equals("OK") ){
                return 0;
            }
        }catch( Exception e ){
            errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
            Log.e(LOG_TAG, "message", e);
            return -1;
        }
        errormsg_ =getString(R.string.network_error) ;
        return -1;
    }

    //----------------------------------------------------------------------------------------------
    int startRead( String sip ) {

        client_ = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(60*3, TimeUnit.SECONDS)  // needs 3 minutes for generating md5
                .build();

        String url = "http://" + sip +":" + port_ + "/start";
        try {
            String result = run(url);
            if( result.equals("OK") ){

                while(true) {
                    sleep( 5000 );
                    url = "http://" + sip + ":" + port_ + "/status";
                    result = run(url);
                    JSONObject Jobject = new JSONObject(result);
                    if (Jobject.getString("state").equals("READY_TO_DOWNLOAD")) {
                        savefile_ = Jobject.getString("name");
                        md5_ = Jobject.getString("md5");
                        return 0;
                    }
                    if (Jobject.getString("state").equals("ERRORED")) {
                        errormsg_ = getString(R.string.server_error);
                        return -1;
                    }

                    if( stop_ ) return -2;
                }
            }
        }catch( Exception e ){
            errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
            Log.e(LOG_TAG, "message", e);
            return -1;
        }
        errormsg_ = getString(R.string.network_error);
        return -1;
    }

    //----------------------------------------------------------------------------------------------
    boolean checkMD5(String md5, File updateFile) {
        if (TextUtils.isEmpty(md5) || updateFile == null) {
            Log.e(TAG, "MD5 string empty or updateFile null");
            return false;
        }

        String calculatedDigest = calculateMD5(updateFile);
        if (calculatedDigest == null) {
            Log.e(TAG, "calculatedDigest null");
            return false;
        }

        Log.v(TAG, "Calculated digest: " + calculatedDigest);
        Log.v(TAG, "Provided digest: " + md5);

        return calculatedDigest.equalsIgnoreCase(md5);
    }

    //----------------------------------------------------------------------------------------------
    public String calculateMD5(File updateFile) {
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("MD5");
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, "Exception while getting digest", e);
            return null;
        }

        InputStream is;
        try {
            is = new FileInputStream(updateFile);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Exception while getting FileInputStream", e);
            return null;
        }

        byte[] buffer = new byte[8192];
        int read;
        try {
            while ((read = is.read(buffer)) > 0) {
                digest.update(buffer, 0, read);
            }
            byte[] md5sum = digest.digest();
            BigInteger bigInt = new BigInteger(1, md5sum);
            String output = bigInt.toString(16);
            // Fill to 32 chars
            output = String.format("%32s", output).replace(' ', '0');
            return output;
        } catch (IOException e) {
            throw new RuntimeException(getString(R.string.unable_to_process_file_for_md5), e);
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                Log.e(TAG, "Exception on closing MD5 input stream", e);
            }
        }
    }

    final long DOWNLOAD_CHUNK_SIZE_ = 1024*8;

    //----------------------------------------------------------------------------------------------
    int download(  String sip, String ext, boolean isResume ){

        client_ = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .build();

        String url = "http://" + sip +":" + port_ + "/download?ext="+ext;


        try {
            Request request = new Request.Builder()
                    .url(url)
                    .addHeader("Accept-Encoding", "gzip")
                    .build();

            Response response = client_.newCall(request).execute();
            ResponseBody body = response.body();
            long contentLength = body.contentLength();
            BufferedSource source = body.source();

            String path = basepath_ + this.savefile_  + "." + ext;

            // Check free space
            StatFs stat = new StatFs(basepath_);
            long bytesAvailable = stat.getBlockSizeLong() * stat.getAvailableBlocksLong();
            if( contentLength > bytesAvailable ){
                errormsg_ = getString(R.string.no_enough_free_space);
                return -1;
            }

            File file = new File(path);
            BufferedInputStream input = new BufferedInputStream(body.byteStream());
            OutputStream output;

            if (isResume) {
                source.skip(file.length());
                output = new FileOutputStream(file, true);
            } else {
                output = new FileOutputStream(file, false);
            }

            long currentDownloadedSize = 0;
            long preDownloadedSize = 0;
            long currentTotalByteSize = body.contentLength();
            byte[] data = new byte[1024*8];
            int count = 0;

            long pretime = System.currentTimeMillis();
            long timespan = 0;
            while ((count = input.read(data)) != -1) {
                currentDownloadedSize += count;
                output.write(data, 0, count);

                long current_time = System.currentTimeMillis();
                timespan += current_time - pretime;
                pretime = current_time;
                if( timespan > 1000 ) {
                    double mbps = (double)currentDownloadedSize-(double)preDownloadedSize;
                    mbps = mbps / (1024*1024) * 8;
                    timespan -= 1000;
                    UpdateDownloadState(currentDownloadedSize, currentTotalByteSize, mbps);
                    preDownloadedSize =  currentDownloadedSize;
                }

                if( stop_ ){
                    output.close();
                    return -2;
                }
            }
            output.close();

        } catch (IOException e) {
            errormsg_ = getString(R.string.network_error) + e.getLocalizedMessage();
            Log.e(LOG_TAG, "message", e);
            return -1;
        }
        return 0;
    }

    //----------------------------------------------------------------------------------------------
    int cleanup( String sip ){

        client_ = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .writeTimeout(10, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .build();

        String url = "http://" + sip +":" + port_ +"/byebye";
        try {
            Request request = new Request.Builder().url(url).build();
            Response response = client_.newCall(request).execute();
        }catch( Exception e){
            Log.e(LOG_TAG, "message", e);
            return -1;
        }

        return 0;
    }

    public class StopReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            stop_ = true;
        }
    }

}
