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

public class Cartridge {
    final static int CART_NONE            =  0;
    final static int CART_PAR             =  1;
    final static int CART_BACKUPRAM4MBIT  =  2;
    final static int CART_BACKUPRAM8MBIT  =  3;
    final static int CART_BACKUPRAM16MBIT =  4;
    final static int CART_BACKUPRAM32MBIT =  5;
    final static int CART_DRAM8MBIT       =  6;
    final static int CART_DRAM32MBIT      =  7;
    final static int CART_NETLINK         =  8;
    final static int CART_ROM16MBIT       =  9;

    static public String getName(int cartridgetype) {
        switch(cartridgetype) {
            case CART_NONE:
	            return "None";
            case CART_PAR:
	            return "Pro Action Replay";
            case CART_BACKUPRAM4MBIT:
	            return "4 Mbit Backup Ram";
            case CART_BACKUPRAM8MBIT:
	            return "8 Mbit Backup Ram";
            case CART_BACKUPRAM16MBIT:
	            return "16 Mbit Backup Ram";
            case CART_BACKUPRAM32MBIT:
	            return "32 Mbit Backup Ram";
            case CART_DRAM8MBIT:
	            return "8 Mbit Dram";
            case CART_DRAM32MBIT:
	            return "32 Mbit Dram";
            case CART_NETLINK:
	            return "Netlink";
            case CART_ROM16MBIT:
	            return "16 Mbit ROM";
        }

        return null;
    }

    static public String getDefaultFilename(int cartridgetype) {
        switch(cartridgetype) {
            case CART_NONE:
                return "none.ram";
            case CART_PAR:
                return "par.ram";
            case CART_BACKUPRAM4MBIT:
                return "backup4.ram";
            case CART_BACKUPRAM8MBIT:
                return "backup8.ram";
            case CART_BACKUPRAM16MBIT:
                return "backup16.ram";
            case CART_BACKUPRAM32MBIT:
                return "backup32.ram";
            case CART_DRAM8MBIT:
                return "dram8.ram";
            case CART_DRAM32MBIT:
                return "dram32.ram";
            case CART_NETLINK:
                return "netlink.ram";
            case CART_ROM16MBIT:
                return "rom16.ram";
        }

        return null;
    }

    static public int getTypeCount() {
        return 10;
    }
}
