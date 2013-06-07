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

public class YabauseSettings extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener {
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

        /* game */
        ListPreference game = (ListPreference) getPreferenceManager().findPreference("pref_game");

        List<CharSequence> gamelabels = new ArrayList<CharSequence>();
        List<CharSequence> gamevalues = new ArrayList<CharSequence>();

        CharSequence[] gamefiles = storage.getGameFiles();
        if (gamefiles != null) {

            for(CharSequence gamefile : gamefiles) {
                gamelabels.add(gamefile);
                gamevalues.add(gamefile);
            }

            CharSequence[] gameentries = new CharSequence[gamelabels.size()];
            gamelabels.toArray(gameentries);

            CharSequence[] gameentryValues = new CharSequence[gamevalues.size()];
            gamevalues.toArray(gameentryValues);

            game.setEntries(gameentries);
            game.setEntryValues(gameentryValues);
            game.setSummary(game.getEntry());
        } else {
            game.setEnabled(false);
            game.setSummary("no game found");
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals("pref_bios")) {
            ListPreference biosPref = (ListPreference) findPreference(key);
            biosPref.setSummary(biosPref.getEntry());
        } else if (key.equals("pref_game")) {
            ListPreference gamePref = (ListPreference) findPreference(key);
            gamePref.setSummary(gamePref.getEntry());
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
