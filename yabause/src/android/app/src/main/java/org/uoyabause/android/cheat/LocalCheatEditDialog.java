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

package org.uoyabause.android.cheat;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import androidx.fragment.app.DialogFragment;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import org.uoyabause.uranus.R;

public class LocalCheatEditDialog extends DialogFragment {

    View mContent;
    public static final int APPLY = 0;
    public static final int CANCEL = 1;
    public static final String KEY = "key";
    public static final String DESC = "desc";
    public static final String CODE = "code";
    CheatItem target_ = null;

    void setEditTarget( CheatItem cheat ){
        target_ = cheat;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mContent = inflater.inflate(R.layout.edit_cheat, null);
        builder.setView(mContent);
        EditText desc  = (EditText )mContent.findViewById(R.id.editText_cheat_desc);
        EditText code = (EditText)mContent.findViewById(R.id.editText_code);

        if( target_ != null ){
            desc.setText(target_.getDescription());
            code.setText(target_.getCheatCode());
        }

        // Apply Button
        Button button_apply = (Button)mContent.findViewById(R.id.button_cheat_edit_apply);
        button_apply.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                EditText desc  = (EditText )mContent.findViewById(R.id.editText_cheat_desc);
                EditText code = (EditText)mContent.findViewById(R.id.editText_code);
                Intent intent = new Intent();
                String sdesc = desc.getText().toString();
                String scode = code.getText().toString();
                intent.putExtra(DESC,sdesc );
                intent.putExtra(CODE,scode);
                if( target_ != null ){
                    intent.putExtra(KEY,target_.getKey());
                }
                getTargetFragment().onActivityResult(getTargetRequestCode(), APPLY, intent);
                getDialog().dismiss();

            }
        });

        Button button_cancel = (Button)mContent.findViewById(R.id.button_edit_cheat_cancel);
        button_cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                getTargetFragment().onActivityResult(getTargetRequestCode(), CANCEL, null);
                LocalCheatEditDialog.this.dismiss();
            }
        });

        ((EditText)mContent.findViewById(R.id.editText_code)).setOnEditorActionListener(
                new EditText.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                        //For ShieldTV
                        if ( actionId == EditorInfo.IME_ACTION_NEXT) {
                            EditText edit_view = (EditText)mContent.findViewById(R.id.editText_code);
                            String currentstr = edit_view.getText().toString();
                            currentstr += System.lineSeparator();
                            edit_view.setText(currentstr);
                            return true;
                        }
                        return false;
                    }
                });

        return builder.create();
    }

}
