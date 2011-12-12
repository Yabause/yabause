/*  Copyright 2011 Guillaume Duhamel

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

import java.lang.Runnable;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.View;
import android.util.AttributeSet;

public class YabauseView extends View {
    public Bitmap bitmap;

    public YabauseView(Context context) {
        super(context);

        final int W = 320;
        final int H = 240;

        bitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
    }

    public YabauseView(Context context, AttributeSet attrs) {
        super(context, attrs);

        final int W = 320;
        final int H = 240;

        bitmap = Bitmap.createBitmap(W, H, Bitmap.Config.ARGB_8888);
    }

    @Override protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(bitmap, 0, 0, null);
        invalidate();
    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(320, 240);
    }
}
