package org.uoyabause.android

import android.app.ActivityManager
import android.app.AlertDialog
import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.SharedPreferences.OnSharedPreferenceChangeListener
import android.content.pm.PackageManager
import android.hardware.input.InputManager
import android.os.Build
import android.os.Bundle
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import android.text.InputType
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat.getExternalFilesDirs
import androidx.core.content.ContextCompat.getSystemService
import androidx.fragment.app.DialogFragment
import androidx.preference.CheckBoxPreference
import androidx.preference.EditTextPreference
import androidx.preference.ListPreference
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import androidx.preference.PreferenceManager
import androidx.preference.PreferenceScreen
import androidx.preference.PreferenceCategory
import net.nend.android.a.g.p
import org.devmiyax.yabasanshiro.BuildConfig
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.YabauseStorage.Companion.storage
import org.uoyabause.android.tv.GameSelectFragment
import java.util.ArrayList


class SettingsActivity : AppCompatActivity() {

    class WarningDialogFragment : DialogFragment() {
        override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
            // Use the Builder class for convenient dialog construction
            val builder = AlertDialog.Builder(requireActivity())
            val res = resources
            builder.setMessage(res.getString(R.string.msg_opengl_not_supported))
                .setPositiveButton("OK") { _, _ ->
                    // FIRE ZE MISSILES!
                }

            // Create the AlertDialog object and return it
            return builder.create()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.settings_activity)
        supportFragmentManager
            .beginTransaction()
            .replace(R.id.settings, SettingsFragment())
            .commit()
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
    }

    class SettingsFragment : PreferenceFragmentCompat(),
        InputManager.InputDeviceListener,
        OnSharedPreferenceChangeListener,
        GameDirectoriesDialogPreference.DirListChangeListener {
        private val DIALOG_FRAGMENT_TAG = "CustomPreference"
        var inputManager: InputManager? = null
        var dirlist_status = false
        var restart_level = 0

        override fun onResume() {
            super.onResume()
            inputManager?.registerInputDeviceListener(this, null)
            preferenceScreen.sharedPreferences
                .registerOnSharedPreferenceChangeListener(this)
        }

        override fun onPause() {
            super.onPause()
            inputManager?.unregisterInputDeviceListener(this)
            preferenceScreen.sharedPreferences
                .unregisterOnSharedPreferenceChangeListener(this)
        }

        override fun onDestroy() {
            super.onDestroy()
        }

        fun setUpInstall() {

            val prefs: SharedPreferences? = YabauseApplication.appContext.getSharedPreferences("private",  Context.MODE_PRIVATE)
            var hasDonated = false
            if (prefs != null) {
                hasDonated = prefs.getBoolean("donated", false)
                if (BuildConfig.BUILD_TYPE == "pro" || hasDonated) {
                    return
                }

                val preferenceCategory = findPreference("game_select_screen") as PreferenceCategory?
                val preference = Preference(preferenceScreen.context)
                preference.title = getString(R.string.remaining_installation_count)
                val count = prefs?.getInt("InstallCount", 3)
                preference.summary = count.toString()
                preferenceCategory?.addPreference(preference)
            }

        }
        override fun onInputDeviceAdded(id: Int) {
            PadManager.updatePadManager()
            syncInputDevice("player1")
            syncInputDevice("player2")
        }

        override fun onInputDeviceRemoved(id: Int) {
            PadManager.updatePadManager()
            syncInputDevice("player1")
            syncInputDevice("player2")
        }

        override fun onInputDeviceChanged(id: Int) {
            PadManager.updatePadManager()
            syncInputDevice("player1")
            syncInputDevice("player2")
        }

        @RequiresApi(Build.VERSION_CODES.N)
        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            setPreferencesFromResource(R.xml.preferences, rootKey)


            var dir = findPreference("pref_game_directory") as Preference?
            if ( dir != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                dir.isVisible = false
            }

            var installLocation = findPreference("pref_install_location") as ListPreference?
            if ( installLocation != null && Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
                installLocation.isVisible = false
            }else{

                val labels: MutableList<CharSequence> = ArrayList()
                val values: MutableList<CharSequence> = ArrayList()

                val sm = requireActivity().getSystemService(STORAGE_SERVICE) as StorageManager
                val map: Map<String, String> = when {
                    Build.VERSION.SDK_INT>= Build.VERSION_CODES.R -> {
                        // Android 11- (API 30)
                        sm.storageVolumes.mapNotNull { volume ->
                            val path = volume.directory?.absolutePath ?: return@mapNotNull null
                            val label = volume.getDescription(requireActivity()) ?: return@mapNotNull null
                            path to label
                        }.toMap()
                    }
                    Build.VERSION.SDK_INT >= Build.VERSION_CODES.N -> {
                        // Android 7-10 (API 24-29)
                        val getPath = StorageVolume::class.java.getDeclaredMethod("getPath")
                        sm.storageVolumes.mapNotNull { volume ->
                            val path = (getPath.invoke(volume) as String?) ?: return@mapNotNull null
                            val label = volume.getDescription(requireActivity()) ?: return@mapNotNull null
                            path to label
                        }.toMap()
                    }
                    else -> {
                        // Android 4-6 (API 14-23)
                        val getVolumeList = sm.javaClass.getDeclaredMethod("getVolumeList")
                        (getVolumeList.invoke(sm) as Array<*>).filterNotNull().mapNotNull { volume ->
                            val getPath = volume.javaClass.getDeclaredMethod("getPath") ?: return@mapNotNull null
                            val getLabel = volume.javaClass.getDeclaredMethod("getDescription", Context::class.java)
                            val path = (getPath.invoke(volume) as String?) ?: return@mapNotNull null
                            val label = (getLabel.invoke(volume, requireActivity()) as String?) ?: return@mapNotNull null
                            path to label
                        }.toMap()
                    }
                }

                var index = 0
                map.forEach() { path,label ->
                    labels.add(label)
                    values.add(index.toString())
                    index++
                }

                installLocation!!.entries = labels.toTypedArray()
                installLocation!!.entryValues = values.toTypedArray()
                installLocation!!.summary = installLocation!!.entry

            }


            var inputsetting1 = findPreference("pref_player1_inputdef_file") as InputSettingPreference?
            inputsetting1!!.setPlayerAndFilename(0, "keymap")
            var inputsetting2 = findPreference("pref_player2_inputdef_file") as InputSettingPreference?
            inputsetting2!!.setPlayerAndFilename(1, "keymap_player2")

            val res = resources
            val storage = storage

            /* bios */
            val p = preferenceManager.findPreference("pref_bios") as ListPreference?
            if (p != null) {
                val labels: MutableList<CharSequence> = ArrayList()
                val values: MutableList<CharSequence> = ArrayList()
                val biosFiles: Array<String?>? = storage.biosFiles
                labels.add("built-in bios")
                values.add("")
                if (biosFiles != null && biosFiles.size > 0) {
                    for (bios in biosFiles) {
                        labels.add(bios!!)
                        values.add(bios)
                    }
                    p.entries = labels.toTypedArray()
                    p.entryValues = values.toTypedArray()
                    p.summary = p.entry
                } else {
                    p.isEnabled = false
                    p.summary = "built-in bios"
                }
            }

            /* cartridge */
            val cart =
                preferenceManager.findPreference("pref_cart") as ListPreference?
            if (cart != null) {
                val cartlabels: MutableList<CharSequence> = ArrayList()
                val cartvalues: MutableList<CharSequence> = ArrayList()

                for (carttype in 0 until Cartridge.typeCount) {
                    cartlabels.add(Cartridge.getName(carttype))
                    cartvalues.add(Integer.toString(carttype))
                }

                cart.entries = cartlabels.toTypedArray() // cartentries
                cart.entryValues = cartvalues.toTypedArray()
                cart.summary = cart.entry
            }

            /* Cpu */
            val cpu_setting =
                preferenceManager.findPreference("pref_cpu") as ListPreference?
            cpu_setting!!.summary = cpu_setting.entry
            val abi = System.getProperty("os.arch")
            if (abi?.contains("64") == true) {
                val cpu_labels: MutableList<CharSequence> = ArrayList()
                val cpu_values: MutableList<CharSequence> = ArrayList()
                cpu_labels.add(res.getString(R.string.new_dynrec_cpu_interface))
                cpu_values.add("3")
                cpu_labels.add(res.getString(R.string.software_cpu_interface))
                cpu_values.add("0")
                // val cpu_entries = arrayOfNulls<CharSequence>(cpu_labels.size)
                // cpu_labels.toArray<CharSequence>(cpu_entries)
                // val cpu_entryValues = arrayOfNulls<CharSequence>(cpu_values.size)
                // cpu_values.toArray<CharSequence>(cpu_entryValues)
                cpu_setting.entries = cpu_labels.toTypedArray()
                cpu_setting.entryValues = cpu_values.toTypedArray()
                cpu_setting.summary = cpu_setting.entry
            }

            /* Video */
            val video_cart =
                preferenceManager.findPreference("pref_video") as ListPreference?

            val video_labels: MutableList<CharSequence> = ArrayList()
            val video_values: MutableList<CharSequence> = ArrayList()

            val activityManager = requireActivity().getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
            val configurationInfo = activityManager.deviceConfigurationInfo
            val supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000

            val deviceSupportsAEP: Boolean =
                requireActivity().getPackageManager().hasSystemFeature(PackageManager.FEATURE_OPENGLES_EXTENSION_PACK)

            if (supportsEs3) {
                video_labels.add(res.getString(R.string.opengl_video_interface))
                video_values.add("1")
            } else {
                val newFragment = WarningDialogFragment()
                newFragment.show(parentFragmentManager, "OGL")
            }

            video_labels.add(res.getString(R.string.software_video_interface))
            video_values.add("2")


            if( requireActivity().getPackageManager().hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL) ) {
                video_labels.add(res.getString(R.string.vulkan_video_interface))
                video_values.add("4")
            }

            video_cart!!.entries = video_labels.toTypedArray()
            video_cart.entryValues = video_values.toTypedArray()
            video_cart.summary = video_cart.entry

            /* Filter */
            val filter_setting =
                preferenceManager.findPreference("pref_filter") as ListPreference?
            filter_setting!!.summary = filter_setting.entry
            filter_setting.isEnabled = video_cart.value == "1"

            /* scsp */
            val scsp_setting =
                preferenceManager.findPreference("pref_scsp_sync_per_frame") as EditTextPreference?
            scsp_setting!!.summary = scsp_setting.text

            /* Polygon Generation */

            // ListPreference cpu_sync_setting = (ListPreference) getPreferenceManager().findPreference("pref_cpu_sync_per_line");
            // cpu_sync_setting.setSummary(cpu_sync_setting.getEntry());

            /* Polygon Generation */
            val polygon_setting =
                preferenceManager.findPreference("pref_polygon_generation") as ListPreference?
            polygon_setting!!.summary = polygon_setting.entry
            polygon_setting.isEnabled = (video_cart.value == "1" || video_cart.value == "4" )

            if (deviceSupportsAEP == false) {
                polygon_setting.entries =
                    arrayOf("Triangles using perspective correction")
                polygon_setting.entryValues = arrayOf("0")
            }

            val r_setting =
                preferenceManager.findPreference("pref_use_compute_shader") as CheckBoxPreference?
            if(video_cart.value == "4" ){
                r_setting?.isEnabled = false;
                r_setting?.isChecked = true;
            }else if(video_cart.value == "1" ){
                r_setting?.isEnabled = true;
            }else{
                r_setting?.isEnabled = false;
                r_setting?.isChecked = false;
            }



            val onscreen_pad =
                findPreference("on_screen_pad") as PreferenceScreen?
            onscreen_pad!!.onPreferenceClickListener = Preference.OnPreferenceClickListener {
                val nextActivity = Intent(requireContext(), PadTestActivity::class.java)
                startActivity(nextActivity)
                true
            }

            syncInputDevice("player1")
            syncInputDevice("player2")

            /*
        Preference select_image = findPreference("select_image");
        select_image.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {

                Intent intent = new Intent();
                intent.setType("image/ *");
                intent.setAction(Intent.ACTION_GET_CONTENT);
                int PICK_IMAGE = 1;
                startActivityForResult(Intent.createChooser(intent, "Select Picture"), PICK_IMAGE);

                return true;
            }
        });
*/
            val soundengine_setting =
                preferenceManager.findPreference("pref_sound_engine") as ListPreference?
            soundengine_setting!!.summary = soundengine_setting.entry

            val resolution_setting =
                preferenceManager.findPreference("pref_resolution") as ListPreference?
            resolution_setting!!.summary = resolution_setting.entry

            val aspect_setting =
                preferenceManager.findPreference("pref_aspect_rate") as ListPreference?
            aspect_setting!!.summary = aspect_setting.entry

            val rbg_resolution_setting =
                preferenceManager.findPreference("pref_rbg_resolution") as ListPreference?
            rbg_resolution_setting!!.summary = rbg_resolution_setting.entry

            val scsp_time_sync_setting =
                preferenceManager.findPreference("scsp_time_sync_mode") as ListPreference?
            scsp_time_sync_setting!!.summary = scsp_time_sync_setting.entry

            val scsp_sync_time =
                preferenceManager.findPreference("pref_scsp_sync_per_frame") as EditTextPreference?
            scsp_sync_time?.setOnBindEditTextListener {
                it.inputType = InputType.TYPE_CLASS_NUMBER
            }


            val frameLimitSetting =
                preferenceManager.findPreference("pref_frameLimit") as ListPreference?
            frameLimitSetting!!.summary = frameLimitSetting.entry


            setUpInstall();



        }

        override fun onDisplayPreferenceDialog(preference: Preference) {
            val f: DialogFragment?
            if (preference is InputSettingPreference) {
                f = InputSettingPreferenceFragment.newInstance(preference.getKey())
            } else if (preference is GameDirectoriesDialogPreference) {

                preference.setDirListChangeListener(this)

                // Above version 10
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {
                    var dir = YabauseStorage.storage.gamePath
                    if (YabauseStorage.storage.hasExternalSD()) {
                        dir += "\n" + YabauseStorage.storage.externalGamePath
                    }

                    Toast.makeText(requireActivity(),
                        "Android 10 device only supports \n $dir",
                        Toast.LENGTH_LONG).show()
                    return
                } else {
                    f = GameDirectoriesDialogFragment.newInstance(preference.getKey())
                }
            } else {
                f = null
            }

            if (f != null) {
                f.setTargetFragment(this, 0)
                f.show(requireActivity().supportFragmentManager, DIALOG_FRAGMENT_TAG)
            } else {
                super.onDisplayPreferenceDialog(preference)
            }
        }

        private fun syncInputDevice(player: String) {

            val devicekey = "pref_" + player + "_inputdevice"
            val defkey = "pref_" + player + "_inputdef_file"
            val sharedPref = PreferenceManager.getDefaultSharedPreferences(activity)
            val res = resources
            val padm = PadManager.getPadManager()
            val input_device =
                preferenceManager.findPreference(devicekey) as ListPreference?
            val Inputlabels: MutableList<CharSequence> = ArrayList()
            val Inputvalues: MutableList<CharSequence> = ArrayList()
            Inputlabels.add(res.getString(R.string.onscreen_pad))
            Inputvalues.add("-1")
            for (inputType in 0 until padm.deviceCount) {
                Inputlabels.add(padm.getName(inputType))
                Inputvalues.add(padm.getId(inputType))
            }
            input_device!!.entries = Inputlabels.toTypedArray()
            input_device.entryValues = Inputvalues.toTypedArray()
            input_device.summary = input_device.entry

            var inputsetting = preferenceManager.findPreference(defkey) as InputSettingPreference?
            var onscreen_pad = preferenceManager.findPreference("on_screen_pad") as PreferenceScreen?
            if (inputsetting != null) {
                try {
                    val selInputdevice = sharedPref.getString(devicekey, "65535")
                    if (padm.getDeviceCount() > 0 && !selInputdevice.equals("-1")) {
                        inputsetting.setEnabled(true)
                        if (player == "player1") onscreen_pad!!.setEnabled(true)
                    } else {
                        inputsetting.setEnabled(false)
                        if (player == "player1") onscreen_pad!!.setEnabled(true)
                    }

                    if (player == "player1") {
                        padm.setPlayer1InputDevice(selInputdevice)
                    } else {
                        padm.setPlayer2InputDevice(selInputdevice)
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }

        override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String) {

            if (key == "pref_bios" || key == "scsp_time_sync_mode" || key == "pref_cart" || key == "pref_video"
                || key == "pref_cpu" || key == "pref_filter" || key == "pref_polygon_generation"
                || key == "pref_sound_engine" || key == "pref_resolution" || key == "pref_rbg_resolution"
                || key == "pref_cpu_sync_per_line" || key == "pref_aspect_rate" || key == "pref_frameLimit" || key == "pref_install_location"
            ) {
                val pref = findPreference(key) as ListPreference?
                pref!!.summary = pref.entry
                if (key == "pref_video") {
                    val filter_setting =
                        preferenceManager.findPreference("pref_filter") as ListPreference?
                    filter_setting!!.isEnabled = pref.value == "1"
                    val polygon_setting =
                        preferenceManager.findPreference("pref_polygon_generation") as ListPreference?
                    polygon_setting!!.summary = polygon_setting.entry
                    polygon_setting.isEnabled = (pref.value == "1" || pref.value == "4" )

                    val r_setting =
                        preferenceManager.findPreference("pref_use_compute_shader") as CheckBoxPreference?
                    if(pref.value == "4" ){
                        r_setting?.isEnabled = false;
                        r_setting?.isChecked = true;
                    }else if(pref.value == "1" ){
                        r_setting?.isEnabled = true;
                    }else{
                        r_setting?.isEnabled = false;
                        r_setting?.isChecked = false;
                    }
                }
            } else if (key == "pref_player1_inputdevice") {
                val pref = findPreference(key) as ListPreference?
                pref!!.summary = pref.entry
                syncInputDevice("player1")
                syncInputDevice("player2")
            } else if (key == "pref_player2_inputdevice") {
                val pref = findPreference(key) as ListPreference?
                pref!!.summary = pref.entry
                syncInputDevice("player1")
                syncInputDevice("player2")
            }


            val install =
                preferenceManager.findPreference("pref_install_location") as ListPreference?
            if (install != null) {
                install.summary = install.entry
            }


            val download =
                preferenceManager.findPreference("pref_game_download_directory") as ListPreference?
            if (download != null) {
                download.summary = download.entry
            }

            if (key == "pref_scsp_sync_per_frame") {
                val ep = findPreference(key) as EditTextPreference?
                val sval = ep!!.text

                var synccount = 0
                try {
                    synccount = sval.toInt()
                } catch( e: Exception ){

                }

                if (synccount <= 0) {
                    synccount = 1
                    val sp = PreferenceManager.getDefaultSharedPreferences(requireActivity())
                    sp.edit().putString("pref_scsp_sync_per_frame", synccount.toString()).commit()
                } else if (synccount > 255) {
                    synccount = 255
                    val sp = PreferenceManager.getDefaultSharedPreferences(requireActivity())
                    sp.edit().putString("pref_scsp_sync_per_frame", synccount.toString()).commit()
                }
                ep.summary = synccount.toString()
            }

            if (key == "pref_force_androidtv_mode") {
                if (restart_level <= 1) restart_level = 2
                updateResultCode()
            }
        }

        override fun onChangeDir(isChange: Boolean?) {
            dirlist_status = isChange!!
            if (dirlist_status == true) {
                if (restart_level <= 0) restart_level = 1
                updateResultCode()
            }
        }

        fun updateResultCode() {
            val resultIntent = Intent()
            if (restart_level == 1) {
                requireActivity().setResult(GameSelectFragment.GAMELIST_NEED_TO_UPDATED,
                    resultIntent)
            } else if (restart_level == 2) {
                requireActivity().setResult(GameSelectFragment.GAMELIST_NEED_TO_RESTART,
                    resultIntent)
            } else {
                requireActivity().setResult(0, resultIntent)
            }
        }

    }
}
