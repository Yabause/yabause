@file:JvmName("GameSharedPreference")

package org.uoyabause.android

import android.content.Context
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.preference.CheckBoxPreference
import androidx.preference.ListPreference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.PreferenceManager
import io.reactivex.Observable
import io.reactivex.ObservableEmitter
import io.reactivex.ObservableOnSubscribe
import io.reactivex.Observer
import io.reactivex.android.schedulers.AndroidSchedulers
import org.devmiyax.yabasanshiro.R

fun setupInGamePreferences(context: Context, gameCode: String?) {
    if (gameCode == null) {
        Log.d("setupInGamePreferences", "gamecode is null. can not read game setting")
        return
    }
    val gamePreference = context.getSharedPreferences(gameCode, Context.MODE_PRIVATE)
    val defaultPreference = PreferenceManager.getDefaultSharedPreferences(context)
    if (!gamePreference.contains("pref_fps")) {
        val editor = gamePreference.edit()
        editor.putBoolean("pref_fps", defaultPreference.getBoolean("pref_fps", false))
        editor.apply()
    }

    if (!gamePreference.contains("pref_frameskip")) {
        val editor = gamePreference.edit()
        editor.putBoolean("pref_frameskip", defaultPreference.getBoolean("pref_frameskip", false))
        editor.apply()
    }

    if (!gamePreference.contains("pref_rotate_screen")) {
        val editor = gamePreference.edit()
        editor.putBoolean(
            "pref_rotate_screen",
            defaultPreference.getBoolean("pref_rotate_screen", false)
        )
        editor.apply()
    }

    if (!gamePreference.contains("pref_polygon_generation")) {
        val editor = gamePreference.edit()
        var default = "0"
        if( defaultPreference.getString("pref_video", "0") == "4" ){
            editor.putString(
                "pref_polygon_generation","2"
            )
        }else {
            editor.putString(
                "pref_polygon_generation",
                defaultPreference.getString("pref_polygon_generation", default)
            )
        }
        editor.apply()
    }

    if (!gamePreference.contains("pref_frameLimit")) {
        val editor = gamePreference.edit()
        editor.putString(
            "pref_frameLimit",
            defaultPreference.getString("pref_frameLimit", "0")
        )
        editor.apply()
    }

    if (!gamePreference.contains("pref_resolution")) {
        val editor = gamePreference.edit()
        editor.putString("pref_resolution", defaultPreference.getString("pref_resolution", "0"))
        editor.apply()
    }

    if (!gamePreference.contains("pref_aspect_rate")) {
        val editor = gamePreference.edit()
/*
            val displayMetrics = DisplayMetrics()
            windowManager.defaultDisplay.getMetrics(displayMetrics)
            val height = displayMetrics.heightPixels
            val width = displayMetrics.widthPixels
            val arate = width.toDouble() / height.toDouble()


            if( arate >= 1.2 && arate <= 1.34 ){
                // for 4:3 display force default setting is 4:3
                val v = defaultPreference.getString("pref_aspect_rate","1")
                editor.putString("pref_aspect_rate", v)
            }else{
                val v = defaultPreference.getString("pref_aspect_rate","0")
                editor.putString("pref_aspect_rate", v)
            }
*/
        val v = defaultPreference.getString("pref_aspect_rate", "0")
        editor.putString("pref_aspect_rate", v)
        editor.apply()
    }

    if (!gamePreference.contains("pref_rbg_resolution")) {
        val editor = gamePreference.edit()
        editor.putString(
            "pref_rbg_resolution",
            defaultPreference.getString("pref_rbg_resolution", "0")
        )
        editor.apply()
    }

    if (!gamePreference.contains("pref_use_compute_shader")) {
        val editor = gamePreference.edit()
        editor.putBoolean(
            "pref_use_compute_shader",
            defaultPreference.getBoolean("pref_use_compute_shader", false)
        )
        editor.apply()
    }

    val gameSharedPreference = context.getSharedPreferences(gameCode, 0)
    val editor = gameSharedPreference.edit()
    editor.putBoolean("pref_fps", gamePreference.getBoolean("pref_fps", false))
    editor.putBoolean("pref_frameskip", gamePreference.getBoolean("pref_frameskip", false))
    editor.putBoolean("pref_rotate_screen", gamePreference.getBoolean("pref_rotate_screen", false))
    editor.putString(
        "pref_polygon_generation",
        gamePreference.getString("pref_polygon_generation", "0")
    )
    editor.putBoolean("pref_use_compute_shader", gamePreference.getBoolean("pref_use_compute_shader", false))
    editor.putString("pref_frameLimit", gamePreference.getString("pref_frameLimit", "0"))
    val v = gamePreference.getString("pref_aspect_rate", "0")
    editor.putString("pref_aspect_rate", v)
    editor.putString("pref_rbg_resolution", gamePreference.getString("pref_rbg_resolution", "0"))
    editor.apply()
}

class InGamePreference(val gamecode: String) : PreferenceFragmentCompat(), SharedPreferences.OnSharedPreferenceChangeListener {

    companion object {
        @JvmField
        val TAG = "InGamePreference"
    }

    private lateinit var emitter: ObservableEmitter<String>
    private lateinit var activityContext: Context

    private fun showSummary(listPreference: ListPreference) {
        listPreference.summary = listPreference.entry
        listPreference.setOnPreferenceChangeListener { preference, newValue ->
            if (preference is ListPreference) {
                val index = preference.findIndexOfValue(newValue.toString())
                val entry = preference.entries.get(index)
                preference.summary = entry
            }
            true
        }
    }

    private fun setSummaries() {
        showSummary(findPreference<ListPreference?>("pref_polygon_generation")!!)
        showSummary(findPreference<ListPreference?>("pref_resolution")!!)
        showSummary(findPreference<ListPreference?>("pref_rbg_resolution")!!)
        showSummary(findPreference<ListPreference?>("pref_aspect_rate")!!)
        showSummary(findPreference<ListPreference?>("pref_frameLimit")!!)
    }

    override fun onAttach(context: Context) {
        this.activityContext = context
        super.onAttach(context)
    }

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setupInGamePreferences(activityContext, gamecode)
        preferenceManager.sharedPreferencesName = gamecode
        setPreferencesFromResource(R.xml.in_game_preferences, rootKey)

        val defaultPreference = PreferenceManager.getDefaultSharedPreferences(activityContext)

        if( defaultPreference.getString("pref_video","0") == "4") {
            var pg = findPreference<ListPreference?>("pref_polygon_generation")
            if (pg != null) {
                pg.isEnabled = false;
                pg.value = "2"
            }

           var cp = findPreference<CheckBoxPreference?>("pref_use_compute_shader")
            if( cp != null ){
                cp.isEnabled = false;
                cp.isChecked = true;
            }
        }


        setSummaries()
        this.preferenceScreen.sharedPreferences!!.registerOnSharedPreferenceChangeListener(this)
    }

    override fun onPause() {
        super.onPause()
        this.preferenceScreen.sharedPreferences!!.unregisterOnSharedPreferenceChangeListener(this)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = super.onCreateView(inflater, container, savedInstanceState)
        view.setBackgroundColor(ContextCompat.getColor(activityContext, R.color.default_background))
        return view
    }

    fun setonEndObserver(onEndObserver: Observer<String?>) {
        Observable.create(ObservableOnSubscribe<String> { emitter ->
            this.emitter = emitter
        }).observeOn(AndroidSchedulers.mainThread())
            .subscribeOn(AndroidSchedulers.mainThread())
            .subscribe(onEndObserver)
    }

    fun onBackPressed() {
        this.emitter.onComplete()
    }

    override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences?, key: String?) {

        if (sharedPreferences == null) {
            return
        }

        val gamePreference = requireContext().getHarmonySharedPreferences(gamecode)

        val editor = gamePreference.edit()
        editor.putBoolean("pref_fps", sharedPreferences.getBoolean("pref_fps", false))
        editor.putBoolean("pref_frameskip", sharedPreferences.getBoolean("pref_frameskip", false))
        editor.putBoolean("pref_rotate_screen", sharedPreferences.getBoolean("pref_rotate_screen", false))
        editor.putString("pref_polygon_generation", sharedPreferences.getString("pref_polygon_generation", "0"))
        editor.putString("pref_frameLimit", sharedPreferences.getString("pref_frameLimit", "0"))
        val v = sharedPreferences.getString("pref_aspect_rate", "0")
        editor.putString("pref_aspect_rate", v)
        editor.putString(
            "pref_resolution",
            sharedPreferences.getString("pref_resolution", "0")
        )
        editor.putString(
            "pref_rbg_resolution",
            sharedPreferences.getString("pref_rbg_resolution", "0")
        )
        editor.putBoolean(
            "pref_use_compute_shader",
            sharedPreferences.getBoolean("pref_use_compute_shader", false)
        )
        editor.apply()
    }
}
