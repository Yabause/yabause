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

package org.uoyabause.android.tv;

public class Subscription {
    private long channelId;
    private String name;
    private String description;
    private String appLinkIntentUri;
    private int channelLogo;

    /** Constructor for Gson to use. */
    public Subscription() {}

    private Subscription(
            String name, String description, String appLinkIntentUri, int channelLogo) {
        this.name = name;
        this.description = description;
        this.appLinkIntentUri = appLinkIntentUri;
        this.channelLogo = channelLogo;
    }

    public static Subscription createSubscription(
            String name, String description, String appLinkIntentUri, int channelLogo) {
        return new Subscription(name, description, appLinkIntentUri, channelLogo);
    }

    public long getChannelId() {
        return channelId;
    }

    public void setChannelId(long channelId) {
        this.channelId = channelId;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getAppLinkIntentUri() {
        return appLinkIntentUri;
    }

    public void setAppLinkIntentUri(String appLinkIntentUri) {
        this.appLinkIntentUri = appLinkIntentUri;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public int getChannelLogo() {
        return channelLogo;
    }

    public void setChannelLogo(int channelLogo) {
        this.channelLogo = channelLogo;
    }

}
