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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.View;
import android.os.Bundle;
import org.uoyabause.uranus.R;

import org.uoyabause.android.GameList;
public class Home extends Activity
{
    private static final String TAG = "Yabause";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.home);
        String vname = getVersionName(this);
        this.setTitle("uoYabause "+vname);
    }

    public void onPadTest(View view)
    {
        Intent intent = new Intent(this, PadTestActivity.class);
        startActivity(intent);
    }
    
    public void onLoadGame(View view)
    {
        Intent intent = new Intent(this, GameList.class);
        startActivity(intent);
    }

    public void onSettings(View view)
    {
        Intent intent = new Intent(this, YabauseSettings.class);
        startActivity(intent);
    }
    
    public void onDonate(View view)
    {
        Intent intent = new Intent(this, DonateActivity.class);
        startActivity(intent);   	
    }

    /**
     * 
     *
     * @param context
     * @return
     */
    public static String getVersionName(Context context){
        PackageManager pm = context.getPackageManager();
        String versionName = "";
        try{
            PackageInfo packageInfo = pm.getPackageInfo(context.getPackageName(), 0);
            versionName = packageInfo.versionName;
        }catch(NameNotFoundException e){
            e.printStackTrace();
        }
        return versionName;    
    }
}
