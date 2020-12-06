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

object Cartridge {
    const val CART_NONE = 0
    const val CART_PAR = 1
    const val CART_BACKUPRAM4MBIT = 2
    const val CART_BACKUPRAM8MBIT = 3
    const val CART_BACKUPRAM16MBIT = 4
    const val CART_BACKUPRAM32MBIT = 5
    const val CART_DRAM8MBIT = 6
    const val CART_DRAM32MBIT = 7
    const val CART_NETLINK = 8
    const val CART_ROM16MBIT = 9
    @JvmStatic
    fun getName(cartridgetype: Int): String {
        when (cartridgetype) {
            CART_NONE -> return "None"
            CART_PAR -> return "Pro Action Replay"
            CART_BACKUPRAM4MBIT -> return "4 Mbit Backup Ram"
            CART_BACKUPRAM8MBIT -> return "8 Mbit Backup Ram"
            CART_BACKUPRAM16MBIT -> return "16 Mbit Backup Ram"
            CART_BACKUPRAM32MBIT -> return "32 Mbit Backup Ram"
            CART_DRAM8MBIT -> return "8 Mbit Dram"
            CART_DRAM32MBIT -> return "32 Mbit Dram"
            CART_NETLINK -> return "Netlink"
            CART_ROM16MBIT -> return "16 Mbit ROM"
        }
        return "null"
    }

    fun getDefaultFilename(cartridgetype: Int): String {
        when (cartridgetype) {
            CART_NONE -> return "none.ram"
            CART_PAR -> return "par.ram"
            CART_BACKUPRAM4MBIT -> return "backup4.ram"
            CART_BACKUPRAM8MBIT -> return "backup8.ram"
            CART_BACKUPRAM16MBIT -> return "backup16.ram"
            CART_BACKUPRAM32MBIT -> return "backup32.ram"
            CART_DRAM8MBIT -> return "dram8.ram"
            CART_DRAM32MBIT -> return "dram32.ram"
            CART_NETLINK -> return "netlink.ram"
            CART_ROM16MBIT -> return "rom16.ram"
        }
        return "dram32.ram"
    }

    @JvmStatic
    val typeCount: Int
        get() = 10
}
