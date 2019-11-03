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

import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.VectorDrawable;
import android.media.tv.TvContract;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import androidx.tvprovider.media.tv.Channel;
import androidx.tvprovider.media.tv.ChannelLogoUtils;
import androidx.tvprovider.media.tv.PreviewProgram;
import androidx.tvprovider.media.tv.TvContractCompat;
import android.util.Log;

import com.activeandroid.query.Select;

import org.uoyabause.android.GameInfo;

import java.net.URLEncoder;
import java.util.List;

public class TvUtil {
    private static final String TAG = "TvUtil";
    private static final long CHANNEL_JOB_ID_OFFSET = 1000;

    private static final String[] CHANNELS_PROJECTION = {
            TvContractCompat.Channels._ID,
            TvContract.Channels.COLUMN_DISPLAY_NAME,
            TvContractCompat.Channels.COLUMN_BROWSABLE
    };
    /**
     * Converts a {@link Subscription} into a {@link Channel} and adds it to the tv provider.
     *
     * @param context used for accessing a content resolver.
     * @param subscription to be converted to a channel and added to the tv provider.
     * @return the id of the channel that the tv provider returns.
     */
    @WorkerThread
    public static long createChannel(Context context, Subscription subscription) {

        // Checks if our subscription has been added to the channels before.
        Cursor cursor =
                context.getContentResolver()
                        .query(
                                TvContractCompat.Channels.CONTENT_URI,
                                CHANNELS_PROJECTION,
                                null,
                                null,
                                null);
        if (cursor != null && cursor.moveToFirst()) {
            do {
                Channel channel = Channel.fromCursor(cursor);
                if (subscription.getName().equals(channel.getDisplayName())) {
                    Log.d(
                            TAG,
                            "Channel already exists. Returning channel "
                                    + channel.getId()
                                    + " from TV Provider.");
                    return channel.getId();
                }
            } while (cursor.moveToNext());
        }

        // Create the channel since it has not been added to the TV Provider.
        Uri appLinkIntentUri = Uri.parse(subscription.getAppLinkIntentUri());

        Channel.Builder builder = new Channel.Builder();
        builder.setType(TvContractCompat.Channels.TYPE_PREVIEW)
                .setDisplayName(subscription.getName())
                .setDescription(subscription.getDescription())
                .setAppLinkIntentUri(appLinkIntentUri);

        Log.d(TAG, "Creating channel: " + subscription.getName());
        Uri channelUrl =
                context.getContentResolver()
                        .insert(
                                TvContractCompat.Channels.CONTENT_URI,
                                builder.build().toContentValues());

        Log.d(TAG, "channel insert at " + channelUrl);
        long channelId = ContentUris.parseId(channelUrl);
        Log.d(TAG, "channel id " + channelId);

        Bitmap bitmap = convertToBitmap(context, subscription.getChannelLogo());
        ChannelLogoUtils.storeChannelLogo(context, channelId, bitmap);
        TvContractCompat.requestChannelBrowsable(context, channelId);
        return channelId;
    }

    public static Bitmap convertToBitmap(Context context, int resourceId) {
        Drawable drawable = context.getDrawable(resourceId);
        if (drawable instanceof VectorDrawable) {
            Bitmap bitmap =
                    Bitmap.createBitmap(
                            drawable.getIntrinsicWidth(),
                            drawable.getIntrinsicHeight(),
                            Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
            drawable.draw(canvas);
            return bitmap;
        }

        return BitmapFactory.decodeResource(context.getResources(), resourceId);
    }

    public static void syncPrograms(Context context, long channelId ) {
        Log.d(TAG, "Sync programs for channel: " + channelId);

        try (Cursor cursor =
                     context.getContentResolver()
                             .query(
                                     TvContractCompat.buildChannelUri(channelId),
                                     null,
                                     null,
                                     null,
                                     null)) {


            if (cursor != null && cursor.moveToNext()) {
                Channel channel = Channel.fromCursor(cursor);
                if (!channel.isBrowsable()) {
                    Log.d(TAG, "Channel is not browsable: " + channelId);
                    //deletePrograms(channelId, movies);
                    deletePrograms(context ,channelId);
                } else {
                    Log.d(TAG, "Channel is browsable: " + channelId);
                    deletePrograms(context ,channelId);
                    createPrograms( context, channelId );
                    //if (movies.isEmpty()) {
                    //   movies = createPrograms(channelId, MockMovieService.getList());
                    //} else {
                    //    movies = updatePrograms(channelId, movies);
                    //}
                    //MockDatabase.saveMovies(context, channelId, movies);
                }
            }
        }
    }

    public static List<GameInfo> createPrograms( Context context, long channelId ) {
        List<GameInfo> recentGameList = null;
        try {
            recentGameList  = new Select()
                    .from(GameInfo.class)
                    .orderBy("lastplay_date DESC")
                    .limit(5)
                    .execute();
        }catch(Exception e ){
            System.out.println(e);
        }

        for (GameInfo game : recentGameList) {
            PreviewProgram previewProgram = buildProgram(channelId, game);

            Uri programUri =
                    context.getContentResolver()
                            .insert(
                                    TvContractCompat.PreviewPrograms.CONTENT_URI,
                                    previewProgram.toContentValues());
            long programId = ContentUris.parseId(programUri);
            Log.d(TAG, "Inserted new program: " + programId);
            //game.setProgramId(programId); ToDo
            //moviesAdded.add(movie);
        }
        return recentGameList;
    }

    public static void deletePrograms(Context context ,long channelId) {

        context.getContentResolver()
                .delete(
                        TvContractCompat.PreviewPrograms.CONTENT_URI,
                        null,
                        null);

    }

    private static final String SCHEMA_URI_PREFIX = "saturngame://yabasanshiro/";
    public static final String PLAY = "play";
    private static final String URI_PLAY = SCHEMA_URI_PREFIX + PLAY;

    @NonNull
    public static PreviewProgram buildProgram(long channelId, GameInfo game) {
        Uri posterArtUri = Uri.parse(game.image_url);
        //Uri appLinkUri = AppLinkHelper.buildPlaybackUri(channelId, movie.getId());
        //Uri previewVideoUri = Uri.parse(movie.getVideoUrl());

        String encodedResult;

        try { encodedResult = URLEncoder.encode(game.file_path,"UTF-8"); }
        catch( Exception e ){ return null; }

        Uri appLinkUri = Uri.parse(URI_PLAY).buildUpon().appendPath(encodedResult).build();

        PreviewProgram.Builder builder = new PreviewProgram.Builder();
        builder.setChannelId(channelId)
                .setType(TvContractCompat.PreviewProgramColumns.TYPE_CLIP)
                .setTitle(game.game_title)
                .setDescription(game.maker_id)
                .setIntentUri(appLinkUri)
                .setPosterArtUri(posterArtUri);
                //.setPreviewVideoUri(previewVideoUri)

        return builder.build();
    }

}
