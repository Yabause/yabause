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

package org.uoyabause.android;

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
import android.util.DisplayMetrics;

import org.uoyabause.android.PadEvent;

class PadButton {
    protected Rect rect;
    protected int isOn;
    Paint back;

    PadButton() {
        isOn = -1;
        rect = new Rect();
    }

    public void updateRect(int x1, int y1, int x2, int y2) {
        rect.set(x1, y1, x2, y2);
    }

    public boolean contains(int x, int y ) {
        return rect.contains(x, y);
    }

    public boolean intersects( Rect r ){
      return Rect.intersects(rect,r);
    }

    public void draw(Canvas canvas, Paint nomal_back, Paint active_back, Paint front) {
      if( isOn != -1 ){
        back = active_back;
      }else{
        back = nomal_back;
      }
    }

    void On( int index ){
      isOn = index;
    }

    void Off(){
      isOn = -1;
    }

    boolean isOn( int index ){
      if( isOn == index ){
        return true;
      }else{
        return false;
      }
    }
}



class DPadButton extends PadButton {
    public void draw(Canvas canvas, Paint nomal_back, Paint active_back, Paint front) {
        super.draw(canvas,nomal_back,active_back,front);
        canvas.drawRect(rect, back);
    }
}

class StartButton extends PadButton {
    public void draw(Canvas canvas, Paint nomal_back, Paint active_back, Paint front) {
      super.draw(canvas,nomal_back,active_back,front);
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

    public void draw(Canvas canvas, Paint nomal_back, Paint active_back, Paint front) {
        super.draw(canvas,nomal_back,active_back,front);
        canvas.drawCircle(rect.centerX(), rect.centerY(), width, back);
        front.setTextSize(textsize);
        front.setTextAlign(Paint.Align.CENTER);
        canvas.drawText(text, rect.centerX() , rect.centerY() , front);
    }
}


interface OnPadListener {
    public abstract boolean onPad(PadEvent event);
}

class YabausePad extends View implements OnTouchListener {
    private PadButton buttons[];
    private OnPadListener listener = null;
    private HashMap<Integer, Integer> active;
    private DisplayMetrics metrics;

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

	metrics = getResources().getDisplayMetrics();

        setOnTouchListener(this);

        buttons = new PadButton[PadEvent.BUTTON_LAST];

        buttons[PadEvent.BUTTON_UP]    = new DPadButton();
        buttons[PadEvent.BUTTON_RIGHT] = new DPadButton();
        buttons[PadEvent.BUTTON_DOWN]  = new DPadButton();
        buttons[PadEvent.BUTTON_LEFT]  = new DPadButton();

        buttons[PadEvent.BUTTON_RIGHT_TRIGGER] = new PadButton();
        buttons[PadEvent.BUTTON_LEFT_TRIGGER]  = new PadButton();

        buttons[PadEvent.BUTTON_START] = new StartButton();

        buttons[PadEvent.BUTTON_A] = new ActionButton((int)(30*metrics.density), "A", 40);
        buttons[PadEvent.BUTTON_B] = new ActionButton((int)(30*metrics.density), "B", 40);
        buttons[PadEvent.BUTTON_C] = new ActionButton((int)(30*metrics.density), "C", 40);

        buttons[PadEvent.BUTTON_X] = new ActionButton((int)(20*metrics.density), "X", 25);
        buttons[PadEvent.BUTTON_Y] = new ActionButton((int)(20*metrics.density), "Y", 25);
        buttons[PadEvent.BUTTON_Z] = new ActionButton((int)(20*metrics.density), "Z", 25);

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
            //Paint p = active.containsValue(i) ? apaint : paint;
            buttons[i].draw(canvas, paint, apaint, tpaint);
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

        Log.d("YabausePad","action:" + action);

        int hitsize = 30;

        Rect hittest = new Rect( (int)(posx - hitsize*metrics.density), (int)(posy - hitsize*metrics.density), (int)(posx+hitsize*metrics.density), (int)(posy+hitsize*metrics.density) );


        if ((action == event.ACTION_DOWN) || (action == event.ACTION_POINTER_DOWN) || (action == event.ACTION_MOVE) ) {
            for(int i = 0;i < PadEvent.BUTTON_LAST;i++) {
                if (buttons[i].intersects(hittest)) {
                    //active.put(index, i);
                    //pe = new PadEvent(action, i);
                    YabauseRunnable.press(i);
                    buttons[i].On(index);
                    invalidate();
                    Log.d("YabausePad","On:"+i);
                }else if( buttons[i].isOn(index) ){
                  YabauseRunnable.release(i);
                  buttons[i].Off();
                  invalidate();
                  Log.d("YabausePad","Off:"+i);
                }
            }
        }

        if ( ((action == event.ACTION_UP) || (action == event.ACTION_POINTER_UP))) {
          for(int i = 0;i < PadEvent.BUTTON_LAST;i++) {
              if( buttons[i].isOn(index) ){
                buttons[i].Off();
                YabauseRunnable.release(i);
                invalidate();
                Log.d("YabausePad","Off:"+i);
              }
          }
        }


        //if (((action == event.ACTION_UP) || (action == event.ACTION_POINTER_UP)) && active.containsKey(index)) {
        //    int i = active.remove(index);
        //    pe = new PadEvent(action, i);
        //}

        if ((listener != null) && (pe != null)) {
            invalidate();
            listener.onPad(pe);
            return true;
        }

        return true;
    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);

	float density = metrics.density;

        buttons[PadEvent.BUTTON_UP].updateRect((int)(100*density), (int)(getHeight() - 210*density), (int)(140*density), (int)(getHeight() - 150*density));
        buttons[PadEvent.BUTTON_RIGHT].updateRect((int)(150*density), (int)(getHeight() - 140*density), (int)(210*density), (int)(getHeight() - 100*density));
        buttons[PadEvent.BUTTON_DOWN].updateRect((int)(100*density), (int)(getHeight() - 90*density),  (int)(140*density), (int)(getHeight() - 30*density));
        buttons[PadEvent.BUTTON_LEFT].updateRect((int)(30*density),  (int)(getHeight() - 140*density), (int)(90*density),  (int)(getHeight() - 100*density));

        buttons[PadEvent.BUTTON_START].updateRect((int)(getWidth() / 2 - 40*density), (int)(getHeight() - 60*density), (int)(getWidth() / 2 + 40*density), (int)(getHeight() - 15*density));

        buttons[PadEvent.BUTTON_A].updateRect((int)(getWidth() - 235*density), (int)(getHeight() - 75*density), (int)(getWidth() - 185*density), (int)(getHeight() - 25*density));
        buttons[PadEvent.BUTTON_B].updateRect((int)(getWidth() - 165*density), (int)(getHeight() - 125*density), (int)(getWidth() - 115*density), (int)(getHeight() - 75*density));
        buttons[PadEvent.BUTTON_C].updateRect((int)(getWidth() - 75*density), (int)(getHeight() - 155*density), (int)(getWidth() - 25*density), (int)(getHeight() - 105*density));

        buttons[PadEvent.BUTTON_X].updateRect((int)(getWidth() - 280*density), (int)(getHeight() - 140*density), (int)(getWidth() - 240*density), (int)(getHeight() - 100*density));
        buttons[PadEvent.BUTTON_Y].updateRect((int)(getWidth() - 210*density), (int)(getHeight() - 190*density), (int)(getWidth() - 170*density), (int)(getHeight() - 150*density));
        buttons[PadEvent.BUTTON_Z].updateRect((int)(getWidth() - 120*density), (int)(getHeight() - 220*density), (int)(getWidth() - 80*density), (int)(getHeight() - 180*density));

        buttons[PadEvent.BUTTON_LEFT_TRIGGER].updateRect((int)(30*density), (int)(getHeight() - 270*density), (int)(90*density), (int)(getHeight() - 230*density));;
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER].updateRect((int)(getWidth() - 90*density), (int)(getHeight() - 270*density), (int)(getWidth() - 30*density), (int)(getHeight() - 150*density));;

        setMeasuredDimension(width, height);
    }
}
