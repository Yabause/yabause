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

package org.yabause.android;

import java.util.ArrayList;
import java.util.List;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.content.SharedPreferences;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.app.DialogFragment;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.app.Dialog;

public class YabauseSettings extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener {

  public static class WarningDialogFragment extends DialogFragment {
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
      // Use the Builder class for convenient dialog construction
      AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
      builder.setMessage("Your device is not support OpenGL ES 3.0 or above.\nYou can not choose OpenGL Video Interface.\n")
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
        addPreferencesFromResource(R.xml.preferences);

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
          video_labels.add("OpenGL Video Interface");
          video_values.add("1");
        }else{
          DialogFragment newFragment = new WarningDialogFragment();
          newFragment.show(getFragmentManager(), "OGL");
        }

        video_labels.add("Software Video Interface");
        video_values.add("2");

        CharSequence[] video_entries = new CharSequence[video_labels.size()];
        video_labels.toArray(video_entries);

        CharSequence[] video_entryValues = new CharSequence[video_values.size()];
        video_values.toArray(video_entryValues);

        video_cart.setEntries(video_entries);
        video_cart.setEntryValues(video_entryValues);
        video_cart.setSummary(video_cart.getEntry());
      }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals("pref_bios") || key.equals("pref_cart") || key.equals("pref_video")) {
            ListPreference pref = (ListPreference) findPreference(key);
            pref.setSummary(pref.getEntry());
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
