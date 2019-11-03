/*  Copyright 2015 Guillaume Duhamel

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

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import android.app.ListActivity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.widget.ListView;

import org.uoyabause.android.FileDialog.FileSelectedListener;
import org.uoyabause.android.GameListAdapter;
import org.uoyabause.android.Yabause;



public class GameList extends ListActivity implements FileSelectedListener
{
    private static final String TAG = "Yabause";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        GameListAdapter adapter = new GameListAdapter(this);
        setListAdapter(adapter);
    }

    private final static int CHOSE_FILE_CODE = 7123;
    
    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {

        String string = (String) l.getItemAtPosition(position);
        
    	if( position == 0 ){
    		
    		// This method is not supported on Android TV
    	    //Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            //intent.setType("file/*");
            //startActivityForResult(intent, CHOSE_FILE_CODE);
    		
    		File yabroot = new File(Environment.getExternalStorageDirectory(), "yabause");
    		
            SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
            String  last_dir = sharedPref.getString("pref_last_dir", yabroot.getPath());
                      
            
    		FileDialog fd = new FileDialog(this,last_dir);
    		fd.addFileListener(this);
    		fd.showDialog();
    		
    	}else{
    		Intent intent = new Intent(this, Yabause.class);
    		intent.putExtra("org.uoyabause.android.FileName", string);
    		startActivity(intent);
    	}
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    	try {
              if (requestCode == CHOSE_FILE_CODE && resultCode == RESULT_OK) {
                  String filePath = data.getDataString().replace("file://", "");
                  String decodedfilePath = URLDecoder.decode(filePath, "utf-8");
          			Intent intent = new Intent(this, Yabause.class);
          			intent.putExtra("org.uoyabause.android.FileNameEx", decodedfilePath);
          			startActivity(intent);
              }
          } catch (UnsupportedEncodingException e) {
        	  e.printStackTrace();
          }
     }

	@Override
	public void fileSelected(File file) {
		String string;
		string = file.getAbsolutePath();
		
		YabauseStorage storage = YabauseStorage.getStorage();

		// save last selected dir
		SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPreferences.Editor editor = sharedPref.edit();
		editor.putString("pref_last_dir", file.getParent());
		editor.apply();
		
		/* seems it doesn't work
		ProcessBuilder pb = new ProcessBuilder("ln", "-s", string, storage.getGamePath());
		try {
			Process p = pb.start();
		} catch (IOException e) {
			e.printStackTrace();
		}
		*/
		Intent intent = new Intent(this, Yabause.class);
		intent.putExtra("org.uoyabause.android.FileNameEx", string);
		startActivity(intent);
	}

}
