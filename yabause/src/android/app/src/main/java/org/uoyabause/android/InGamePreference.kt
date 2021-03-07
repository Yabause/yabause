@file:JvmName("GameSharedPreference")

package org.uoyabause.android

import android.content.Context
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
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
        editor.putString(
            "pref_polygon_generation",
            defaultPreference.getString("pref_polygon_generation", "0")
        )
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
        // editor.putInt("pref_aspect_rate", defaultPrefernce.getInt("pref_resolution",0))
        editor.putString("pref_aspect_rate", "0")
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
}

class InGamePreference(val gamecode: String) : PreferenceFragmentCompat() {

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
        setSummaries()
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = super.onCreateView(inflater, container, savedInstanceState)
        view?.setBackgroundColor(ContextCompat.getColor(activityContext, R.color.default_background))
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
}
