package org.uoyabause.android

import android.app.Activity
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.Purchase
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.launch
import org.uoyabause.android.billing.BillingClientWrapper
import org.uoyabause.android.repository.SubscriptionDataRepository

data class MainState(
    val hasRenewableBasic: Boolean? = false,
    val hasPrepaidBasic: Boolean? = false,
    val hasRenewablePremium: Boolean? = false,
    val hasPrepaidPremium: Boolean? = false,
    val basicProductDetails: ProductDetails? = null,
    val premiumProductDetails: ProductDetails? = null,
    val purchases: List<Purchase>? = null,
)

class BillingViewModel : ViewModel(){
    var billingClient: BillingClientWrapper = BillingClientWrapper(YabauseApplication.appContext)
    private var repo: SubscriptionDataRepository =
        SubscriptionDataRepository(billingClientWrapper = billingClient)
    private val _billingConnectionState = MutableLiveData(false)
    val billingConnectionState: LiveData<Boolean> = _billingConnectionState

    // The productsForSaleFlows object combines all the Product flows into one for emission.
    val productsForSaleFlows = repo.basicProductDetails

    // The userCurrentSubscriptionFlow object combines all the possible subscription flows into one
    // for emission.
    val userCurrentSubscriptionFlow = combine(
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

    init {

        billingClient.startBillingConnection(billingConnectionState = _billingConnectionState)

        viewModelScope.launch {
            userCurrentSubscriptionFlow.collectLatest { collectedSubscriptions ->
                when {
                    collectedSubscriptions.hasPrepaidBasic == true -> {
                       Log.d(TAG,"hasPrepaidBasic")
                    }
                    collectedSubscriptions.hasRenewableBasic == true -> {
                        Log.d(TAG,"hasRenewableBasic")
                    }
                    else -> {
                        Log.d(TAG,"else")
                    }
                }
            }
        }

    }


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

    companion object {
        private const val MAX_CURRENT_PURCHASES_ALLOWED = 1
        private const val TAG = "BillingViewModel"
    }

}
