package org.uoyabause.android

import android.content.Context
import android.content.res.TypedArray
import android.util.AttributeSet
import androidx.preference.DialogPreference

class InputSettingPreference : DialogPreference {
  constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int) : super(
    context,
    attrs,
    defStyleAttr,
    defStyleRes
  ) {
  }

  constructor(context: Context?, attrs: AttributeSet?, defStyleAttr: Int) : super(
    context,
    attrs,
    defStyleAttr
  ) {
  }

  constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {}

  override fun onGetDefaultValue(arr: TypedArray, index: Int): Any {
    return arr.getString(index)!!
  }

  override fun onSetInitialValue(defaultValueObj: Any?) {
    //String colorName = getPersistedString((String) defaultValueObj);
    //setSummary( colorName );
  }

  /** 現在のPreferenceの値を返す  */
  fun retrieveValue(): String {
    return getPersistedString("")
  }

  fun save() {
    persistString("yabause/" + save_filename + "_v2.json")
    notifyChanged()
  }
  //is.saveFilename("yabause/" + save_filename + ".json")
  /** Preferenceに値を設定＆反映させる  */
  fun saveFilename(filename: String?) {
    persistString(filename)
    notifyChanged()
    //setSummary( filename );
  }

  var save_filename = "keymap"
    private set
  var playerid = 0
    private set

  fun setPlayerAndFilename(playerid: Int, fname: String) {
    this.playerid = playerid
    save_filename = fname
  }
}