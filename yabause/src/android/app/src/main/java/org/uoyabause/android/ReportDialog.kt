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

import android.app.AlertDialog
import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.widget.RatingBar.OnRatingBarChangeListener
import android.widget.RatingBar
import android.widget.TextView
import android.widget.EditText
import android.widget.CheckBox
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import androidx.fragment.app.DialogFragment
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.Yabause

class ReportDialog : DialogFragment(), DialogInterface.OnClickListener, OnRatingBarChangeListener {
    var _GameStatusRating: RatingBar? = null
    var _rateText: TextView? = null
    var _edt: EditText? = null
    var _chk: CheckBox? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //setStyle(DialogFragment.STYLE_NORMAL, R.style.FullScreenDialogStyle);
    }

    override fun onStart() {
        super.onStart()
        val dialog = dialog
        if (dialog != null) {
            //int width = ViewGroup.LayoutParams.MATCH_PARENT;
            //int height = ViewGroup.LayoutParams.MATCH_PARENT;
            //dialog.getWindow().setLayout(width, height);
        }
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        // Use the Builder class for convenient dialog construction
        val builder = AlertDialog.Builder(requireActivity())
        val inflater =
            requireActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater
        val content = inflater.inflate(R.layout.report, null)
        _GameStatusRating =
            content.findViewById<View>(R.id.report_ratingBar) as RatingBar
        _GameStatusRating!!.numStars = 5
        _GameStatusRating!!.rating = 3.0f
        _GameStatusRating!!.stepSize = 1.0f
        _GameStatusRating!!.onRatingBarChangeListener = this
        _edt = content.findViewById<View>(R.id.report_message) as EditText
        _chk = content.findViewById<View>(R.id.report_Screenshot) as CheckBox
        _rateText = content.findViewById<View>(R.id.report_rateString) as TextView
        onRatingChanged(_GameStatusRating!!, 3.0f, false)
        builder.setView(content)
        builder.setPositiveButton(R.string.send, this)
            .setNegativeButton(R.string.cancel, this)
        return builder.create()
    }

    override fun onClick(dialog: DialogInterface, which: Int) {
        if (which == -1) {
            val activity = activity as Yabause?
            val rating = _GameStatusRating!!.rating.toInt()
            val message = _edt!!.text.toString()
            activity!!.doReportCurrentGame(rating, message, _chk!!.isChecked)
        } else if (which == -2) {
            val activity = activity as Yabause?
            activity!!.cancelReportCurrentGame()
        } else {
        }
    }

    override fun onRatingChanged(
        ratingBar: RatingBar, rating: Float,
        fromUser: Boolean
    ) {
        val iRate = rating.toInt()
        when (iRate) {
            0 -> _rateText!!.setText(R.string.report_message_1)
            1 -> _rateText!!.setText(R.string.report_message_2)
            2 -> _rateText!!.setText(R.string.report_message_3)
            3 -> _rateText!!.setText(R.string.report_message_4)
            4 -> _rateText!!.setText(R.string.report_message_5)
            5 -> _rateText!!.setText(R.string.report_message_6)
            else -> {}
        }
    }
}