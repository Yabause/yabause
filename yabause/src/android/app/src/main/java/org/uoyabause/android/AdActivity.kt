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

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.ImageButton
import androidx.appcompat.app.AppCompatActivity

class AdActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(org.devmiyax.yabasanshiro.R.layout.activity_ad)

        val button = findViewById<ImageButton>(org.devmiyax.yabasanshiro.R.id.got_to_store_button)
        button.setOnClickListener {
            Log.v("aaa", "clicked")
            val url = "https://play.google.com/store/apps/details?id=org.devmiyax.yabasanshioro2.pro"
            val intent = Intent(Intent.ACTION_VIEW)
            intent.data = Uri.parse(url)
            intent.setPackage("com.android.vending")
            startActivity(intent)
        }

        findViewById<Button>(org.devmiyax.yabasanshiro.R.id.close)?.setOnClickListener {
            finish()
        }
    }
}
