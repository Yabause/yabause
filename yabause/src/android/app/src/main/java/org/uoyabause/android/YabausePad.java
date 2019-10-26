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


package org.uoyabause.android;

import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.AttributeSet;

import java.util.HashMap;

import org.uoyabause.uranus.R;

class PadButton {
    protected RectF rect;
    protected int isOn;
    protected int pointid_;
    Paint back;
    float scale;

    PadButton() {
        pointid_ = -1;
        isOn = -1;
        rect = new RectF();
        scale = 1.0f;
    }

    public void updateRect(int x1, int y1, int x2, int y2) {
        rect.set(x1, y1, x2, y2);
    }

    public void updateRect(Matrix matrix, int x1, int y1, int x2, int y2) {
        rect.set(x1, y1, x2, y2);
        matrix.mapRect(rect);
    }
    
    public void updateScale( float scale ){
    	this.scale = scale;
    }
    
    public boolean contains(int x, int y ) {
        return rect.contains(x, y);
    }

    public boolean intersects( RectF r ){
      return RectF.intersects(rect,r);
    }

    public void draw(Canvas canvas, Paint nomal_back, Paint active_back, Paint front) {
      if( isOn != -1 ){
        back = active_back;
      }else{
        back = nomal_back;
      }
    }

    void On( int index  ){
        isOn = index;
    }

    void Off(){
        isOn = -1;
        pointid_ = -1;
    }

    boolean isOn( int index ){
      if( isOn == index ){
        return true;
      }else{
        return false;
      }
    }

    boolean isOn( ){
        if( isOn != -1 ){
            return true;
        }else{
            return false;
        }
    }

    int getPointId(){
        return isOn;
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
        canvas.drawCircle(rect.centerX(), rect.centerY(), width * this.scale, back);
        //front.setTextSize(textsize);
        //front.setTextAlign(Paint.Align.CENTER);
        //canvas.drawText(text, rect.centerX() , rect.centerY() , front);
    }
}

class AnalogPad extends PadButton {
    private int width;
    private String text;
    private int textsize;
    private Paint paint = new Paint();

    AnalogPad(int w, String t, int ts) {
        super();
        width = w;
        text = t;
        textsize = ts;
        paint.setARGB(0x80, 0x80, 0x80, 0x80);
    }

    int getXvalue( int posx ){
       float xv =  ((float)(posx-rect.centerX())/((width * this.scale)/2) * 128 + 128);
        if( xv > 255 ) xv = 255;
        if( xv < 0 ) xv = 0;
        return (int)xv;
    }

    int getYvalue( int posy ){
        float yv =  ((float)(posy-rect.centerY())/((width * this.scale)/2) * 128 + 128);
        if( yv > 255 ) yv = 255;
        if( yv < 0 ) yv = 0;
        return (int)yv;
    }

    public void draw(Canvas canvas, int sx, int sy, Paint nomal_back, Paint active_back, Paint front) {
        super.draw(canvas,nomal_back,active_back,front);
        //canvas.drawCircle(rect.centerX(), rect.centerY(), width * this.scale, back);
        //front.setTextSize(textsize);
        //front.setTextAlign(Paint.Align.CENTER);
        //canvas.drawText(text, rect.centerX() , rect.centerY() , front);
        double dx = ((sx-128.0) /128.0) * ((width * this.scale)/2);
        double dy = ((sy-128.0) /128.0) * ((width * this.scale)/2);
        canvas.drawCircle(rect.centerX() + (int)dx, rect.centerY() + (int)dy, (width * this.scale/2), paint);

    }
}


public class YabausePad extends View implements OnTouchListener {

  interface OnPadListener {
    boolean onPad(PadEvent event);
  }

  private PadButton buttons[];
    private OnPadListener listener = null;
    private HashMap<Integer, Integer> active;
   // private DisplayMetrics metrics = null;

  int width_;
  int height_;

  Bitmap bitmap_pad_left = null;
    Bitmap bitmap_pad_right = null;
    Bitmap bitmap_pad_middle = null;
    
    private Paint mPaint = new Paint();
    private Matrix matrix_left = new Matrix();
    private Matrix matrix_right = new Matrix();
    private Matrix matrix_center = new Matrix();
    private Paint paint = new Paint();
    private Paint apaint = new Paint();
    private Paint tpaint = new Paint();
    
    private float base_scale = 1.0f; 
    final float basewidth = 1920.0f;
    final float baseheight = 1080.0f;
    private float wscale; 
    private float hscale;  
    boolean testmode = false;
    private String status;
    private AnalogPad _analog_pad;
    private int _axi_x = 128;
    private int _axi_y = 128;
    private int _pad_mode = 0;
    private float _transparent = 1.0f;

    public void setPadMode( int mode ){
        _pad_mode = mode;
        invalidate();
    }

    public void show( boolean b ){
        if( b==false ){
            bitmap_pad_left = null;
            bitmap_pad_right= null;
        }else{
            bitmap_pad_left = BitmapFactory.decodeResource(getResources(), R.drawable.pad_l);
            bitmap_pad_right= BitmapFactory.decodeResource(getResources(), R.drawable.pad_r);
            bitmap_pad_middle= BitmapFactory.decodeResource(getResources(), R.drawable.pad_m);
        }
        invalidate();
    }
    
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
    
    public void setScale( float scale ){
    	this.base_scale = scale;
    }
    
    public float getScale(){
    	return base_scale;
    }

    public void setTrans( float scale ){
        this._transparent = scale;
    }

    public float getTrans(){
        return _transparent;
    }


    public void setTestmode( boolean test ){
    	this.testmode = test;
    }
    
    public String getStatusString(){
    	return status;
    }

    public void updateScale(){
      SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getContext());
      base_scale= sharedPref.getFloat("pref_pad_scale", 0.75f);
      _transparent = sharedPref.getFloat("pref_pad_trans",1.0f);
      //setPadScale( width_, height_ );
      this.requestLayout();
      this.invalidate();
    }

    private void init() {

        setOnTouchListener(this);

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getContext());
        base_scale= sharedPref.getFloat("pref_pad_scale", 0.75f);
        _transparent = sharedPref.getFloat("pref_pad_trans",1.0f);

        buttons = new PadButton[PadEvent.BUTTON_LAST];
        buttons[PadEvent.BUTTON_UP]    = new DPadButton();
        buttons[PadEvent.BUTTON_RIGHT] = new DPadButton();
        buttons[PadEvent.BUTTON_DOWN]  = new DPadButton();
        buttons[PadEvent.BUTTON_LEFT]  = new DPadButton();
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER] = new DPadButton();
        buttons[PadEvent.BUTTON_LEFT_TRIGGER]  = new DPadButton();
        buttons[PadEvent.BUTTON_START] = new StartButton();
        buttons[PadEvent.BUTTON_A] = new ActionButton((int)(100), "", 40);
        buttons[PadEvent.BUTTON_B] = new ActionButton((int)(100), "", 40);
        buttons[PadEvent.BUTTON_C] = new ActionButton((int)(100), "", 40);
        buttons[PadEvent.BUTTON_X] = new ActionButton((int)(72), "", 25);
        buttons[PadEvent.BUTTON_Y] = new ActionButton((int)(72), "", 25);
        buttons[PadEvent.BUTTON_Z] = new ActionButton((int)(72), "", 25);

        _analog_pad = new AnalogPad((int)(256), "", 40);

        active = new HashMap<Integer, Integer>();

    }

    @Override protected void onAttachedToWindow (){
        paint.setARGB(0xFF, 0, 0, 0xFF);
        apaint.setARGB(0xFF, 0xFF, 0x00, 0x00);
        tpaint.setARGB(0x80, 0xFF, 0xFF, 0xFF);
        //bitmap_pad_left = BitmapFactory.decodeResource(getResources(), R.drawable.pad_l);
        //bitmap_pad_right= BitmapFactory.decodeResource(getResources(), R.drawable.pad_r);
        mPaint.setAntiAlias(true);
        mPaint.setFilterBitmap(true);
        mPaint.setDither(true);
        super.onAttachedToWindow();
    }

    @Override public void onDraw(Canvas canvas) {

        if( bitmap_pad_left == null || bitmap_pad_right == null ){
            return;
        }

        mPaint.setAlpha( (int)(255.0f*_transparent) );

        canvas.drawBitmap(bitmap_pad_left, matrix_left, mPaint);
        canvas.drawBitmap(bitmap_pad_right, matrix_right, mPaint);
        canvas.drawBitmap(bitmap_pad_middle, matrix_center, mPaint);
        
        canvas.setMatrix(null);
/*
        canvas.save();
    	canvas.concat(matrix_left);
        buttons[PadEvent.BUTTON_UP].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_DOWN].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_LEFT].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_RIGHT].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_START].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_LEFT_TRIGGER].draw(canvas, paint, apaint, tpaint);
        
        canvas.restore();
        canvas.concat(matrix_right);
        buttons[PadEvent.BUTTON_A].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_B].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_C].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_X].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_Y].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_Z].draw(canvas, paint, apaint, tpaint);
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER].draw(canvas, paint, apaint, tpaint);
*/
        if(_pad_mode==1) {
            _analog_pad.draw(canvas, _axi_x, _axi_y, paint, apaint, tpaint);
        }
    }

    public void setOnPadListener(OnPadListener listener) {
        this.listener = listener;
    }

    private void updatePad(RectF hittest, int posx, int posy, int pointerId){
        if (_analog_pad.intersects(hittest)) {
            _analog_pad.On(pointerId);
            _axi_x = _analog_pad.getXvalue(posx);
            _axi_y = _analog_pad.getYvalue(posy);
            invalidate();
            if (!testmode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x);
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y);
            }
        } else if (_analog_pad.isOn(pointerId)) {

            _axi_x = _analog_pad.getXvalue(posx);
            _axi_y = _analog_pad.getYvalue(posy);

            //_analog_pad.Off();
            //_axi_x = 128;
            //_axi_y = 128;

            invalidate();
            if (!testmode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x);
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y);
            }
        }
    }



    private void releasePad(int pointerId) {
        if (_analog_pad.isOn(pointerId)) {
            _analog_pad.Off();
            _axi_x = 128;
            _axi_y = 128;
            if (!testmode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x);
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y);
            }
            invalidate();
        }
    }



    public boolean onTouch(View v, MotionEvent event) {

        int action = event.getActionMasked();
        int touchCount = event.getPointerCount();

        int pointerIndex = event.getActionIndex();
        int pointerId = event.getPointerId(pointerIndex);
        int posx = (int) event.getX(pointerIndex);
        int posy = (int) event.getY(pointerIndex);

        float hitsize = 15.0f * wscale * base_scale;
        RectF hittest = new RectF((int) (posx - hitsize), (int) (posy - hitsize), (int) (posx + hitsize), (int) (posy + hitsize));


        switch (action) {
            case MotionEvent.ACTION_DOWN:
                for (int btnindex = 0; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if( buttons[btnindex].intersects(hittest) ) {
                        buttons[btnindex].On(pointerId);
                    }
                }

                if (_pad_mode == 1) {
                    updatePad(hittest, posx, posy, pointerId);
                }

                break;

            case MotionEvent.ACTION_POINTER_DOWN:
                for (int btnindex = 0; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if( buttons[btnindex].intersects(hittest) ) {
                        buttons[btnindex].On(pointerId);
                    }
                }
                if (_pad_mode == 1) {
                    updatePad(hittest, posx, posy, pointerId);
                }

                break;

            case MotionEvent.ACTION_POINTER_UP:
                for (int btnindex = 0; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if ( buttons[btnindex].isOn() && buttons[btnindex].getPointId() == pointerId)  {
                        buttons[btnindex].Off();
                    }
                }

                if (_pad_mode == 1) {
                    releasePad(pointerId);
                }

                break;

            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
                for (int btnindex = 0; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if ( buttons[btnindex].isOn() && buttons[btnindex].getPointId() == pointerId)  {
                        buttons[btnindex].Off();
                    }
                }
                if (_pad_mode == 1) {
                    releasePad(pointerId);
                }

                break;

            case MotionEvent.ACTION_MOVE:

                for(int index = 0; index < touchCount; index++) {

                    int eventID2 = event.getPointerId(index);
                    float x2 =event.getX(index);
                    float y2 =event.getY(index);

                    RectF hittest2 = new RectF((int) (x2 - hitsize), (int) (y2 - hitsize), (int) (x2 + hitsize), (int) (y2 + hitsize));

                    if( buttons[PadEvent.BUTTON_DOWN].isOn() && eventID2 ==  buttons[PadEvent.BUTTON_DOWN].getPointId() ){
                        int val  = _analog_pad.getYvalue((int)y2);
                        if( val < (128+10) ){
                            buttons[PadEvent.BUTTON_DOWN].Off();
                        }
                    }

                    if( buttons[PadEvent.BUTTON_UP].isOn() && eventID2 ==  buttons[PadEvent.BUTTON_UP].getPointId() ){
                        int val  = _analog_pad.getYvalue((int)y2);
                        if( val > (128-10) ){
                            buttons[PadEvent.BUTTON_UP].Off();
                        }
                    }

                    if( buttons[PadEvent.BUTTON_RIGHT].isOn() && eventID2 ==  buttons[PadEvent.BUTTON_RIGHT].getPointId() ){
                        int val  = _analog_pad.getXvalue((int)x2);
                        if( val < (128+10) ){
                            buttons[PadEvent.BUTTON_RIGHT].Off();
                        }
                    }

                    if( buttons[PadEvent.BUTTON_LEFT].isOn() && eventID2 ==  buttons[PadEvent.BUTTON_LEFT].getPointId() ){
                        int val  = _analog_pad.getXvalue((int)x2);
                        if( val > (128-10) ){
                            buttons[PadEvent.BUTTON_LEFT].Off();
                        }
                    }

                    if( buttons[PadEvent.BUTTON_UP].intersects(hittest2) ){
//                        if( buttons[PadEvent.BUTTON_DOWN].isOn() ){
//                            buttons[PadEvent.BUTTON_DOWN].Off();
//                        }
                        buttons[PadEvent.BUTTON_UP].On(eventID2);
                    }

                    if( buttons[PadEvent.BUTTON_DOWN].intersects(hittest2) ){
//                        if( buttons[PadEvent.BUTTON_UP].isOn() ){
//                            buttons[PadEvent.BUTTON_UP].Off();
//                        }
                        buttons[PadEvent.BUTTON_DOWN].On(eventID2);
                    }

                    if( buttons[PadEvent.BUTTON_LEFT].intersects(hittest2) ){
//                        if( buttons[PadEvent.BUTTON_RIGHT].isOn() ){
//                            buttons[PadEvent.BUTTON_RIGHT].Off();
//                        }
                        buttons[PadEvent.BUTTON_LEFT].On(eventID2);
                    }

                    if( buttons[PadEvent.BUTTON_RIGHT].intersects(hittest2) ){
//                        if( buttons[PadEvent.BUTTON_LEFT].isOn() ){
//                            buttons[PadEvent.BUTTON_LEFT].Off();
//                        }
                        buttons[PadEvent.BUTTON_RIGHT].On(eventID2);
                    }

                    for (int btnindex = PadEvent.BUTTON_RIGHT_TRIGGER; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                        if( eventID2 ==  buttons[btnindex].getPointId() ){
                            if(  buttons[btnindex].intersects(hittest2) == false ){
                                buttons[btnindex].Off();
                            }
                        } else if(  buttons[btnindex].intersects(hittest2) ) {
                            buttons[btnindex].On(eventID2);
                        }
                    }

                    if (_pad_mode == 1) {
                        updatePad(hittest2, (int)x2, (int)y2, eventID2);
                    }
               }
               break;
        }

        if (!testmode) {

            if (_pad_mode == 0) {

                for (int btnindex = 0; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if (buttons[btnindex].isOn()) {
                        YabauseRunnable.press(btnindex, 0);
                    } else {
                        YabauseRunnable.release(btnindex, 0);
                    }
                }
            }else{
                for (int btnindex = PadEvent.BUTTON_RIGHT_TRIGGER; btnindex < PadEvent.BUTTON_LAST; btnindex++) {
                    if (buttons[btnindex].isOn()) {
                        YabauseRunnable.press(btnindex, 0);
                    } else {
                        YabauseRunnable.release(btnindex, 0);
                    }
                }
            }
        }


        if( testmode ){
        	status = "";
            status += "START:";
            if( buttons[PadEvent.BUTTON_START].isOn() ) status += "ON "; else status += "OFF ";

            status += "\nUP:";
        	if( buttons[PadEvent.BUTTON_UP].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "DOWN:";
        	if( buttons[PadEvent.BUTTON_DOWN].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "LEFT:";
        	if( buttons[PadEvent.BUTTON_LEFT].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "RIGHT:";
        	if( buttons[PadEvent.BUTTON_RIGHT].isOn() ) status += "ON "; else status += "OFF "; 

        	status += "\nA:";
        	if( buttons[PadEvent.BUTTON_A].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "B:";
        	if( buttons[PadEvent.BUTTON_B].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "C:";
        	if( buttons[PadEvent.BUTTON_C].isOn() ) status += "ON "; else status += "OFF "; 
        	
        	status += "\nX:";
        	if( buttons[PadEvent.BUTTON_X].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "Y:";
        	if( buttons[PadEvent.BUTTON_Y].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "Z:";
        	if( buttons[PadEvent.BUTTON_Z].isOn() ) status += "ON "; else status += "OFF "; 
        	
        	status += "\nLT:";
        	if( buttons[PadEvent.BUTTON_LEFT_TRIGGER].isOn() ) status += "ON "; else status += "OFF "; 
        	status += "RT:";
        	if( buttons[PadEvent.BUTTON_RIGHT_TRIGGER].isOn() ) status += "ON "; else status += "OFF ";

            status += "\nAX:";
            if( _analog_pad.isOn() ) status += "ON " + _axi_x ; else status += "OFF " + _axi_x;
            status += "AY:";
            if( _analog_pad.isOn() ) status += "ON " + _axi_y; else status += "OFF " + _axi_y;

        }
        if ((listener != null) ) {
             listener.onPad(null);
        }
        return true;
    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

      if (bitmap_pad_left == null || bitmap_pad_right == null) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        return;
      }

      width_ = MeasureSpec.getSize(widthMeasureSpec);
      height_ = MeasureSpec.getSize(heightMeasureSpec);
      setPadScale( width_, height_);
    }

    void setPadScale( int width, int height ){

        float dens = getResources().getDisplayMetrics().density;
        dens /= 2.0;


        if (width > height) {
            wscale = (float) width / basewidth;
            hscale = (float) height / baseheight;
        }else{
            wscale = (float) width / baseheight;
            hscale = (float) height / basewidth;
        }

        int bitmap_height = bitmap_pad_right.getHeight();
    	
        matrix_right.reset();
        matrix_right.postTranslate(-780, -baseheight);
        matrix_right.postScale(base_scale*wscale, base_scale*hscale);
        matrix_right.postTranslate(width, height);
        
        matrix_left.reset();
        matrix_left.postTranslate(0, -baseheight);
        matrix_left.postScale(base_scale*wscale, base_scale*hscale);
        matrix_left.postTranslate(0, height);

        matrix_center.reset();
        matrix_center.postTranslate(-bitmap_pad_middle.getWidth(), -bitmap_pad_middle.getHeight());
        matrix_center.postScale(base_scale*wscale, base_scale*hscale);
        matrix_center.postTranslate(width/2, height);

        // Left Part
        _analog_pad.updateRect(matrix_left, 130, 512, 420+144,533+378);
        _analog_pad.updateScale(base_scale*wscale);

        //buttons[PadEvent.BUTTON_UP].updateRect(matrix_left, 303, 497, 303+ 89,497+180);
        buttons[PadEvent.BUTTON_UP].updateRect(matrix_left, 130, 512, 130+ 429,512+151);
        //buttons[PadEvent.BUTTON_DOWN].updateRect(matrix_left,303,752,303+89,752+180);
        buttons[PadEvent.BUTTON_DOWN].updateRect(matrix_left,130,784,130+429,784+151);        
        //buttons[PadEvent.BUTTON_RIGHT].updateRect(matrix_left,392,671,392+162,671+93);
        buttons[PadEvent.BUTTON_RIGHT].updateRect(matrix_left,436,533,436+128,533+378);
        //buttons[PadEvent.BUTTON_LEFT].updateRect(matrix_left,141,671,141+162,671+93);
        buttons[PadEvent.BUTTON_LEFT].updateRect(matrix_left,148,533,148+128,533+378);

        buttons[PadEvent.BUTTON_LEFT_TRIGGER].updateRect(matrix_left,56,57,56+376,57+92);
        //buttons[PadEvent.BUTTON_START].updateRect(matrix_left,510,1013,510+182,1013+57);

        buttons[PadEvent.BUTTON_START].updateRect(matrix_center,0,57,bitmap_pad_middle.getWidth(),bitmap_pad_middle.getHeight());

        // Right Part
        buttons[PadEvent.BUTTON_A].updateRect(matrix_right,59,801,59+213,801+225);
        buttons[PadEvent.BUTTON_A].updateScale(base_scale*wscale);
        buttons[PadEvent.BUTTON_B].updateRect(matrix_right,268,672,268+229,672+221);
        buttons[PadEvent.BUTTON_B].updateScale(base_scale*wscale);        
        buttons[PadEvent.BUTTON_C].updateRect(matrix_right,507,577,507+224,577+229);
        buttons[PadEvent.BUTTON_C].updateScale(base_scale*wscale);
        buttons[PadEvent.BUTTON_X].updateRect(matrix_right,15,602,15+149,602+150);
        buttons[PadEvent.BUTTON_X].updateScale(base_scale*wscale);
        buttons[PadEvent.BUTTON_Y].updateRect(matrix_right,202,481,202+149,481+148);
        buttons[PadEvent.BUTTON_Y].updateScale(base_scale*wscale);
        buttons[PadEvent.BUTTON_Z].updateRect(matrix_right,397,409,397+151,409+152);
        buttons[PadEvent.BUTTON_Z].updateScale(base_scale*wscale);
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER].updateRect(matrix_right,350,59,350+379,59+91);
        
        matrix_right.reset();
        matrix_right.postTranslate(- bitmap_pad_right.getWidth(), - bitmap_pad_right.getHeight());
        matrix_right.postScale(base_scale*wscale/dens, base_scale*hscale/dens);
        matrix_right.postTranslate(width, height);
        
        matrix_left.reset();
        matrix_left.postTranslate(0, - bitmap_pad_right.getHeight());
        matrix_left.postScale(base_scale*wscale/dens, base_scale*hscale/dens);
        matrix_left.postTranslate(0, height);

        matrix_center.reset();
        matrix_center.postTranslate(-bitmap_pad_middle.getWidth(), -bitmap_pad_middle.getHeight());
        matrix_center.postScale(base_scale*wscale/dens, base_scale*hscale/dens);
        matrix_center.postTranslate(width/2, height);
      setMeasuredDimension(width, height);
    }
}


