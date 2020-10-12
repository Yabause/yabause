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
package org.uoyabause.android

import android.app.Activity
import android.media.AudioManager
import android.media.AudioManager.OnAudioFocusChangeListener

class YabauseAudio internal constructor(private val activity: Activity) :
    OnAudioFocusChangeListener {
    val SYSTEM = 1
    val USER = 2
    private var muteFlags: Int
    private var muted: Boolean
    fun mute(flags: Int) {
        muted = true
        muteFlags = muteFlags or flags
        val am = activity.getSystemService(Activity.AUDIO_SERVICE) as AudioManager
        am.abandonAudioFocus(this)
        YabauseRunnable.setVolume(0)
    }

    fun unmute(flags: Int) {
        muteFlags = muteFlags and flags.inv()
        if (0 == muteFlags) {
            muted = false
            val am = activity.getSystemService(Activity.AUDIO_SERVICE) as AudioManager
            val result =
                am.requestAudioFocus(this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
            if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                YabauseRunnable.setVolume(0)
            } else {
                YabauseRunnable.setVolume(100)
            }
        }
    }

    override fun onAudioFocusChange(focusChange: Int) {
        if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
            mute(SYSTEM)
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
            YabauseRunnable.setVolume(50)
        } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
            if (muted) unmute(SYSTEM) else YabauseRunnable.setVolume(100)
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
            mute(SYSTEM)
        }
    }

    init {
        activity.volumeControlStream = AudioManager.STREAM_MUSIC
        muteFlags = 0
        muted = false
    }
}
