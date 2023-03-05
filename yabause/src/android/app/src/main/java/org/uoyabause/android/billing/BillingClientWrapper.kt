/*
 * Copyright 2022 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.uoyabause.android.billing

import android.app.Activity
import android.content.Context
import android.util.Log
import androidx.lifecycle.MutableLiveData
import com.android.billingclient.api.AcknowledgePurchaseParams
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.ProductDetailsResponseListener
import com.android.billingclient.api.Purchase
import com.android.billingclient.api.PurchasesUpdatedListener
import com.android.billingclient.api.QueryProductDetailsParams
import com.android.billingclient.api.QueryPurchasesParams
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow

/**
 * The [BillingClientWrapper] isolates the Google Play Billing's [BillingClient] methods needed
 * to have a simple implementation and emits responses to the data repository for processing.
 *
 */
class BillingClientWrapper(
    context: Context
) : PurchasesUpdatedListener, ProductDetailsResponseListener {

    // New Subscription ProductDetails
    private val _productWithProductDetails =
        MutableStateFlow<Map<String, ProductDetails>>(emptyMap())
    val productWithProductDetails =
        _productWithProductDetails.asStateFlow()

    // Current Purchases
    private val _purchases =
        MutableStateFlow<List<Purchase>>(listOf())
    val purchases = _purchases.asStateFlow()

    // Tracks new purchases acknowledgement state.
    // Set to true when a purchase is acknowledged and false when not.
    private val _isNewPurchaseAcknowledged = MutableStateFlow(value = false)
    val isNewPurchaseAcknowledged = _isNewPurchaseAcknowledged.asStateFlow()

    // Initialize the BillingClient.
    private val billingClient = BillingClient.newBuilder(context)
        .setListener(this)
        .enablePendingPurchases()
        .build()

    // Establish a connection to Google Play.
    fun startBillingConnection(billingConnectionState: MutableLiveData<Boolean>) {

        billingClient.startConnection(object : BillingClientStateListener {
            override fun onBillingSetupFinished(billingResult: BillingResult) {
                if (billingResult.responseCode == BillingClient.BillingResponseCode.OK) {
                    Log.d(TAG, "Billing response OK")
                    // The BillingClient is ready. You can query purchases and product details here
                    queryPurchases()
                    queryProductDetails()
                    billingConnectionState.postValue(true)
                } else {
                    Log.e(TAG, billingResult.debugMessage)
                }
            }

            override fun onBillingServiceDisconnected() {
                Log.i(TAG, "Billing connection disconnected")
                startBillingConnection(billingConnectionState)
            }
        })
    }

    // Query Google Play Billing for existing purchases.
    // New purchases will be provided to PurchasesUpdatedListener.onPurchasesUpdated().
    fun queryPurchases() {
        if (!billingClient.isReady) {
            Log.e(TAG, "queryPurchases: BillingClient is not ready")
        }
        // Query for existing subscription products that have been purchased.
        billingClient.queryPurchasesAsync(
            QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.SUBS).build()
        ) { billingResult, purchaseList ->
            if (billingResult.responseCode == BillingClient.BillingResponseCode.OK) {
                if (!purchaseList.isNullOrEmpty()) {
                    _purchases.value = purchaseList
                } else {
                    _purchases.value = emptyList()
                }

            } else {
                Log.e(TAG, billingResult.debugMessage)
            }
        }
    }

    // Query Google Play Billing for products available to sell and present them in the UI
    fun queryProductDetails() {
        val params = QueryProductDetailsParams.newBuilder()
        val productList = mutableListOf<QueryProductDetailsParams.Product>()
        for (product in LIST_OF_PRODUCTS) {

            productList.add(
                QueryProductDetailsParams.Product.newBuilder()
                    .setProductId(product)
                    .setProductType(BillingClient.ProductType.SUBS)
                    .build()
            )
        }
        params.setProductList(productList).let { productDetailsParams ->
            Log.i(TAG, "queryProductDetailsAsync")
            billingClient.queryProductDetailsAsync(productDetailsParams.build(), this)
        }
    }

    // [ProductDetailsResponseListener] implementation
    // Listen to response back from [queryProductDetails] and emits the results
    // to [_productWithProductDetails].
    override fun onProductDetailsResponse(
        billingResult: BillingResult,
        productDetailsList: MutableList<ProductDetails>
    ) {
        val responseCode = billingResult.responseCode
        val debugMessage = billingResult.debugMessage
        when (responseCode) {
            BillingClient.BillingResponseCode.OK -> {
                var newMap = emptyMap<String, ProductDetails>()
                if (productDetailsList.isNullOrEmpty()) {
                    Log.e(
                        TAG,
                        "onProductDetailsResponse: " +
                                "Found null or empty ProductDetails. " +
                                "Check to see if the Products you requested are correctly " +
                                "published in the Google Play Console."
                    )
                } else {
                    newMap = productDetailsList.associateBy {
                        it.productId
                    }
                }
                _productWithProductDetails.value = newMap
            }
            else -> {
                Log.i(TAG, "onProductDetailsResponse: $responseCode $debugMessage")
            }
        }
    }

    // Launch Purchase flow
    fun launchBillingFlow(activity: Activity, params: BillingFlowParams) {
        if (!billingClient.isReady) {
            Log.e(TAG, "launchBillingFlow: BillingClient is not ready")
        }
        billingClient.launchBillingFlow(activity, params)

    }

    // PurchasesUpdatedListener that helps handle new purchases returned from the API
    override fun onPurchasesUpdated(
        billingResult: BillingResult,
        purchases: List<Purchase>?
    ) {
        if (billingResult.responseCode == BillingClient.BillingResponseCode.OK
            && !purchases.isNullOrEmpty()
        ) {
            // Post new purchase List to _purchases
            _purchases.value = purchases

            // Then, handle the purchases
            for (purchase in purchases) {
                acknowledgePurchases(purchase)
            }
        } else if (billingResult.responseCode == BillingClient.BillingResponseCode.USER_CANCELED) {
            // Handle an error caused by a user cancelling the purchase flow.
            Log.e(TAG, "User has cancelled")
        } else {
            // Handle any other error codes.
        }
    }

    // Perform new subscription purchases' acknowledgement client side.
    private fun acknowledgePurchases(purchase: Purchase?) {
        purchase?.let {
            if (!it.isAcknowledged) {
                val params = AcknowledgePurchaseParams.newBuilder()
                    .setPurchaseToken(it.purchaseToken)
                    .build()

                billingClient.acknowledgePurchase(
                    params
                ) { billingResult ->
                    if (billingResult.responseCode == BillingClient.BillingResponseCode.OK &&
                        it.purchaseState == Purchase.PurchaseState.PURCHASED
                    ) {
                        _isNewPurchaseAcknowledged.value = true
                    }
                }
            }
        }
    }

    // End Billing connection.
    fun terminateBillingConnection() {
        Log.i(TAG, "Terminating connection")
        billingClient.endConnection()
    }

    companion object {
        private const val TAG = "BillingClient"

        // List of subscription product offerings
        private const val BASIC_SUB = "up_basic_sub"
        private const val PREMIUM_SUB = "up_premium_sub"

        private val LIST_OF_PRODUCTS = listOf(BASIC_SUB, PREMIUM_SUB)
    }
}
