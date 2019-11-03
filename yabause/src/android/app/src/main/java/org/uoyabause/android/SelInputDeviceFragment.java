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

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

import org.uoyabause.uranus.R;

import java.util.ArrayList;
import java.util.List;

import androidx.appcompat.app.AlertDialog;

class InputDevice{

    public InputDevice( CharSequence Inputlabel, CharSequence Inputvalue){
        Inputlabel_ = Inputlabel;
        Inputvalue_ = Inputvalue;
    }
    public CharSequence Inputlabel_;
    public CharSequence Inputvalue_;
}

public class SelInputDeviceFragment extends DialogFragment implements InputManager.InputDeviceListener{

    public final static int PLAYER1 = 0;
    public final static int PLAYER2 = 1;

    public interface InputDeviceListener{
        public void onDeviceUpdated( int target );
        public void onSelected( int target, String name, String id );
        public void onCancel(int target );
    }

    InputManager mInputManager;
    InputDeviceListener listener_ = null;
    private PadManager padm;
    List<InputDevice> InputDevices = new ArrayList<InputDevice>();
    int target_ = PLAYER1;

    public void setTarget( int target ){
        target_ = target;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mInputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    public void setListener( InputDeviceListener l ){
        listener_ = l;
    }

    public Dialog onCreateDialog(Bundle savedInstanceState){

        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(getActivity());
        Resources res = getResources();

        PadManager.updatePadManager();
        padm = PadManager.getPadManager();
        padm.loadSettings();

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());

        String selInputdevice;
        switch(target_){
            case PLAYER1:
                selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
                break;
            case PLAYER2:
                selInputdevice = sharedPref.getString("pref_player2_inputdevice", "65535");
                break;
            default:
                selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
                break;
        }


        InputDevices.clear();

        String[] items;
        int insert_index =0;
        if( target_ == PLAYER1 ) {
          InputDevice pad = new InputDevice(res.getString(R.string.onscreen_pad), "-1");
          InputDevices.add(pad);
          items = new String[padm.getDeviceCount() + 1];
          items[insert_index] = res.getString(R.string.onscreen_pad);
          insert_index++;
        }else{
          InputDevice pad = new InputDevice("Disconnect", "-1");
          InputDevices.add(pad);
          items = new String[padm.getDeviceCount()+1];
          items[insert_index] = "Disconnect";
          insert_index++;
        }

        for(int inputType = 0;inputType < padm.getDeviceCount();inputType++) {
            InputDevice tmp =  new InputDevice(padm.getName(inputType),padm.getId(inputType));
            InputDevices.add(tmp);
            items[insert_index] = padm.getName(inputType);
          insert_index++;
        }

        int selid = 0;
        for(int i = 0;i < InputDevices.size(); i++) {
            if( InputDevices.get(i).Inputvalue_.equals(selInputdevice)  ){
                selid = i;
            }
        }

        dialogBuilder.setTitle("Select Input Device");
        dialogBuilder.setSingleChoiceItems(items, selid, null);
        dialogBuilder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                if( listener_ != null ){
                    listener_.onCancel( SelInputDeviceFragment.this.target_ );
                }

            }
        });
        dialogBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
                int selectedPosition = ((AlertDialog)dialog).getListView().getCheckedItemPosition();

                SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
                SharedPreferences.Editor editor = sharedPref.edit();
                String edit_target;
                switch(target_){
                    case PLAYER1:
                        edit_target = "pref_player1_inputdevice";
                        break;
                    case PLAYER2:
                        edit_target = "pref_player2_inputdevice";
                        break;
                    default:
                        edit_target = "pref_player1_inputdevice";
                        break;
                }
                editor.putString(edit_target, (String)InputDevices.get(selectedPosition).Inputvalue_);
                editor.apply();

                if( listener_ != null ){
                    listener_.onSelected( SelInputDeviceFragment.this.target_,
                        (String)InputDevices.get(selectedPosition).Inputlabel_,
                        (String)InputDevices.get(selectedPosition).Inputvalue_ );
                }
            }
        });

        return dialogBuilder.create();
    }

    @Override
    public void onResume() {
        super.onResume();
        mInputManager.registerInputDeviceListener(this, null);
    }

    @Override
    public void onPause() {
        super.onPause();
        mInputManager.unregisterInputDeviceListener(this);
    }

    @Override
    public void onInputDeviceAdded(int i) {
        //PadManager.updatePadManager();
        if( listener_ != null ) listener_.onDeviceUpdated( target_ );
        dismiss();
    }

    @Override
    public void onInputDeviceRemoved(int i) {
        //PadManager.updatePadManager();
        if( listener_ != null ) listener_.onDeviceUpdated( target_ );
        dismiss();

    }

    @Override
    public void onInputDeviceChanged(int i) {
        //PadManager.updatePadManager();
        if( listener_ != null ) listener_.onDeviceUpdated( target_ );
        dismiss();
    }
}
