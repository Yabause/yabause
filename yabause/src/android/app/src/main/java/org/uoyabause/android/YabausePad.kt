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

import android.content.Context
import android.graphics.RectF
import android.view.View.OnTouchListener
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import org.devmiyax.yabasanshiro.R
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Paint
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.preference.PreferenceManager
import java.util.HashMap

internal open class PadButton {
    protected var rect: RectF
    var pointId: Int
        protected set
    protected var pointid_: Int
    var back: Paint? = null
    var scale: Float
    fun updateRect(x1: Int, y1: Int, x2: Int, y2: Int) {
        rect[x1.toFloat(), y1.toFloat(), x2.toFloat()] = y2.toFloat()
    }

    fun updateRect(matrix: Matrix, x1: Int, y1: Int, x2: Int, y2: Int) {
        rect[x1.toFloat(), y1.toFloat(), x2.toFloat()] = y2.toFloat()
        matrix.mapRect(rect)
    }

    fun updateScale(scale: Float) {
        this.scale = scale
    }

    fun contains(x: Int, y: Int): Boolean {
        return rect.contains(x.toFloat(), y.toFloat())
    }

    fun intersects(r: RectF?): Boolean {
        return RectF.intersects(rect, r!!)
    }

    open fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        back = if (pointId != -1) {
            active_back
        } else {
            nomal_back
        }
    }

    fun On(index: Int) {
        pointId = index
    }

    fun Off() {
        pointId = -1
        pointid_ = -1
    }

    fun isOn(index: Int): Boolean {
        return if (pointId == index) {
            true
        } else {
            false
        }
    }

    fun isOn(): Boolean {
        return if (pointId != -1) {
            true
        } else {
            false
        }
    }

    init {
        pointid_ = -1
        pointId = -1
        rect = RectF()
        scale = 1.0f
    }
}

internal class DPadButton : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        canvas.drawRect(rect, back!!)
    }
}

internal class StartButton : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        canvas.drawOval(RectF(rect), back!!)
    }
}

internal class ActionButton(
    private val width: Int,
    private val text: String,
    private val textsize: Int
) : PadButton() {
    override fun draw(canvas: Canvas, nomal_back: Paint?, active_back: Paint?, front: Paint?) {
        super.draw(canvas, nomal_back, active_back, front)
        canvas.drawCircle(rect.centerX(), rect.centerY(), width * scale, back!!)
        //front.setTextSize(textsize);
        //front.setTextAlign(Paint.Align.CENTER);
        //canvas.drawText(text, rect.centerX() , rect.centerY() , front);
    }
}

internal class AnalogPad(
    private val width: Int,
    private val text: String,
    private val textsize: Int
) : PadButton() {
    private val paint = Paint()
    fun getXvalue(posx: Int): Int {
        var xv = (posx - rect.centerX()) / (width * scale / 2) * 128 + 128
        if (xv > 255) xv = 255f
        if (xv < 0) xv = 0f
        return xv.toInt()
    }

    fun getYvalue(posy: Int): Int {
        var yv = (posy - rect.centerY()) / (width * scale / 2) * 128 + 128
        if (yv > 255) yv = 255f
        if (yv < 0) yv = 0f
        return yv.toInt()
    }

    fun draw(
        canvas: Canvas,
        sx: Int,
        sy: Int,
        nomal_back: Paint?,
        active_back: Paint?,
        front: Paint?
    ) {
        super.draw(canvas, nomal_back, active_back, front)
        //canvas.drawCircle(rect.centerX(), rect.centerY(), width * this.scale, back);
        //front.setTextSize(textsize);
        //front.setTextAlign(Paint.Align.CENTER);
        //canvas.drawText(text, rect.centerX() , rect.centerY() , front);
        val dx = (sx - 128.0) / 128.0 * (width * scale / 2)
        val dy = (sy - 128.0) / 128.0 * (width * scale / 2)
        canvas.drawCircle(rect.centerX() + dx.toInt(),
            rect.centerY() + dy.toInt(),
            width * scale / 2,
            paint)
    }

    init {
        paint.setARGB(0x80, 0x80, 0x80, 0x80)
    }
}

class YabausePad : View, OnTouchListener {
    interface OnPadListener {
        fun onPad(event: PadEvent?): Boolean
    }

    private lateinit var buttons: Array<PadButton?>
    private var listener: OnPadListener? = null
    private var active: HashMap<Int, Int>? = null

    // private DisplayMetrics metrics = null;
    var width_ = 0
    var height_ = 0
    var bitmap_pad_left: Bitmap? = null
    var bitmap_pad_right: Bitmap? = null
    var bitmap_pad_middle: Bitmap? = null
    private val mPaint = Paint()
    private val matrix_left = Matrix()
    private val matrix_right = Matrix()
    private val matrix_center = Matrix()
    private val paint = Paint()
    private val apaint = Paint()
    private val tpaint = Paint()
    var scale = 1.0f
    var ypos = 0.0f
    val basewidth = 1920.0f
    val baseheight = 1080.0f
    private var wscale = 0f
    private var hscale = 0f
    var padTestestMode: Boolean = false
    var statusString: String? = null
        private set
    private var _analog_pad: AnalogPad? = null
    private var _axi_x = 128
    private var _axi_y = 128
    private var _pad_mode = 0
    var trans = 1.0f
    fun setPadMode(mode: Int) {
        _pad_mode = mode
        invalidate()
    }

    fun show(b: Boolean) {
        if (b == false) {
            bitmap_pad_left = null
            bitmap_pad_right = null
        } else {
            bitmap_pad_left = BitmapFactory.decodeResource(resources, R.drawable.pad_l)
            bitmap_pad_right = BitmapFactory.decodeResource(resources, R.drawable.pad_r)
            bitmap_pad_middle = BitmapFactory.decodeResource(resources, R.drawable.pad_m)
        }
        invalidate()
    }

    constructor(context: Context?) : super(context) {
        init()
    }

    constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {
        init()
    }

    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(context,
        attrs,
        defStyle) {
        init()
    }

    fun setTestmode(test: Boolean) {
        padTestestMode = test
    }

    fun updateScale() {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(
            context)
        scale = sharedPref.getFloat("pref_pad_scale", 0.75f)
        ypos = sharedPref.getFloat("pref_pad_pos", 0.1f)
        trans = sharedPref.getFloat("pref_pad_trans", 0.7f)
        //setPadScale( width_, height_ );
        requestLayout()
        this.invalidate()
    }

    private fun init() {
        setOnTouchListener(this)
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(
            context)
        scale = sharedPref.getFloat("pref_pad_scale", 0.75f)
        ypos = sharedPref.getFloat("pref_pad_pos", 0.1f)
        trans = sharedPref.getFloat("pref_pad_trans", 0.7f)
        buttons = arrayOfNulls(PadEvent.BUTTON_LAST)
        buttons[PadEvent.BUTTON_UP] = DPadButton()
        buttons[PadEvent.BUTTON_RIGHT] = DPadButton()
        buttons[PadEvent.BUTTON_DOWN] = DPadButton()
        buttons[PadEvent.BUTTON_LEFT] = DPadButton()
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER] = DPadButton()
        buttons[PadEvent.BUTTON_LEFT_TRIGGER] = DPadButton()
        buttons[PadEvent.BUTTON_START] = StartButton()
        buttons[PadEvent.BUTTON_A] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_B] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_C] = ActionButton(100, "", 40)
        buttons[PadEvent.BUTTON_X] = ActionButton(72, "", 25)
        buttons[PadEvent.BUTTON_Y] = ActionButton(72, "", 25)
        buttons[PadEvent.BUTTON_Z] = ActionButton(72, "", 25)
        _analog_pad = AnalogPad(256, "", 40)
        active = HashMap()
    }

    override fun onAttachedToWindow() {
        paint.setARGB(0xFF, 0, 0, 0xFF)
        apaint.setARGB(0xFF, 0xFF, 0x00, 0x00)
        tpaint.setARGB(0x80, 0xFF, 0xFF, 0xFF)
        //bitmap_pad_left = BitmapFactory.decodeResource(getResources(), R.drawable.pad_l);
        //bitmap_pad_right= BitmapFactory.decodeResource(getResources(), R.drawable.pad_r);
        mPaint.isAntiAlias = true
        mPaint.isFilterBitmap = true
        mPaint.isDither = true
        super.onAttachedToWindow()
    }

    public override fun onDraw(canvas: Canvas) {
        if (bitmap_pad_left == null || bitmap_pad_right == null) {
            return
        }
        mPaint.alpha = (255.0f * trans).toInt()
        canvas.drawBitmap(bitmap_pad_left!!, matrix_left, mPaint)
        canvas.drawBitmap(bitmap_pad_right!!, matrix_right, mPaint)
        canvas.drawBitmap(bitmap_pad_middle!!, matrix_center, mPaint)
        canvas.setMatrix(null)
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
*/if (_pad_mode == 1) {
            _analog_pad!!.draw(canvas, _axi_x, _axi_y, paint, apaint, tpaint)
        }
    }

    fun setOnPadListener(listener: OnPadListener?) {
        this.listener = listener
    }

    private fun updatePad(hittest: RectF, posx: Int, posy: Int, pointerId: Int) {
        if (_analog_pad!!.intersects(hittest)) {
            _analog_pad!!.On(pointerId)
            _axi_x = _analog_pad!!.getXvalue(posx)
            _axi_y = _analog_pad!!.getYvalue(posy)
            invalidate()
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
        } else if (_analog_pad!!.isOn(pointerId)) {
            _axi_x = _analog_pad!!.getXvalue(posx)
            _axi_y = _analog_pad!!.getYvalue(posy)

            //_analog_pad.Off();
            //_axi_x = 128;
            //_axi_y = 128;
            invalidate()
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
        }
    }

    private fun releasePad(pointerId: Int) {
        if (_analog_pad!!.isOn(pointerId)) {
            _analog_pad!!.Off()
            _axi_x = 128
            _axi_y = 128
            if (!padTestestMode) {
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_X, 0, _axi_x)
                YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_Y, 0, _axi_y)
            }
            invalidate()
        }
    }

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        val action = event.actionMasked
        val touchCount = event.pointerCount
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)
        val posx = event.getX(pointerIndex).toInt()
        val posy = event.getY(pointerIndex).toInt()
        val hitsize = 15.0f * wscale * scale
        val hittest = RectF((posx - hitsize).toFloat(),
            (posy - hitsize).toFloat(),
            (posx + hitsize).toFloat(),
            (posy + hitsize).toFloat())
        when (action) {
            MotionEvent.ACTION_DOWN -> {
                var btnindex = 0
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.intersects(hittest)) {
                        buttons[btnindex]!!.On(pointerId)
                    }
                    btnindex++
                }
                if (_pad_mode == 1) {
                    updatePad(hittest, posx, posy, pointerId)
                }
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                var btnindex = 0
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.intersects(hittest)) {
                        buttons[btnindex]!!.On(pointerId)
                    }
                    btnindex++
                }
                if (_pad_mode == 1) {
                    updatePad(hittest, posx, posy, pointerId)
                }
            }
            MotionEvent.ACTION_POINTER_UP -> {
                var btnindex = 0
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn() && buttons[btnindex]!!.pointId == pointerId) {
                        buttons[btnindex]!!.Off()
                    }
                    btnindex++
                }
                if (_pad_mode == 1) {
                    releasePad(pointerId)
                }
            }
            MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_UP -> {
                var btnindex = 0
                while (btnindex < PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn() && buttons[btnindex]!!.pointId == pointerId) {
                        buttons[btnindex]!!.Off()
                    }
                    btnindex++
                }
                if (_pad_mode == 1) {
                    releasePad(pointerId)
                }
            }
            MotionEvent.ACTION_MOVE -> {
                var index = 0
                while (index < touchCount) {
                    val eventID2 = event.getPointerId(index)
                    val x2 = event.getX(index)
                    val y2 = event.getY(index)
                    val hittest2 = RectF((x2 - hitsize).toFloat(),
                        (y2 - hitsize).toFloat(),
                        (x2 + hitsize).toFloat(),
                        (y2 + hitsize).toFloat())
                    if (buttons[PadEvent.BUTTON_DOWN]!!
                            .isOn() && eventID2 == buttons[PadEvent.BUTTON_DOWN]!!.pointId
                    ) {
                        val `val` = _analog_pad!!.getYvalue(y2.toInt())
                        if (`val` < 128 + 10) {
                            buttons[PadEvent.BUTTON_DOWN]!!.Off()
                        }
                    }
                    if (buttons[PadEvent.BUTTON_UP]!!
                            .isOn() && eventID2 == buttons[PadEvent.BUTTON_UP]!!.pointId
                    ) {
                        val `val` = _analog_pad!!.getYvalue(y2.toInt())
                        if (`val` > 128 - 10) {
                            buttons[PadEvent.BUTTON_UP]!!.Off()
                        }
                    }
                    if (buttons[PadEvent.BUTTON_RIGHT]!!
                            .isOn() && eventID2 == buttons[PadEvent.BUTTON_RIGHT]!!.pointId
                    ) {
                        val `val` = _analog_pad!!.getXvalue(x2.toInt())
                        if (`val` < 128 + 10) {
                            buttons[PadEvent.BUTTON_RIGHT]!!.Off()
                        }
                    }
                    if (buttons[PadEvent.BUTTON_LEFT]!!
                            .isOn() && eventID2 == buttons[PadEvent.BUTTON_LEFT]!!.pointId
                    ) {
                        val `val` = _analog_pad!!.getXvalue(x2.toInt())
                        if (`val` > 128 - 10) {
                            buttons[PadEvent.BUTTON_LEFT]!!.Off()
                        }
                    }
                    if (buttons[PadEvent.BUTTON_UP]!!.intersects(hittest2)) {
//                        if( buttons[PadEvent.BUTTON_DOWN].isOn() ){
//                            buttons[PadEvent.BUTTON_DOWN].Off();
//                        }
                        buttons[PadEvent.BUTTON_UP]!!.On(eventID2)
                    }
                    if (buttons[PadEvent.BUTTON_DOWN]!!.intersects(hittest2)) {
//                        if( buttons[PadEvent.BUTTON_UP].isOn() ){
//                            buttons[PadEvent.BUTTON_UP].Off();
//                        }
                        buttons[PadEvent.BUTTON_DOWN]!!.On(eventID2)
                    }
                    if (buttons[PadEvent.BUTTON_LEFT]!!.intersects(hittest2)) {
//                        if( buttons[PadEvent.BUTTON_RIGHT].isOn() ){
//                            buttons[PadEvent.BUTTON_RIGHT].Off();
//                        }
                        buttons[PadEvent.BUTTON_LEFT]!!.On(eventID2)
                    }
                    if (buttons[PadEvent.BUTTON_RIGHT]!!.intersects(hittest2)) {
//                        if( buttons[PadEvent.BUTTON_LEFT].isOn() ){
//                            buttons[PadEvent.BUTTON_LEFT].Off();
//                        }
                        buttons[PadEvent.BUTTON_RIGHT]!!.On(eventID2)
                    }
                    var btnindex = PadEvent.BUTTON_RIGHT_TRIGGER
                    while (btnindex < PadEvent.BUTTON_LAST) {
                        if (eventID2 == buttons[btnindex]!!.pointId) {
                            if (buttons[btnindex]!!.intersects(hittest2) == false) {
                                buttons[btnindex]!!.Off()
                            }
                        } else if (buttons[btnindex]!!.intersects(hittest2)) {
                            buttons[btnindex]!!.On(eventID2)
                        }
                        btnindex++
                    }
                    if (_pad_mode == 1) {
                        updatePad(hittest2, x2.toInt(), y2.toInt(), eventID2)
                    }
                    index++
                }
            }
        }
        if (!padTestestMode) {
            if (_pad_mode == 0) {
                for (btnindex in 0 until PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn()) {
                        YabauseRunnable.press(btnindex, 0)
                    } else {
                        YabauseRunnable.release(btnindex, 0)
                    }
                }
            } else {
                for (btnindex in PadEvent.BUTTON_RIGHT_TRIGGER until PadEvent.BUTTON_LAST) {
                    if (buttons[btnindex]!!.isOn()) {
                        YabauseRunnable.press(btnindex, 0)
                        if (btnindex == PadEvent.BUTTON_RIGHT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_RTRIGGER, 0, 255)
                        }
                        if (btnindex == PadEvent.BUTTON_LEFT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_LTRIGGER, 0, 255)
                        }
                    } else {
                        YabauseRunnable.release(btnindex, 0)
                        if (btnindex == PadEvent.BUTTON_RIGHT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_RTRIGGER, 0, 0)
                        }
                        if (btnindex == PadEvent.BUTTON_LEFT_TRIGGER) {
                            YabauseRunnable.axis(PadEvent.PERANALOG_AXIS_LTRIGGER, 0, 0)
                        }
                    }
                }
            }
        }
        if (padTestestMode) {
            statusString = ""
            statusString += "START:"
            if (buttons[PadEvent.BUTTON_START]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nUP:"
            if (buttons[PadEvent.BUTTON_UP]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "DOWN:"
            if (buttons[PadEvent.BUTTON_DOWN]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "LEFT:"
            if (buttons[PadEvent.BUTTON_LEFT]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "RIGHT:"
            if (buttons[PadEvent.BUTTON_RIGHT]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nA:"
            if (buttons[PadEvent.BUTTON_A]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "B:"
            if (buttons[PadEvent.BUTTON_B]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "C:"
            if (buttons[PadEvent.BUTTON_C]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nX:"
            if (buttons[PadEvent.BUTTON_X]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "Y:"
            if (buttons[PadEvent.BUTTON_Y]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "Z:"
            if (buttons[PadEvent.BUTTON_Z]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nLT:"
            if (buttons[PadEvent.BUTTON_LEFT_TRIGGER]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "RT:"
            if (buttons[PadEvent.BUTTON_RIGHT_TRIGGER]!!.isOn()) statusString += "ON " else statusString += "OFF "
            statusString += "\nAX:"
            if (_analog_pad!!.isOn()) statusString += "ON $_axi_x" else statusString += "OFF $_axi_x"
            statusString += "AY:"
            if (_analog_pad!!.isOn()) statusString += "ON $_axi_y" else statusString += "OFF $_axi_y"
        }
        if (listener != null) {
            listener!!.onPad(null)
        }
        return true
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        if (bitmap_pad_left == null || bitmap_pad_right == null) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            return
        }
        width_ = MeasureSpec.getSize(widthMeasureSpec)
        height_ = MeasureSpec.getSize(heightMeasureSpec)
        setPadScale(width_, height_)
    }

    fun setPadScale(width: Int, height: Int) {
        var dens = resources.displayMetrics.density
        dens /= 2.0f
        var pos = 0.0f
        val bitmap_height = bitmap_pad_right!!.height
        if (width > height) {
            wscale = width.toFloat() / basewidth
            hscale = wscale //(float) height / baseheight;
        } else {
            wscale = width.toFloat() / baseheight
            hscale = wscale //(float) height / basewidth;
        }
        var maxpos = height - bitmap_height * scale * hscale
        if (maxpos < 0.0f) {
            maxpos = 0.0f
        }
        pos = ypos * maxpos
        matrix_right.reset()
        matrix_right.postTranslate(-780f, -baseheight)
        matrix_right.postScale(scale * wscale, scale * hscale)
        matrix_right.postTranslate(width.toFloat(), height - pos)
        matrix_left.reset()
        matrix_left.postTranslate(0f, -baseheight)
        matrix_left.postScale(scale * wscale, scale * hscale)
        matrix_left.postTranslate(0f, height - pos)
        matrix_center.reset()
        matrix_center.postTranslate(-bitmap_pad_middle!!.width.toFloat(),
            -bitmap_pad_middle!!.height.toFloat())
        matrix_center.postScale(scale * wscale, scale * hscale)
        matrix_center.postTranslate((width / 2).toFloat(), height - pos)

        // Left Part
        _analog_pad!!.updateRect(matrix_left, 130, 512, 420 + 144, 533 + 378)
        _analog_pad!!.updateScale(scale * wscale)

        //buttons[PadEvent.BUTTON_UP].updateRect(matrix_left, 303, 497, 303+ 89,497+180);
        buttons[PadEvent.BUTTON_UP]!!.updateRect(matrix_left, 130, 512, 130 + 429, 512 + 151)
        //buttons[PadEvent.BUTTON_DOWN].updateRect(matrix_left,303,752,303+89,752+180);
        buttons[PadEvent.BUTTON_DOWN]!!.updateRect(matrix_left, 130, 784, 130 + 429, 784 + 151)
        //buttons[PadEvent.BUTTON_RIGHT].updateRect(matrix_left,392,671,392+162,671+93);
        buttons[PadEvent.BUTTON_RIGHT]!!.updateRect(matrix_left, 436, 533, 436 + 128, 533 + 378)
        //buttons[PadEvent.BUTTON_LEFT].updateRect(matrix_left,141,671,141+162,671+93);
        buttons[PadEvent.BUTTON_LEFT]!!.updateRect(matrix_left, 148, 533, 148 + 128, 533 + 378)
        buttons[PadEvent.BUTTON_LEFT_TRIGGER]!!.updateRect(matrix_left, 56, 57, 56 + 376, 57 + 92)
        //buttons[PadEvent.BUTTON_START].updateRect(matrix_left,510,1013,510+182,1013+57);
        buttons[PadEvent.BUTTON_START]!!
            .updateRect(matrix_center, 0, 57, bitmap_pad_middle!!.width, bitmap_pad_middle!!.height)

        // Right Part
        buttons[PadEvent.BUTTON_A]!!.updateRect(matrix_right, 59, 801, 59 + 213, 801 + 225)
        buttons[PadEvent.BUTTON_A]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_B]!!.updateRect(matrix_right, 268, 672, 268 + 229, 672 + 221)
        buttons[PadEvent.BUTTON_B]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_C]!!.updateRect(matrix_right, 507, 577, 507 + 224, 577 + 229)
        buttons[PadEvent.BUTTON_C]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_X]!!.updateRect(matrix_right, 15, 602, 15 + 149, 602 + 150)
        buttons[PadEvent.BUTTON_X]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_Y]!!.updateRect(matrix_right, 202, 481, 202 + 149, 481 + 148)
        buttons[PadEvent.BUTTON_Y]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_Z]!!.updateRect(matrix_right, 397, 409, 397 + 151, 409 + 152)
        buttons[PadEvent.BUTTON_Z]!!.updateScale(scale * wscale)
        buttons[PadEvent.BUTTON_RIGHT_TRIGGER]!!.updateRect(matrix_right,
            350,
            59,
            350 + 379,
            59 + 91)
        matrix_right.reset()
        matrix_right.postTranslate(-bitmap_pad_right!!.width.toFloat(),
            -bitmap_pad_right!!.height.toFloat())
        matrix_right.postScale(scale * wscale / dens, scale * hscale / dens)
        matrix_right.postTranslate(width.toFloat(), height - pos)
        matrix_left.reset()
        matrix_left.postTranslate(0f, -bitmap_pad_right!!.height.toFloat())
        matrix_left.postScale(scale * wscale / dens, scale * hscale / dens)
        matrix_left.postTranslate(0f, height - pos)
        matrix_center.reset()
        matrix_center.postTranslate(-bitmap_pad_middle!!.width.toFloat(),
            -bitmap_pad_middle!!.height.toFloat())
        matrix_center.postScale(scale * wscale / dens, scale * hscale / dens)
        matrix_center.postTranslate((width / 2).toFloat(), height - pos)
        setMeasuredDimension(width, height)
    }
}