package org.uoyabause.android.phone

import android.app.Activity
import android.os.Bundle
import android.util.Log
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.preference.PreferenceManager
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.ConsumeParams
import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.Purchase
import com.android.billingclient.api.PurchasesUpdatedListener
import com.android.billingclient.api.QueryProductDetailsParams
import com.android.billingclient.api.QueryPurchasesParams
import com.google.common.collect.ImmutableList
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.withContext
import org.devmiyax.yabasanshiro.R
import org.uoyabause.android.GameSelectPresenter
import org.uoyabause.android.ShowPinInFragment
import org.uoyabause.android.YabauseApplication
import org.uoyabause.android.YabauseStorage
import org.uoyabause.android.billing.BillingClientWrapper
import org.uoyabause.android.phone.placeholder.PlaceholderContent
import org.uoyabause.android.repository.SubscriptionDataRepository
import java.io.File

data class MainState(
    val hasRenewableBasic: Boolean? = false,
    val hasPrepaidBasic: Boolean? = false,
    val hasRenewablePremium: Boolean? = false,
    val hasPrepaidPremium: Boolean? = false,
    val basicProductDetails: ProductDetails? = null,
    val premiumProductDetails: ProductDetails? = null,
    val purchases: List<Purchase>? = null,
)

/**
 * A fragment representing a list of Items.
 */
class BackupBackupItemFragment : Fragment() {

    private var columnCount = 1
    private lateinit var presenter_: GameSelectPresenter


    var billingClient: BillingClientWrapper = BillingClientWrapper(YabauseApplication.appContext)
    private var repo: SubscriptionDataRepository =
        SubscriptionDataRepository(billingClientWrapper = billingClient)
    private val _billingConnectionState = MutableLiveData(false)
    val billingConnectionState: LiveData<Boolean> = _billingConnectionState

    init {
        billingClient.startBillingConnection(billingConnectionState = _billingConnectionState)
    }

    // The productsForSaleFlows object combines all the Product flows into one for emission.
    val productsForSaleFlows = combine(
        repo.basicProductDetails,
        repo.premiumProductDetails
    ) { basicProductDetails,
        premiumProductDetails
        ->
        MainState(
            basicProductDetails = basicProductDetails,
            premiumProductDetails = premiumProductDetails
        )
    }

    // The userCurrentSubscriptionFlow object combines all the possible subscription flows into one
    // for emission.
    private val userCurrentSubscriptionFlow = combine(
        repo.hasRenewableBasic,
        repo.hasPrepaidBasic,
        repo.hasRenewablePremium,
        repo.hasPrepaidPremium
    ) { hasRenewableBasic,
        hasPrepaidBasic,
        hasRenewablePremium,
        hasPrepaidPremium
        ->
        MainState(
            hasRenewableBasic = hasRenewableBasic,
            hasPrepaidBasic = hasPrepaidBasic,
            hasRenewablePremium = hasRenewablePremium,
            hasPrepaidPremium = hasPrepaidPremium
        )
    }

    val currentPurchasesFlow = repo.purchases

    private fun retrieveEligibleOffers(
        offerDetails: MutableList<ProductDetails.SubscriptionOfferDetails>,
        tag: String
    ): List<ProductDetails.SubscriptionOfferDetails> {
        val eligibleOffers = emptyList<ProductDetails.SubscriptionOfferDetails>().toMutableList()
        offerDetails.forEach { offerDetail ->
            if (offerDetail.offerTags.contains(tag)) {
                eligibleOffers.add(offerDetail)
            }
        }

        return eligibleOffers
    }

    private fun leastPricedOfferToken(
        offerDetails: List<ProductDetails.SubscriptionOfferDetails>
    ): String {
        var offerToken = String()
        var leastPricedOffer: ProductDetails.SubscriptionOfferDetails
        var lowestPrice = Int.MAX_VALUE

        if (!offerDetails.isNullOrEmpty()) {
            for (offer in offerDetails) {
                for (price in offer.pricingPhases.pricingPhaseList) {
                    if (price.priceAmountMicros < lowestPrice) {
                        lowestPrice = price.priceAmountMicros.toInt()
                        leastPricedOffer = offer
                        offerToken = leastPricedOffer.offerToken
                    }
                }
            }
        }
        return offerToken
    }

    private fun upDowngradeBillingFlowParamsBuilder(
        productDetails: ProductDetails,
        offerToken: String,
        oldToken: String
    ): BillingFlowParams {
        return BillingFlowParams.newBuilder().setProductDetailsParamsList(
            listOf(
                BillingFlowParams.ProductDetailsParams.newBuilder()
                    .setProductDetails(productDetails)
                    .setOfferToken(offerToken)
                    .build()
            )
        ).setSubscriptionUpdateParams(
            BillingFlowParams.SubscriptionUpdateParams.newBuilder()
                .setOldPurchaseToken(oldToken)
                .setReplaceProrationMode(
                    BillingFlowParams.ProrationMode.IMMEDIATE_AND_CHARGE_FULL_PRICE
                )
                .build()
        ).build()
    }

    private fun billingFlowParamsBuilder(
        productDetails: ProductDetails,
        offerToken: String
    ): BillingFlowParams.Builder {
        return BillingFlowParams.newBuilder().setProductDetailsParamsList(
            listOf(
                BillingFlowParams.ProductDetailsParams.newBuilder()
                    .setProductDetails(productDetails)
                    .setOfferToken(offerToken)
                    .build()
            )
        )
    }

    fun buy(
        productDetails: ProductDetails,
        currentPurchases: List<Purchase>?,
        activity: Activity,
        tag: String
    ) {
        val offers =
            productDetails.subscriptionOfferDetails?.let {
                retrieveEligibleOffers(
                    offerDetails = it,
                    tag = tag.lowercase()
                )
            }
        val offerToken = offers?.let { leastPricedOfferToken(it) }
        val oldPurchaseToken: String

        // Get current purchase. In this app, a user can only have one current purchase at
        // any given time.
        if (!currentPurchases.isNullOrEmpty() &&
            currentPurchases.size == MAX_CURRENT_PURCHASES_ALLOWED
        ) {
/*
            // This either an upgrade, downgrade, or conversion purchase.
            val currentPurchase = currentPurchases.first()

            // Get the token from current purchase.
            oldPurchaseToken = currentPurchase.purchaseToken

            val billingParams = offerToken?.let {
                upDowngradeBillingFlowParamsBuilder(
                    productDetails = productDetails,
                    offerToken = it,
                    oldToken = oldPurchaseToken
                )
            }

            if (billingParams != null) {
                billingClient.launchBillingFlow(
                    activity,
                    billingParams
                )
            }

 */
        } else if (currentPurchases == null) {
            // This is a normal purchase.
            val billingParams = offerToken?.let {
                billingFlowParamsBuilder(
                    productDetails = productDetails,
                    offerToken = it
                )
            }

            if (billingParams != null) {
                billingClient.launchBillingFlow(
                    activity,
                    billingParams.build()
                )
            }
        } else if (!currentPurchases.isNullOrEmpty() &&
            currentPurchases.size > MAX_CURRENT_PURCHASES_ALLOWED
        ) {
            // The developer has allowed users  to have more than 1 purchase, so they need to
            /// implement a logic to find which one to use.
            Log.d(TAG, "User has more than 1 current purchase.")
        }
    }

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


                        val downloadFilename =
                            PlaceholderContent.ITEMS[it].rawdata["filename"] as String
                        val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
                        presenter_.downloadBackupMemory(
                            downloadFilename,
                            currentUser,
                            destinationDirectory,
                            true
                        ) {
                            val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
                            val localFile = File(mem)
                            val localUpdateTime = localFile.lastModified()
                            val database = FirebaseDatabase.getInstance()
                            val backupReference =
                                database.getReference("user-posts").child(currentUser.uid)
                                    .child("backupHistory")

                            val dataref = backupReference.child(PlaceholderContent.ITEMS[it].key)

                            dataref.child("date").setValue(localUpdateTime)
                                .addOnCompleteListener { task ->
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

/*
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
*/


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
        private const val MAX_CURRENT_PURCHASES_ALLOWED = 1
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