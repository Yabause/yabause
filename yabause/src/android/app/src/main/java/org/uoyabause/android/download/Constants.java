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

package org.uoyabause.android.download;


public final class Constants {

    // Defines a custom Intent action
    public static final String BROADCAST_ACTION = "org.uoyabause.android.download.BROADCAST";

    public static final String EXTENDED_DATA_STATUS = "org.uoyabause.android.download.STATUS";

    public static final String BROADCAST_STOP = "org.uoyabause.android.download.STOP";

    public static final int MESSAGE_TYPE = 0;

    public static final int TYPE_UPDATE_STATE = 0;
    public static final int TYPE_UPDATE_MESSAGE = 1;
    public static final int TYPE_UPDATE_DOWNLOAD = 2;

    public static final String NEW_STATE = "org.uoyabause.android.download.NEWSTATE";

    public static final String FINISH_STATUS = "org.uoyabause.android.download.FINISH_STATUS";

    public static final String MESSAGE = "org.uoyabause.android.download.MESSAGE";

    public static final String ERROR_MESSAGE = "org.uoyabause.android.download.ERROR_MESSAGE";

    public static final String CURRENT = "org.uoyabause.android.download.CURRENT";
    public static final String MAX = "org.uoyabause.android.download.MAX";
    public static final String BPS = "org.uoyabause.android.download.BPS";

    public static final int STATE_IDLE = 0;
    public static final int STATE_SERCHING_SERVER = 1;
    public static final int STATE_REQUEST_GENERATE_ISO = 2;
    public static final int STATE_DOWNLOADING = 3;
    public static final int STATE_CHECKING_FILE = 4;
    public static final int STATE_FINISHED = 5;
}
