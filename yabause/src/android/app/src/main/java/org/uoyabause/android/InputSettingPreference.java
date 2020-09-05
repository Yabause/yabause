package org.uoyabause.android;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;

import androidx.preference.DialogPreference;

public class InputSettingPreference  extends DialogPreference {
  public InputSettingPreference(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
    super(context, attrs, defStyleAttr, defStyleRes);
  }
  public InputSettingPreference(Context context, AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
  }
  public InputSettingPreference(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  @Override
  protected Object onGetDefaultValue(TypedArray arr, int index ) {
    return arr.getString(index);
  }

  @Override
  protected void onSetInitialValue( Object defaultValueObj ) {
    //String colorName = getPersistedString((String) defaultValueObj);
    //setSummary( colorName );
  }

  /** 現在のPreferenceの値を返す **/
  public String retrieveValue() {
    return getPersistedString("");
  }

  public void save(){
    persistString( "yabause/" + save_filename + ".json" );
    notifyChanged();
  }
  //is.saveFilename("yabause/" + save_filename + ".json")
  /** Preferenceに値を設定＆反映させる **/
  public void saveFilename( String filename ) {
    persistString( filename );
    notifyChanged();
    //setSummary( filename );
  }

  private String save_filename = "keymap";
  private int playerid = 0;

  public int getPlayerid() {
    return playerid;
  }

  public String getSave_filename(){
    return save_filename;
  }

  public void setPlayerAndFilename(int playerid, String fname) {
    this.playerid = playerid;
    this.save_filename = fname;
  }
}
