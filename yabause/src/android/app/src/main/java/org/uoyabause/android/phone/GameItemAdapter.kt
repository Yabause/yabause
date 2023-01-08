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
package org.uoyabause.android.phone

import android.content.res.Configuration
import android.os.Build
import android.util.Log
import android.util.TypedValue
import android.view.ContextMenu
import android.view.LayoutInflater
import android.view.MenuInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.widget.PopupMenu
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.bumptech.glide.request.target.DrawableImageViewTarget
import com.frybits.harmony.getHarmonySharedPreferences
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.GameInfo
import org.uoyabause.android.phone.GameItemAdapter.GameViewHolder
import java.io.File


class GameItemAdapter(private val dataSet: MutableList<GameInfo?>?) :
    RecyclerView.Adapter<GameViewHolder>() {

    class GameViewHolder(var rootview: View) :
        RecyclerView.ViewHolder(rootview), View.OnCreateContextMenuListener {
        var textViewName: TextView
        var textViewVersion: TextView
        var imageViewIcon: ImageView
        var menuButton: ImageButton

        init {
            textViewName =
                rootview.findViewById<View>(R.id.textViewName) as TextView
            textViewVersion =
                rootview.findViewById<View>(R.id.textViewVersion) as TextView
            imageViewIcon =
                rootview.findViewById<View>(R.id.imageView) as ImageView

            if (Build.VERSION.SDK_INT > 23) {
                var card = rootview.findViewById<View>(R.id.card_view_main) as CardView
                card.setCardBackgroundColor(rootview.context.getColorStateList(R.color.card_view_selectable))
                card.isFocusable = true
                card.setOnCreateContextMenuListener(this)
            }

            menuButton = rootview.findViewById<View>(R.id.game_card_menu) as ImageButton



            /*
            ViewGroup.LayoutParams lp = imageViewIcon.getLayoutParams();
            lp.width = CARD_WIDTH;
            lp.height = CARD_HEIGHT;
            imageViewIcon.setLayoutParams(lp);
            */
        }

       override fun onCreateContextMenu(
           menu: ContextMenu?,
           v: View?,
           menuInfo: ContextMenu.ContextMenuInfo?,
       ) {
        }

    }


    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): GameViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.cards_layout, parent, false)
        view.setOnClickListener(GameSelectFragmentPhone.myOnClickListener)


        // work here if you need to control height of your items
        // keep in mind that parent is RecyclerView in this case
        //val height = 10;
        //view.minimumHeight = height
/*
        if (parent?.resources?.configuration?.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            try {
                val lp: GridLayoutManager.LayoutParams =
                    view.getLayoutParams() as GridLayoutManager.LayoutParams

                val tv = TypedValue()
                if (parent.context.theme.resolveAttribute(android.R.attr.actionBarSize, tv, true)) {
                    val actionBarHeight = TypedValue.complexToDimensionPixelSize(tv.data,
                        parent.resources.displayMetrics)
                    lp.height = (parent.getMeasuredHeight() - actionBarHeight - 48 )
                    view.setLayoutParams(lp)
                }

            } catch (e: Exception) {

            }
        }else{
            val tv = TypedValue()
            if (parent.context.theme.resolveAttribute(android.R.attr.actionBarSize, tv, true)) {
                val actionBarHeight = TypedValue.complexToDimensionPixelSize(tv.data,
                    parent.resources.displayMetrics)
                view.minimumHeight = (parent.getMeasuredHeight() - actionBarHeight - 48 ) / 4
            }

        }
*/
        return GameViewHolder(view)
    }

    private var mListener: OnItemClickListener? = null
    fun setOnItemClickListener(listener: OnItemClickListener?) {
        mListener = listener
    }

    interface OnItemClickListener {
        fun onItemClick(position: Int, item: GameInfo?, v: View?)
        fun onGameRemoved(item: GameInfo?)
    }

    override fun onBindViewHolder(holder: GameViewHolder, position: Int) {
        val textViewName = holder.textViewName
        val textViewVersion = holder.textViewVersion
        val imageView = holder.imageViewIcon
        val ctx = holder.rootview.context
        val game = dataSet?.get(position)
        if (game != null) {
            textViewName.text = game.game_title
            // textViewVersion.setText(game.product_number);
            var rate = ""
            for (i in 0 until game.rating) {
                rate += "★"
            }
            if (game.device_infomation == "CD-1/1") {
            } else {
                rate += " " + game.device_infomation
            }
            textViewVersion.text = rate
            if (game.image_url != null && game.image_url != "") { // try {


                if (game.image_url!!.startsWith("http")) {
                    Glide.with(ctx)
                        .load(game.image_url)
                        .into(imageView)
                } else {
                    Glide.with(holder.rootview.context)
                        .load(game.image_url?.let { File(it) })
                        .into(imageView)
                }
            } else {
            }
            holder.rootview.setOnClickListener {
                if (null != mListener) {
                    mListener!!.onItemClick(position, dataSet?.get(position), null)
                }
            }
        }

        holder.menuButton.setOnClickListener(View.OnClickListener {
            showPopupMenu(
                holder.menuButton,
                position
            )
        })
    }

    private fun showPopupMenu(
        view: View,
        position: Int,
    ) { // inflate menu
        val popup = PopupMenu(view.context, view)
        val inflater: MenuInflater = popup.getMenuInflater()
        inflater.inflate(R.menu.game_item_popup_menu, popup.getMenu())
        // popup.setOnMenuItemClickListener(MyMenuItemClickListener(position))
        popup.setOnMenuItemClickListener(PopupMenu.OnMenuItemClickListener {
            when (it.itemId) {
                R.id.delete -> {
/*
                    val prefs = view.context.getSharedPreferences("private", Context.MODE_PRIVATE)
                    val hasDonated = prefs.getBoolean("donated", false)

                    if (!BuildConfig.BUILD_TYPE.equals("pro") && !hasDonated) {
                        AlertDialog.Builder(view.context)
                                .setTitle(R.string.not_available)
                                .setMessage(R.string.only_pro_version)
                                .setPositiveButton(R.string.got_it) { _, _ ->
                                    val url = "https://play.google.com/store/apps/details?id=org.uoyabause.uranus.pro"
                                    val intent = Intent(Intent.ACTION_VIEW)
                                    intent.data = Uri.parse(url)
                                    intent.setPackage("com.android.vending")
                                    view.context.startActivity(intent)
                                }.setNegativeButton(R.string.cancel) { _, _ ->
                                }
                                .show()
                    } else {

 */
                        Log.d("textext", "R.id.delete is selected")
                        AlertDialog.Builder(view.context)
                                .setTitle(R.string.delete_confirm_title)
                                .setMessage(R.string.delete_confirm)
                                .setPositiveButton(R.string.ok) { _, _ ->

                                    var game_info = dataSet?.get(position)!!

                                    dataSet.removeAt(position)

                                    mListener?.onGameRemoved(game_info)
                                    game_info.removeInstance()

                                    notifyItemRemoved(position)
                                }
                                .setNegativeButton(R.string.no) { _, _ ->
                                }
                                .show()
                    }
                else -> {
                    Log.d("textext", "Unknown value (value = $it.itemId)")
                }
            }
            true
        })
        popup.show()
    }

    override fun getItemCount(): Int {
        return dataSet!!.size
    }

    fun removeItem(id: Long) {
        val index = dataSet?.indexOfFirst({ it!!.id == id })
        if (index != null && index != -1) {
            dataSet?.removeAt(index)
            notifyItemRemoved(index)
        }
    }

    companion object {
        private const val CARD_WIDTH = 320
        private const val CARD_HEIGHT = 224
    }
}
