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

class PadEvent(val action: Int, val key: Int) {

    companion object {
        const val BUTTON_UP = 0
        const val BUTTON_RIGHT = 1
        const val BUTTON_DOWN = 2
        const val BUTTON_LEFT = 3
        const val BUTTON_RIGHT_TRIGGER = 4
        const val BUTTON_LEFT_TRIGGER = 5
        const val BUTTON_START = 6
        const val BUTTON_A = 7
        const val BUTTON_B = 8
        const val BUTTON_C = 9
        const val BUTTON_X = 10
        const val BUTTON_Y = 11
        const val BUTTON_Z = 12
        const val BUTTON_LAST = 13
        const val PERANALOG_AXIS_X = 18 // left to right
        const val PERANALOG_AXIS_Y = 19 // up to down
        const val PERANALOG_AXIS_RTRIGGER = 20 // right trigger
        const val PERANALOG_AXIS_LTRIGGER = 21 // left trigger
        const val MENU = 22 // show menu botton
    }
}
