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

package org.uoyabause.android;

import java.util.ArrayList;
import java.util.List;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;
import android.content.Intent;
import android.content.SharedPreferences;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.content.res.Resources;
import android.app.DialogFragment;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.app.Dialog;

public class YabauseSettings extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener{

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
  
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        try{
        	addPreferencesFromResource(R.xml.preferences);
        }catch( Exception e ){
        	e.printStackTrace();
        }
      
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

        /* Video */
        ListPreference video_cart = (ListPreference) getPreferenceManager().findPreference("pref_video");

        List<CharSequence> video_labels = new ArrayList<CharSequence>();
        List<CharSequence> video_values = new ArrayList<CharSequence>();

        final ActivityManager activityManager = (ActivityManager) getSystemService(this.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000;

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
        
        
        /* Plyaer1 input device */
        ListPreference plyaer1_input_device = (ListPreference) getPreferenceManager().findPreference("pref_player1_inputdevice");

        List<CharSequence> Inputlabels = new ArrayList<CharSequence>();
        List<CharSequence> Inputvalues = new ArrayList<CharSequence>();
        
        Inputlabels.add(res.getString(R.string.onscreen_pad));
        Inputvalues.add("-1");
        
        PadManager padm = PadManager.getPadManager();

        for(int inputType = 0;inputType < padm.getDeviceCount();inputType++) {
        	Inputlabels.add(padm.getName(inputType));
        	Inputvalues.add(padm.getId(inputType));
        }

        CharSequence[] input_entries = new CharSequence[Inputlabels.size()];
        Inputlabels.toArray(input_entries);

        CharSequence[] input_entryValues = new CharSequence[Inputvalues.size()];
        Inputvalues.toArray(input_entryValues);

        plyaer1_input_device.setEntries(input_entries);
        plyaer1_input_device.setEntryValues(input_entryValues);
        plyaer1_input_device.setSummary(plyaer1_input_device.getEntry());
        
        SyncInputDevice();
        
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
        
      }
    
    private void SyncInputDevice(){
        InputSettingPrefernce inputsetting= (InputSettingPrefernce)findPreference("pref_inputdef_file");
        PreferenceScreen onscreen_pad = (PreferenceScreen) findPreference("on_screen_pad");
        if( inputsetting != null ){
        	PadManager padm = PadManager.getPadManager();
            SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
             try {
            	String selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
                if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
                	inputsetting.setEnabled(true);
                	onscreen_pad.setEnabled(false);
                }else{
                	inputsetting.setEnabled(false);
                	onscreen_pad.setEnabled(true);
                }            	
                
                padm.setPlayer1InputDevice(selInputdevice);
                
            }catch( Exception e ){
            	e.printStackTrace();
            }
        }
    	
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals("pref_bios") || key.equals("pref_cart") || key.equals("pref_video")) {
            ListPreference pref = (ListPreference) findPreference(key);
            pref.setSummary(pref.getEntry());
        }
        
        else if (key.equals("pref_player1_inputdevice") ) {
        	ListPreference pref = (ListPreference) findPreference(key);
        	pref.setSummary(pref.getEntry());
        	SyncInputDevice();
/*        	
        	PadManager padm = PadManager.getPadManager();
        	String selInputdevice = sharedPreferences.getString(key, "0");
        	if( padm.getDeviceCount() > 0 && !selInputdevice.equals("-1") ){
        		InputSettingPrefernce InputSetting = new InputSettingPrefernce(this);
        		InputSetting.showDialog(null);
        	}
*/        	
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        getPreferenceScreen().getSharedPreferences()
                .registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        getPreferenceScreen().getSharedPreferences()
                .unregisterOnSharedPreferenceChangeListener(this);
    }

}
