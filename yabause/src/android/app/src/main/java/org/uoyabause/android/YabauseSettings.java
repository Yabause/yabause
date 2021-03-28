/*  Copyright 2013 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
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

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.input.InputManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.StatFs;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;

import androidx.preference.DialogPreference;
import androidx.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.AttributeSet;
import android.util.Log;
import android.content.Intent;
import android.content.SharedPreferences;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.content.res.Resources;
import android.app.DialogFragment;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.app.Dialog;

import org.devmiyax.yabasanshiro.R;
import org.uoyabause.android.tv.GameSelectFragment;


public class YabauseSettings extends PreferenceActivity
        implements SharedPreferences.OnSharedPreferenceChangeListener, Preference.OnPreferenceClickListener, InputManager.InputDeviceListener {

    boolean dirlist_status = false;

    int restart_level = 0;

    public void setDireListChangeStatus( boolean status ) {
        dirlist_status = status;
        if( dirlist_status==true ){
            //Intent resultIntent = new Intent();
            //setResult(  GameSelectFragment.GAMELIST_NEED_TO_UPDATED , resultIntent);
            if(restart_level<=0 ) restart_level = 1;
            updateResultCode();
        }
    }

    int tap_count_ = 0;
    @Override
    public boolean onPreferenceClick(Preference preference) {
        tap_count_++;
        if( tap_count_ > 5 ){
            tap_count_ = 0;
            //Intent intent = new Intent(this, CheckPaymentActivity.class );
            //startActivity(intent);
            //startActivityForResult(intent, CHECK_PAYMENT);
        }
        return false;
    }

    InputManager mInputManager;
    @Override
    public void onInputDeviceAdded(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    @Override
    public void onInputDeviceRemoved(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    @Override
    public void onInputDeviceChanged(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    public static class WarningDialogFragment extends DialogFragment {
    @Override
    public boolean onPreferenceClick(Preference preference) {
        tap_count_++;
        if( tap_count_ > 5 ){
            tap_count_ = 0;
            //Intent intent = new Intent(this, CheckPaymentActivity.class );
            //startActivity(intent);
            //startActivityForResult(intent, CHECK_PAYMENT);
        }
        return false;
    }

    InputManager mInputManager;
    @Override
    public void onInputDeviceAdded(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    @Override
    public void onInputDeviceRemoved(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    @Override
    public void onInputDeviceChanged(int i) {
        PadManager.updatePadManager();
        SyncInputDevice();
        SyncInputDeviceForPlayer2();
    }

    public static class WarningDialogFragment extends DialogFragment {
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            // Use the Builder class for convenient dialog construction
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            Resources res = getResources();
            builder.setMessage(res.getString(R.string.msg_opengl_not_supported))
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        // FIRE ZE MISSILES!
                    }
                });

            // Create the AlertDialog object and return it
            return builder.create();
        }
    }
  }

  void setUpDonateInfo(){
      PreferenceScreen preferenceScreen = this.getPreferenceScreen();
      // create preferences manually
      PreferenceCategory preferenceCategory = new PreferenceCategory(preferenceScreen.getContext());
      preferenceCategory.setTitle("Donation Status");
      //do anything you want with the preferencecategory here
      preferenceScreen.addPreference(preferenceCategory);

      Preference preference = new Preference(preferenceScreen.getContext());
      preference.setTitle("Donated");
      preference.setOnPreferenceClickListener(this);
      SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
      Boolean hasDonated = prefs.getBoolean("donated", false);
      if( hasDonated ) {
          preference.setSummary("Yes");
      }else{
          preference.setSummary("No");
      }
      //do anything you want with the preferencey here
      preferenceCategory.addPreference(preference);

      if( hasDonated ){
          //editor.putString("donate_payload", purchase.getDeveloperPayload());
          //editor.putString( "donate_item", purchase.getSku());

          Boolean uoyabause_donation = prefs.getBoolean("uoyabause_donation", false);
          if(uoyabause_donation){
              String payload = prefs.getString("donate_payload","");
              Preference prefPayload = new Preference(preferenceScreen.getContext());
              prefPayload.setTitle("Payment ID");
              prefPayload.setSummary( "uoYabause" );
              preferenceCategory.addPreference(prefPayload);
              return;
          }

          String payload = prefs.getString("donate_payload","");
          Preference prefPayload = new Preference(preferenceScreen.getContext());
          prefPayload.setTitle("Payment ID");
          try {
              String id = payload.substring(0, 6);
              if (id != null) {
                  prefPayload.setSummary(id);
              } else {
                  prefPayload.setSummary("REDEEM");
              }
          }catch(Exception e){
              prefPayload.setSummary("REDEEM");
          }
          preferenceCategory.addPreference(prefPayload);

          String donate_item = prefs.getString("donate_item","");
          int count = prefs.getInt("donate_activate_count",-1);
          String activate_status = "";
          if (donate_item.equals(DonateActivity.SKU_DONATE_SMALL)) {
              activate_status = count + "/1";
          }
          if (donate_item.equals(DonateActivity.SKU_DONATE_MEDIUM)) {
              activate_status = count + "/5";
          }
          if (donate_item.equals(DonateActivity.SKU_DONATE_LARGE)  ) {
              activate_status = count + "/Infinity";
          }

          Preference pref_activate_status = new Preference(preferenceScreen.getContext());
          pref_activate_status.setTitle("Activate Count");
          pref_activate_status.setSummary( activate_status );
          preferenceCategory.addPreference(pref_activate_status);
      }

  }
  
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mInputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);

        try{
            addPreferencesFromResource(R.xml.preferences);
        }catch( Exception e ){
            e.printStackTrace();
        }

        setUpDonateInfo();



        Preference myCheckbox = findPreference("pref_extend_internal_memory");
        if( myCheckbox != null) {
            myCheckbox.setEnabled(false);
            myCheckbox.setOnPreferenceChangeListener(myCheckboxListener);
        }

        //GameDirectoriesDialogPreference dires = (GameDirectoriesDialogPreference)findPreference("pref_game_directory");

        GameDirectoriesDialogPreference dires = null;
        if(dires != null)
         dires.setActivity(this);
/*
        Preference filePicker = (Preference) findPreference("pref_game_download_directory");
        filePicker.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                //Intent intent = new Intent(......); //Intent to start openIntents File Manager
                //startActivityForResult(intent, requestMode);
                return true;
            }
        });
*/

        ListPreference download = (ListPreference) getPreferenceManager().findPreference("pref_game_download_directory");
        if( download != null ) {
            YabauseStorage ys = YabauseStorage.getStorage();
            if (ys.hasExternalSD() == false) {
                download.setEnabled(false);
            } else {
                download.setEnabled(true);
            }

            StatFs stat = new StatFs(ys.getGamePath());
            long bytesAvailable = stat.getBlockSizeLong() * stat.getAvailableBlocksLong();
            double gAvailable = (double)bytesAvailable / (double)(1024*1024*1024);
            String gbyte_str = String.format("%1$.2fGB", gAvailable);

            List<CharSequence> labels = new ArrayList<CharSequence>();
            labels.add( getString( R.string.internal) + "\n  " + getString(R.string.free_space) + " " + gbyte_str + "" );

            if (ys.hasExternalSD() == true ) {
                stat = new StatFs(ys.getExternalGamePath());
                bytesAvailable = stat.getBlockSizeLong() * stat.getAvailableBlocksLong();
                gAvailable = (double)bytesAvailable / (double)(1024*1024*1024);
                gbyte_str = String.format("%1$.2fGB", gAvailable);
                labels.add( getString( R.string.external) + "\n  " + getString(R.string.free_space) + " " + gbyte_str + "" );
            }

            CharSequence[] entries = new CharSequence[labels.size()];
            labels.toArray(entries);

            download.setEntries(entries);
            download.setSummary(download.getEntry());
        }

    	//InputSettingPreference inputsetting1 = (InputSettingPreference)findPreference("pref_inputdef_file");
        //inputsetting1.setPlayerAndFilename(0,"keymap");
    	//InputSettingPreference inputsetting2 = (InputSettingPreference)findPreference("pref_player2_inputdef_file");
        //inputsetting2.setPlayerAndFilename(1,"keymap_player2");

        ListPreference download = (ListPreference) getPreferenceManager().findPreference("pref_game_download_directory");
        if( download != null ) {
            YabauseStorage ys = YabauseStorage.getStorage();
            if (ys.hasExternalSD() == false) {
                download.setEnabled(false);
            } else {
                download.setEnabled(true);
            }

            StatFs stat = new StatFs(ys.getGamePath());
            long bytesAvailable = stat.getBlockSizeLong() * stat.getAvailableBlocksLong();
            double gAvailable = (double)bytesAvailable / (double)(1024*1024*1024);
            String gbyte_str = String.format("%1$.2fGB", gAvailable);

            List<CharSequence> labels = new ArrayList<CharSequence>();
            labels.add( getString( R.string.internal) + "\n  " + getString(R.string.free_space) + " " + gbyte_str + "" );

            if (ys.hasExternalSD() == true ) {
                stat = new StatFs(ys.getExternalGamePath());
                bytesAvailable = stat.getBlockSizeLong() * stat.getAvailableBlocksLong();
                gAvailable = (double)bytesAvailable / (double)(1024*1024*1024);
                gbyte_str = String.format("%1$.2fGB", gAvailable);
                labels.add( getString( R.string.external) + "\n  " + getString(R.string.free_space) + " " + gbyte_str + "" );
            }

            CharSequence[] entries = new CharSequence[labels.size()];
            labels.toArray(entries);

            download.setEntries(entries);
            download.setSummary(download.getEntry());
        }

    	//InputSettingPreference inputsetting1 = (InputSettingPreference)findPreference("pref_inputdef_file");
        //inputsetting1.setPlayerAndFilename(0,"keymap");
    	//InputSettingPreference inputsetting2 = (InputSettingPreference)findPreference("pref_player2_inputdef_file");
        //inputsetting2.setPlayerAndFilename(1,"keymap_player2");


        Resources res = getResources();
        YabauseStorage storage = YabauseStorage.getStorage();

        /* bios */
        ListPreference p = (ListPreference) getPreferenceManager().findPreference("pref_bios");

        List<CharSequence> labels = new ArrayList<CharSequence>();
        List<CharSequence> values = new ArrayList<CharSequence>();

        CharSequence[] biosfiles = storage.getBiosFiles();

        labels.add("built-in bios");
        values.add("");

        if ((biosfiles != null) && (biosfiles.length > 0)) {
            for(CharSequence bios : biosfiles) {
                labels.add(bios);
                values.add(bios);
            }

            CharSequence[] entries = new CharSequence[labels.size()];
            labels.toArray(entries);

            CharSequence[] entryValues = new CharSequence[values.size()];
            values.toArray(entryValues);

            p.setEntries(entries);
            p.setEntryValues(entryValues);
            p.setSummary(p.getEntry());
        } else {
            p.setEnabled(false);
            p.setSummary("built-in bios");
        }

        /* cartridge */
        ListPreference cart = (ListPreference) getPreferenceManager().findPreference("pref_cart");

        List<CharSequence> cartlabels = new ArrayList<CharSequence>();
        List<CharSequence> cartvalues = new ArrayList<CharSequence>();

        for(int carttype = 0;carttype < Cartridge.getTypeCount();carttype++) {
            cartlabels.add(Cartridge.getName(carttype));
            cartvalues.add(Integer.toString(carttype));
        }

        CharSequence[] cartentries = new CharSequence[cartlabels.size()];
        cartlabels.toArray(cartentries);

        CharSequence[] cartentryValues = new CharSequence[cartvalues.size()];
        cartvalues.toArray(cartentryValues);

        cart.setEntries(cartentries);
        cart.setEntryValues(cartentryValues);
        cart.setSummary(cart.getEntry());

        /* Cpu */
        ListPreference cpu_setting = (ListPreference) getPreferenceManager().findPreference("pref_cpu");
        cpu_setting.setSummary(cpu_setting.getEntry());
        String abi =System.getProperty("os.arch");
        if( abi.contains("64")){
            List<CharSequence> cpu_labels = new ArrayList<CharSequence>();
            List<CharSequence> cpu_values = new ArrayList<CharSequence>();

            cpu_labels.add(res.getString(R.string.new_dynrec_cpu_interface));
            cpu_values.add("3");
            cpu_labels.add(res.getString(R.string.software_cpu_interface));
            cpu_values.add("0");

            CharSequence[] cpu_entries = new CharSequence[cpu_labels.size()];
            cpu_labels.toArray(cpu_entries);

            CharSequence[] cpu_entryValues = new CharSequence[cpu_values.size()];
            cpu_values.toArray(cpu_entryValues);

            cpu_setting.setEntries(cpu_entries);
            cpu_setting.setEntryValues(cpu_entryValues);
            cpu_setting.setSummary(cpu_setting.getEntry());
        }

        /* Video */
        ListPreference video_cart = (ListPreference) getPreferenceManager().findPreference("pref_video");

        List<CharSequence> video_labels = new ArrayList<CharSequence>();
        List<CharSequence> video_values = new ArrayList<CharSequence>();

        final ActivityManager activityManager = (ActivityManager) getSystemService(this.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000;

        boolean deviceSupportsAEP = getPackageManager().hasSystemFeature
            (PackageManager.FEATURE_OPENGLES_EXTENSION_PACK);

        if( supportsEs3 ) {
            video_labels.add(res.getString(R.string.opengl_video_interface));
            video_values.add("1");
        }else{
            DialogFragment newFragment = new WarningDialogFragment();
            newFragment.show(getFragmentManager(), "OGL");
        }

        video_labels.add(res.getString(R.string.software_video_interface));
        video_values.add("2");

        CharSequence[] video_entries = new CharSequence[video_labels.size()];
        video_labels.toArray(video_entries);

        CharSequence[] video_entryValues = new CharSequence[video_values.size()];
        video_values.toArray(video_entryValues);

        video_cart.setEntries(video_entries);
        video_cart.setEntryValues(video_entryValues);
        video_cart.setSummary(video_cart.getEntry());

        /* Filter */
        ListPreference filter_setting = (ListPreference) getPreferenceManager().findPreference("pref_filter");
        filter_setting.setSummary(filter_setting.getEntry());
        if( video_cart.getValue().equals("1") ){
            filter_setting.setEnabled(true);
        }else{
            filter_setting.setEnabled(false);
        }

        EditTextPreference scsp_setting = (EditTextPreference) getPreferenceManager().findPreference("pref_scsp_sync_per_frame");
        scsp_setting.setSummary(scsp_setting.getText());

        //ListPreference cpu_sync_setting = (ListPreference) getPreferenceManager().findPreference("pref_cpu_sync_per_line");
        //cpu_sync_setting.setSummary(cpu_sync_setting.getEntry());

         /* Polygon Generation */
        ListPreference polygon_setting = (ListPreference) getPreferenceManager().findPreference("pref_polygon_generation");
        polygon_setting.setSummary(polygon_setting.getEntry());
        if( video_cart.getValue().equals("1") ){
            polygon_setting.setEnabled(true);
        }else{
            polygon_setting.setEnabled(false);
        }

        if( deviceSupportsAEP == false ){
            polygon_setting.setEntries(new String[]{"Triangles using perspective correction","CPU Tessellation"});
            polygon_setting.setEntryValues(new String[]{"0", "1"});
        }

        /* Plyaer1 input device */
        SyncInputDevice();

        /* Plyaer2 input device */
        SyncInputDeviceForPlayer2();


        PreferenceScreen onscreen_pad = (PreferenceScreen) findPreference("on_screen_pad");
        onscreen_pad.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                Intent nextActivity = new Intent(
                    YabauseSettings.this,
                    PadTestActivity.class);

                startActivity(nextActivity);
                return true;
            }
        });
/*
        Preference select_image = findPreference("select_image");
        select_image.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {

                Intent intent = new Intent();
                intent.setType("image/*");
                intent.setAction(Intent.ACTION_GET_CONTENT);
                int PICK_IMAGE = 1;
                startActivityForResult(Intent.createChooser(intent, "Select Picture"), PICK_IMAGE);

                return true;
            }
        });
*/
        ListPreference soundengine_setting = (ListPreference) getPreferenceManager().findPreference("pref_sound_engine");
        soundengine_setting.setSummary(soundengine_setting.getEntry());

        ListPreference resolution_setting = (ListPreference) getPreferenceManager().findPreference("pref_resolution");
        resolution_setting.setSummary(resolution_setting.getEntry());

        ListPreference aspect_setting = (ListPreference) getPreferenceManager().findPreference("pref_aspect_rate");
        aspect_setting.setSummary(aspect_setting.getEntry());

        ListPreference rbg_resolution_setting = (ListPreference) getPreferenceManager().findPreference("pref_rbg_resolution");
        rbg_resolution_setting.setSummary(rbg_resolution_setting.getEntry());

        ListPreference scsp_time_sync_setting = (ListPreference) getPreferenceManager().findPreference("scsp_time_sync_mode");
        scsp_time_sync_setting.setSummary(scsp_time_sync_setting.getEntry());

      }

    public String getRealPathFromURI(Uri image_path) {
        grantUriPermission("org.uoyabause.android",
                image_path,
                Intent.FLAG_GRANT_READ_URI_PERMISSION);

        try {
            InputStream inputStream = getContentResolver().openInputStream(image_path);

            BitmapFactory.Options imageOptions = new BitmapFactory.Options();
            imageOptions.inJustDecodeBounds = true;
            BitmapFactory.decodeStream(inputStream, null, imageOptions);
            Log.v("image", "Original Image Size: " + imageOptions.outWidth + " x " + imageOptions.outHeight);

            inputStream.close();

            Bitmap bitmap;
            int imageSizeMax = 1920;
            inputStream = getContentResolver().openInputStream(image_path);
            float imageScaleWidth = (float) imageOptions.outWidth / imageSizeMax;
            float imageScaleHeight = (float) imageOptions.outHeight / imageSizeMax;

            if (imageScaleWidth > 2 && imageScaleHeight > 2) {
                BitmapFactory.Options imageOptions2 = new BitmapFactory.Options();
                int imageScale = (int) Math.floor((imageScaleWidth > imageScaleHeight ? imageScaleHeight : imageScaleWidth));
                for (int i = 2; i <= imageScale; i *= 2) {
                    imageOptions2.inSampleSize = i;
                }
                bitmap = BitmapFactory.decodeStream(inputStream, null, imageOptions2);
                Log.v("image", "Sample Size: 1/" + imageOptions2.inSampleSize);
            } else {
                bitmap = BitmapFactory.decodeStream(inputStream);
            }

            inputStream.close();

            FileOutputStream out = null;
            String path =getFilesDir()+"/backgound.png";
            try {
                out = new FileOutputStream(path);
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, out); // bmp is your Bitmap instance
                // PNG is a lossless format, the compression factor (100) is ignored
            } catch (Exception e) {
                e.printStackTrace();
                return null;
            } finally {
                try {
                    if (out != null) {
                        out.close();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    return null;
                }
            }
            return path;
        }catch( Exception e ){
            Log.d("Yabasettings",e.getLocalizedMessage());
        }

        return null;
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent imageReturnedIntent) {
        super.onActivityResult(requestCode, resultCode, imageReturnedIntent);
/*
        if (resultCode == RESULT_OK) {
            Uri selectedImage = imageReturnedIntent.getData();
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
            String s = getRealPathFromURI(selectedImage);
            sp.edit().putString("select_image", s).commit();
        }
 */
    }

    private void SyncInputDevice(){

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        Resources res = getResources();
        PadManager padm = PadManager.getPadManager();

        ListPreference plyaer1_input_device = (ListPreference) getPreferenceManager().findPreference("pref_player1_inputdevice");

        List<CharSequence> Inputlabels = new ArrayList<CharSequence>();
        List<CharSequence> Inputvalues = new ArrayList<CharSequence>();

        Inputlabels.add(res.getString(R.string.onscreen_pad));
        Inputvalues.add("-1");



        String selInputdevice = sharedPref.getString("pref_player2_inputdevice", "65535");

        for(int inputType = 0;inputType < padm.getDeviceCount();inputType++) {
            //if( !selInputdevice.equals( padm.getId(inputType) ) ){
            Inputlabels.add(padm.getName(inputType));
            Inputvalues.add(padm.getId(inputType));
            //}
        }

        CharSequence[] input_entries = new CharSequence[Inputlabels.size()];
        Inputlabels.toArray(input_entries);

        CharSequence[] input_entryValues = new CharSequence[Inputvalues.size()];
        Inputvalues.toArray(input_entryValues);

        plyaer1_input_device.setEntries(input_entries);
        plyaer1_input_device.setEntryValues(input_entryValues);
        plyaer1_input_device.setSummary(plyaer1_input_device.getEntry());

/*
        InputSettingPreference inputsetting= (InputSettingPreference)findPreference("pref_inputdef_file");
        PreferenceScreen onscreen_pad = (PreferenceScreen) findPreference("on_screen_pad");
        if( inputsetting != null ){
            try {
                selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
                if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
                    inputsetting.setEnabled(true);
                    onscreen_pad.setEnabled(true);
                }else{
                    inputsetting.setEnabled(false);
                    onscreen_pad.setEnabled(true);
                }

                padm.setPlayer1InputDevice(selInputdevice);

            }catch( Exception e ){
                e.printStackTrace();
            }
        }
 */
    }

    private void SyncInputDeviceForPlayer2(){

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        Resources res = getResources();
        PadManager padm = PadManager.getPadManager();

        ListPreference plyaer2_input_device = (ListPreference) getPreferenceManager().findPreference("pref_player2_inputdevice");

        List<CharSequence> Inputlabels_p2 = new ArrayList<CharSequence>();
        List<CharSequence> Inputvalues_p2 = new ArrayList<CharSequence>();

        Inputlabels_p2.add(res.getString(R.string.nopad));
        Inputvalues_p2.add("-1");

        String selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");

        for(int inputType = 0;inputType < padm.getDeviceCount();inputType++) {

            //if( !selInputdevice.equals( padm.getId(inputType) ) ){
            Inputlabels_p2.add(padm.getName(inputType));
            Inputvalues_p2.add(padm.getId(inputType));
            //}
        }

        CharSequence[] input_entries_p2 = new CharSequence[Inputlabels_p2.size()];
        Inputlabels_p2.toArray(input_entries_p2);

        CharSequence[] input_entryValues_p2 = new CharSequence[Inputvalues_p2.size()];
        Inputvalues_p2.toArray(input_entryValues_p2);

        plyaer2_input_device.setEntries(input_entries_p2);
        plyaer2_input_device.setEntryValues(input_entryValues_p2);
        plyaer2_input_device.setSummary(plyaer2_input_device.getEntry());

/*
        InputSettingPreference inputsetting= (InputSettingPreference)findPreference("pref_player2_inputdef_file");
        if( inputsetting != null ){
            try {
                selInputdevice = sharedPref.getString("pref_player2_inputdevice", "65535");
                if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
                    inputsetting.setEnabled(true);
                }else{
                    inputsetting.setEnabled(false);
                }
                padm.setPlayer2InputDevice(selInputdevice);

            }catch( Exception e ){
                e.printStackTrace();
            }
        }
 */
    }
    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals("pref_bios") ||
                key.equals("scsp_time_sync_mode") ||
                key.equals("pref_cart") ||
            key.equals("pref_video") || 
            key.equals("pref_cpu") || 
            key.equals("pref_filter") || 
            key.equals("pref_polygon_generation") || 
            key.equals("pref_sound_engine" ) ||
            key.equals("pref_resolution") ||
            key.equals("pref_rbg_resolution") ||
            key.equals("pref_cpu_sync_per_line") ||
            key.equals("pref_aspect_rate")

            ) {
                ListPreference pref = (ListPreference) findPreference(key);
                pref.setSummary(pref.getEntry());

                if (key.equals("pref_video")) {
                    ListPreference filter_setting = (ListPreference) getPreferenceManager().findPreference("pref_filter");
                    if (pref.getValue().equals("1")) {
                        filter_setting.setEnabled(true);
                    } else {
                        filter_setting.setEnabled(false);
                    }

                ListPreference polygon_setting = (ListPreference) getPreferenceManager().findPreference("pref_polygon_generation");
                polygon_setting.setSummary(polygon_setting.getEntry());
                if (pref.getValue().equals("1")) {
                    polygon_setting.setEnabled(true);
                } else {
                    polygon_setting.setEnabled(false);
                }
            }
            ListPreference download = (ListPreference) getPreferenceManager().findPreference("pref_game_download_directory");
            if( download != null) {
                download.setSummary(download.getEntry());
            }

            if( key.equals("pref_scsp_sync_per_frame") ){
                EditTextPreference ep = (EditTextPreference)findPreference(key);
                String sval = ep.getText();
                int val =  Integer.parseInt(sval);
                if( val <= 0 ){
                    val = 1;
                    SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
                    sp.edit().putString("pref_scsp_sync_per_frame", String.valueOf(val)).commit();
                }else if( val > 255 ){
                    val = 255;
                    SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
                    sp.edit().putString("pref_scsp_sync_per_frame", String.valueOf(val)).commit();
                }
                ep.setSummary( String.valueOf(val) );
            }

        if (key.equals("pref_force_androidtv_mode") ){
            if(restart_level<=1 ) restart_level = 2;
            updateResultCode();
        }


      }

    void updateResultCode(){
        Intent resultIntent = new Intent();
        if( restart_level == 1 ) {
            setResult(GameSelectFragment.GAMELIST_NEED_TO_UPDATED, resultIntent);
        } else if( restart_level == 2 ) {
            setResult(GameSelectFragment.GAMELIST_NEED_TO_RESTART, resultIntent);
        }else{
            setResult(0, resultIntent);
        }
    }

    private Preference.OnPreferenceChangeListener myCheckboxListener = new Preference.OnPreferenceChangeListener() {

        public boolean onPreferenceChange(Preference preference, Object newValue) {
            //if( newValue == false ){
            //}
            return true;
        }
    };
        @Override
        protected void onResume () {
            super.onResume();
            mInputManager.registerInputDeviceListener(this, null);
            getPreferenceScreen().getSharedPreferences()
                    .registerOnSharedPreferenceChangeListener(this);

        }

        @Override
        protected void onPause () {
            super.onPause();
            mInputManager.unregisterInputDeviceListener(this);
            getPreferenceScreen().getSharedPreferences()
                    .unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();

        }
}
