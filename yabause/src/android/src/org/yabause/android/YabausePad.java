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

package org.yabause.android;

import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.View.OnTouchListener;
import android.view.View;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import java.util.HashMap;

import org.yabause.android.PadEvent;

class PadButton {
    protected Rect rect;

    PadButton() {
        rect = new Rect();
    }

    public void updateRect(int x1, int y1, int x2, int y2) {
        rect.set(x1, y1, x2, y2);
    }

    public boolean contains(int x, int y) {
        return rect.contains(x, y);
    }

    public void draw(Canvas canvas, Paint back, Paint front) {
    }
}

class DPadButton extends PadButton {
    public void draw(Canvas canvas, Paint back, Paint front) {
        canvas.drawRect(rect, back);
    }
}

class StartButton extends PadButton {
    public void draw(Canvas canvas, Paint back, Paint front) {
        canvas.drawOval(new RectF(rect), back);
    }
}

class ActionButton extends PadButton {
    private int width;
    private String text;
    private int textsize;

    ActionButton(int w, String t, int ts) {
        super();
        width = w;
        text = t;
        textsize = ts;
    }

    public void draw(Canvas canvas, Paint back, Paint front) {
        canvas.drawCircle(rect.centerX(), rect.centerY(), width, back);
        front.setTextSize(textsize);
        front.setTextAlign(Paint.Align.CENTER);
        canvas.drawText(text, rect.centerX(), rect.centerY() + (width / 2), front);
    }
}

interface OnPadListener {
    public abstract boolean onPad(PadEvent event);
}

class YabausePad extends View implements OnTouchListener {
    private PadButton buttons[];
    private OnPadListener listener = null;
    private HashMap<Integer, Integer> active;

    public YabausePad(Context context) {
        super(context);
        init();
    }

    public YabausePad(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public YabausePad(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    private void init() {
        setOnTouchListener(this);

        buttons = new PadButton[PadEvent.BUTTON_LAST];

        buttons[PadEvent.BUTTON_UP]    = new DPadButton();
        buttons[PadEvent.BUTTON_RIGHT] = new DPadButton();
        buttons[PadEvent.BUTTON_DOWN]  = new DPadButton();
        buttons[PadEvent.BUTTON_LEFT]  = new DPadButton();

        buttons[PadEvent.BUTTON_RIGHT_TRIGGER] = new PadButton();
        buttons[PadEvent.BUTTON_LEFT_TRIGGER]  = new PadButton();

        buttons[PadEvent.BUTTON_START] = new StartButton();

        buttons[PadEvent.BUTTON_A] = new ActionButton(30, "A", 40);
        buttons[PadEvent.BUTTON_B] = new ActionButton(30, "B", 40);
        buttons[PadEvent.BUTTON_C] = new ActionButton(30, "C", 40);

        buttons[PadEvent.BUTTON_X] = new ActionButton(20, "X", 25);
        buttons[PadEvent.BUTTON_Y] = new ActionButton(20, "Y", 25);
        buttons[PadEvent.BUTTON_Z] = new ActionButton(20, "Z", 25);

        active = new HashMap<Integer, Integer>();
    }

    @Override public void onDraw(Canvas canvas) {
        Paint paint = new Paint();
        paint.setARGB(0x80, 0x80, 0x80, 0x80);
        Paint apaint = new Paint();
        apaint.setARGB(0x80, 0xFF, 0x00, 0x00);
        Paint tpaint = new Paint();
        tpaint.setARGB(0x80, 0xFF, 0xFF, 0xFF);

        for(int i = 0;i < PadEvent.BUTTON_LAST;i++) {
            Paint p = active.containsValue(i) ? apaint : paint;
            buttons[i].draw(canvas, p, tpaint);
        }
    }

    public void setOnPadListener(OnPadListener listener) {
        this.listener = listener;
    }

    public boolean onTouch(View v, MotionEvent event) {
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        int posx = (int) event.getX(index);
        int posy = (int) event.getY(index);
        PadEvent pe = null;

        if ((action == event.ACTION_DOWN) || (action == event.ACTION_POINTER_DOWN)) {
            for(int i = 0;i < PadEvent.BUTTON_LAST;i++) {
                if (buttons[i].contains(posx, posy)) {
                    active.put(index, i);
                    pe = new PadEvent(action, i);
                }
            }
        }

        if (((action == event.ACTION_UP) || (action == event.ACTION_POINTER_UP)) && active.containsKey(index)) {
            int i = active.remove(index);
            pe = new PadEvent(action, i);
        }

        if ((listener != null) && (pe != null)) {
            invalidate();
            listener.onPad(pe);
            return true;
        }

        return false;
    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);

        buttons[PadEvent.BUTTON_UP].updateRect(100, getHeight() - 210, 140, getHeight() - 150);
        buttons[PadEvent.BUTTON_RIGHT].updateRect(150, getHeight() - 140, 210, getHeight() - 100);
        buttons[PadEvent.BUTTON_DOWN].updateRect(100, getHeight() - 90,  140, getHeight() - 30);
        buttons[PadEvent.BUTTON_LEFT].updateRect(30,  getHeight() - 140, 90,  getHeight() - 100);

        buttons[PadEvent.BUTTON_START].updateRect(getWidth() / 2 - 40, getHeight() - 60, getWidth() / 2 + 40, getHeight() - 15);

        buttons[PadEvent.BUTTON_A].updateRect(getWidth() - 235, getHeight() - 75, getWidth() - 185, getHeight() - 25);
        buttons[PadEvent.BUTTON_B].updateRect(getWidth() - 165, getHeight() - 125, getWidth() - 115, getHeight() - 75);
        buttons[PadEvent.BUTTON_C].updateRect(getWidth() - 75, getHeight() - 155, getWidth() - 25, getHeight() - 105);

        buttons[PadEvent.BUTTON_X].updateRect(getWidth() - 280, getHeight() - 140, getWidth() - 240, getHeight() - 100);
        buttons[PadEvent.BUTTON_Y].updateRect(getWidth() - 210, getHeight() - 190, getWidth() - 170, getHeight() - 150);
        buttons[PadEvent.BUTTON_Z].updateRect(getWidth() - 120, getHeight() - 220, getWidth() - 80, getHeight() - 180);

        setMeasuredDimension(width, height);
    }
}
