package org.uoyabause.android.phone.placeholder

import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import org.uoyabause.android.phone.BackupBackupItemRecyclerViewAdapter
import java.text.SimpleDateFormat
import java.util.ArrayList
import java.util.Date
import java.util.HashMap
import java.util.Locale

/**
 * Helper class for providing sample content for user interfaces created by
 * Android template wizards.
 *
 * TODO: Replace all uses of this class before publishing your app.
 */
object PlaceholderContent {

    /**
     * An array of sample (placeholder) items.
     */
    val ITEMS: MutableList<PlaceholderItem> = ArrayList()

    /**
     * A map of sample (placeholder) items, by ID.
     */
    val ITEM_MAP: MutableMap<String, PlaceholderItem> = HashMap()

    var adapter : BackupBackupItemRecyclerViewAdapter? = null

    private val COUNT = 25

    init {
        // Add some sample items.
        //for (i in 1..COUNT) {
        //    addItem(createPlaceholderItem(i))
        //}
        val auth = FirebaseAuth.getInstance()
        val currentUser = auth.currentUser
        if (currentUser != null) {
            val database = FirebaseDatabase.getInstance()
            val backupReference = database.getReference("user-posts").child(currentUser.uid)
                .child("backupHistory")

            val query = backupReference.orderByChild("date")
            query.addValueEventListener(object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {

                    ITEMS.clear()
                    ITEM_MAP.clear()

                    val children = snapshot.children.reversed()
                    for (childSnapshot in children) {
                        val latestData = childSnapshot.value as Map<*, *>
                        addItem( createPlaceholderItem(latestData) )

                    }
                    adapter?.notifyDataSetChanged()
                }

                override fun onCancelled(error: DatabaseError) {
                    TODO("Not yet implemented")
                }
            })
        }

    }

    private fun addItem(item: PlaceholderItem) {
        ITEMS.add(item)
        ITEM_MAP.put(item.id, item)
    }

    private fun createPlaceholderItem(mapdata: Map<*, *>): PlaceholderItem {
        val lastdate = mapdata["date"] as Long
        val date = Date(lastdate)
        val sdf = SimpleDateFormat("yyyy/MM/dd HH:mm:ss", Locale.JAPAN)
        val formattedDate = sdf.format(date)

        return PlaceholderItem(formattedDate, mapdata["deviceName"] as String,mapdata["filename"] as String  )
    }

    private fun createPlaceholderItem(position: Int): PlaceholderItem {
        return PlaceholderItem(position.toString(), "Item " + position, makeDetails(position))
    }

    private fun makeDetails(position: Int): String {
        val builder = StringBuilder()
        builder.append("Details about Item: ").append(position)
        for (i in 0..position - 1) {
            builder.append("\nMore details information here.")
        }
        return builder.toString()
    }

    /**
     * A placeholder item representing a piece of content.
     */
    //data class PlaceholderItem(val id: String, val content: String, val details: String) {
    //    override fun toString(): String = content
    //}

    data class PlaceholderItem(val id: String, val content: String, val details: String) {
        override fun toString(): String = content
    }
}