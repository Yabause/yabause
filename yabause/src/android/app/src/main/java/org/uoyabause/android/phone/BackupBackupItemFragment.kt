package org.uoyabause.android.phone

import android.os.Bundle
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import androidx.preference.PreferenceManager
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.GameSelectPresenter
import org.uoyabause.android.ShowPinInFragment
import org.uoyabause.android.YabauseStorage
import org.uoyabause.android.phone.placeholder.PlaceholderContent
import java.io.File


/**
 * A fragment representing a list of Items.
 */
class BackupBackupItemFragment : Fragment() {

    private var columnCount = 1
    private lateinit var presenter_: GameSelectPresenter


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        arguments?.let {
            columnCount = it.getInt(ARG_COLUMN_COUNT)
        }
    }



    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?,
    ): View? {
        val view = inflater.inflate(R.layout.fragment_backupbackup_item_list, container, false)
        val cview = view.findViewById<RecyclerView>(R.id.BackupBackuplist)
        // Set the adapter
        if (cview is RecyclerView) {
            with(cview) {
                layoutManager = when {
                    columnCount <= 1 -> LinearLayoutManager(context)
                    else -> GridLayoutManager(context, columnCount)
                }
                adapter = BackupBackupItemRecyclerViewAdapter(PlaceholderContent.ITEMS)
                PlaceholderContent.adapter = adapter as BackupBackupItemRecyclerViewAdapter

                val a = adapter as BackupBackupItemRecyclerViewAdapter
                a.setRollback {

                    val auth = FirebaseAuth.getInstance()
                    val currentUser = auth.currentUser
                    if (currentUser != null) {

                        /*
                        val downloadFilename =
                            PlaceholderContent.ITEMS[it].rawdata["filename"] as String
                        val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
                        presenter_.downloadBackupMemory(
                            downloadFilename,
                            currentUser,
                            destinationDirectory,
                            true
                        )
                        */

                        val database = FirebaseDatabase.getInstance()
                        val backupReference = database.getReference("user-posts").child(currentUser.uid)
                            .child("backupHistory")

                        val dataref = backupReference.child( PlaceholderContent.ITEMS[it].key )

                        dataref.child("date").setValue(System.currentTimeMillis() ).addOnCompleteListener { task ->
                            if (task.isSuccessful) {
                                // データの更新が成功した場合に実行される処理
                                //Log.d(TAG, "Data updated successfully.")
                                presenter_.syncBackup()
                            } else {
                                // データの更新が失敗した場合に実行される処理
                                //Log.w(TAG, "Failed to update data.", task.exception)
                            }
                        }



                    }
                }
            }
        }

        val chk = view.findViewById<CheckBox>(R.id.checkBoxAutoBackup)
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(this.requireActivity())
        chk.isChecked  = sharedPref.getBoolean("auto_backup",false)
        if( chk.isChecked  == false ){
            val touchInterceptorView = view.findViewById<View>(R.id.touch_interceptor_view)
            touchInterceptorView.setOnTouchListener { _, _ -> true }
            cview.alpha = 0.5f
        }else{
            val touchInterceptorView = view.findViewById<View>(R.id.touch_interceptor_view)
            touchInterceptorView.setOnTouchListener { _, _ -> false }
            cview.alpha = 1.0f
        }

        chk.setOnCheckedChangeListener { buttonView, isChecked ->
            if (isChecked) {
                // チェックが入ったときの処理
                //Log.d(TAG, "CheckBox is checked")
                val touchInterceptorView = view.findViewById<View>(R.id.touch_interceptor_view)
                touchInterceptorView.setOnTouchListener { _, _ -> false }
                cview.alpha = 1.0f

                val sharedPref = PreferenceManager.getDefaultSharedPreferences(this.requireActivity())
                sharedPref.edit().putBoolean("auto_backup",true).commit()

                presenter_.syncBackup()

            } else {
                // チェックが外れたときの処理
                //Log.d(TAG, "CheckBox is unchecked")
                val touchInterceptorView = view.findViewById<View>(R.id.touch_interceptor_view)
                touchInterceptorView.setOnTouchListener { _, _ -> true }
                cview.alpha = 0.5f

                val sharedPref = PreferenceManager.getDefaultSharedPreferences(this.requireActivity())
                sharedPref.edit().putBoolean("auto_backup",false).commit()
            }
        }

        return view
    }

    companion object {

        // TODO: Customize parameter argument names
        const val ARG_COLUMN_COUNT = "column-count"
        const val ARG_PRESENTER = "presenter"

        const val TAG ="backup-backup-fragment"

        // TODO: Customize parameter initialization
        @JvmStatic
        fun newInstance(columnCount: Int, presenter: GameSelectPresenter ) =
            BackupBackupItemFragment().apply {
                arguments = Bundle().apply {
                    putInt(ARG_COLUMN_COUNT, columnCount)
                }
                presenter_ = presenter
            }
    }
}