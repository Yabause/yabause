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

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.RatingBar;
import android.widget.TextView;

import org.uoyabause.uranus.R;

public class ReportDialog extends DialogFragment implements DialogInterface.OnClickListener,RatingBar.OnRatingBarChangeListener {

    RatingBar _GameStatusRating;
    TextView _rateText;
    EditText _edt;
    CheckBox _chk;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setStyle(DialogFragment.STYLE_NORMAL, R.style.FullScreenDialogStyle);
    }

    @Override
    public void onStart() {
        super.onStart();
        Dialog dialog = getDialog();
        if (dialog != null) {
            //int width = ViewGroup.LayoutParams.MATCH_PARENT;
            //int height = ViewGroup.LayoutParams.MATCH_PARENT;
            //dialog.getWindow().setLayout(width, height);
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        // Use the Builder class for convenient dialog construction
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

        LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View content = inflater.inflate(R.layout.report, null);
        _GameStatusRating = (RatingBar)content.findViewById(R.id.report_ratingBar);
        _GameStatusRating.setNumStars(5);
        _GameStatusRating.setRating(3.0f);
        _GameStatusRating.setStepSize(1.0f);
        _GameStatusRating.setOnRatingBarChangeListener(this);

        _edt =(EditText)content.findViewById(R.id.report_message);
        _chk =(CheckBox)content.findViewById(R.id.report_Screenshot);
        _rateText = (TextView)content.findViewById( R.id.report_rateString);
        onRatingChanged(_GameStatusRating,3.0f,false);
        builder.setView(content);

        builder.setPositiveButton(R.string.send, this)
               .setNegativeButton(R.string.cancel,this);

        AlertDialog dialog = builder.create();

        return dialog;
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {

        if( which == -1 ) {
            Yabause activity = (Yabause) getActivity();
            int rating = (int) _GameStatusRating.getRating();
            String message = _edt.getText().toString();
            activity.doReportCurrentGame(rating, message, _chk.isChecked());
        }else if( which == -2 ){
            Yabause activity = (Yabause) getActivity();
            activity.cancelReportCurrentGame();
        }else{

        }
    }

    @Override
    public void onRatingChanged(RatingBar ratingBar, float rating,
                                boolean fromUser) {
        int iRate = (int)rating;
        switch(iRate){
            case 0:
                _rateText.setText(R.string.report_message_1);
                break;
            case 1:
                _rateText.setText(R.string.report_message_2);
                break;
            case 2:
                _rateText.setText(R.string.report_message_3);
                break;
            case 3:
                _rateText.setText(R.string.report_message_4);
                break;
            case 4:
                _rateText.setText(R.string.report_message_5);
                break;
            case 5:
                _rateText.setText(R.string.report_message_6);
                break;
            default:
                break;
        }

    }
}
