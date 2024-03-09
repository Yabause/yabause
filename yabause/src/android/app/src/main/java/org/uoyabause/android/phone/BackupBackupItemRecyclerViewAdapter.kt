package org.uoyabause.android.phone

import android.app.AlertDialog
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import org.devmiyax.yabasanshiro.databinding.FragmentBackupbackupItemBinding
import org.uoyabause.android.phone.placeholder.PlaceholderContent.PlaceholderItem




/**
 * [RecyclerView.Adapter] that can display a [PlaceholderItem].
 * TODO: Replace the implementation with code for your data type.
 */
class BackupBackupItemRecyclerViewAdapter(
    private val values: List<PlaceholderItem>,
) : RecyclerView.Adapter<BackupBackupItemRecyclerViewAdapter.ViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {

        return ViewHolder(
            FragmentBackupbackupItemBinding.inflate(
                LayoutInflater.from(parent.context),
                parent,
                false
            )
        )

    }

    lateinit var cbk : ( index : Int) -> Unit

    fun setRollback( callback: ( index : Int) -> Unit ){
        cbk = callback
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val item = values[position]
        holder.idView.text = item.id
        holder.contentView.text = item.content
        holder.gameNameView.text = item.details
        holder.itemView.setOnClickListener {
            val builder = AlertDialog.Builder(holder.itemView.getContext())
            builder.setTitle("Rollback")
            builder.setMessage("Are you sure want to rollback your backup memory to here?")
            builder.setPositiveButton("YES") { dialog, which ->
                cbk(position)
            }
            builder.setNegativeButton("NO", null)
            builder.show()
        }
    }

    override fun getItemCount(): Int = values.size

    inner class ViewHolder(binding: FragmentBackupbackupItemBinding) :
        RecyclerView.ViewHolder(binding.root) {
        val idView: TextView = binding.itemNumber
        val contentView: TextView = binding.content
        val gameNameView: TextView = binding.lastPlayGameName

        override fun toString(): String {
            return super.toString() + " '" + contentView.text + "'"
        }
    }

}