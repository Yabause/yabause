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

package org.uoyabause.android.repository

import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.Purchase
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.filter
import kotlinx.coroutines.flow.map
import org.uoyabause.android.billing.BillingClientWrapper


/**
 * The [SubscriptionDataRepository] processes and tranforms the [StateFlow] data received from
 * the [BillingClientWrapper] into [Flow] data available to the viewModel.
 *
 */
class SubscriptionDataRepository(billingClientWrapper: BillingClientWrapper) {

    // Set to true when a returned purchase is an auto-renewing basic subscription.
    val hasRenewableBasic: Flow<Boolean> = billingClientWrapper.purchases.map { purchaseList ->
        purchaseList.any { purchase ->
            purchase.products.contains(BASIC_SUB) && purchase.isAutoRenewing
        }
    }

    // Set to true when a returned purchase is prepaid basic subscription.
    val hasPrepaidBasic: Flow<Boolean> = billingClientWrapper.purchases.map { purchaseList ->
        purchaseList.any { purchase ->
            !purchase.isAutoRenewing && purchase.products.contains(BASIC_SUB)
        }
    }

    // Set to true when a returned purchases is an auto-renewing premium subscription.
    val hasRenewablePremium: Flow<Boolean> = billingClientWrapper.purchases.map { purchaseList ->
        purchaseList.any { purchase ->
            purchase.products.contains(PREMIUM_SUB) && purchase.isAutoRenewing
        }
    }

    // Set to true when a returned purchase is prepaid premium subscription.
    val hasPrepaidPremium: Flow<Boolean> = billingClientWrapper.purchases.map { purchaseList ->
        purchaseList.any { purchase ->
            !purchase.isAutoRenewing && purchase.products.contains(PREMIUM_SUB)
        }
    }

    // ProductDetails for the basic subscription.
    val basicProductDetails: Flow<ProductDetails> =
        billingClientWrapper.productWithProductDetails.filter {
            it.containsKey(
                BASIC_SUB
            )
        }.map { it[BASIC_SUB]!! }

    // ProductDetails for the premium subscription.
    val premiumProductDetails: Flow<ProductDetails> =
        billingClientWrapper.productWithProductDetails.filter {
            it.containsKey(
                PREMIUM_SUB
            )
        }.map { it[PREMIUM_SUB]!! }

    // List of current purchases returned by the Google PLay Billing client library.
    val purchases: Flow<List<Purchase>> = billingClientWrapper.purchases

    // Set to true when a purchase is acknowledged.
    val isNewPurchaseAcknowledged: Flow<Boolean> = billingClientWrapper.isNewPurchaseAcknowledged

    companion object {
        // List of subscription product offerings
        private const val BASIC_SUB = "up_basic_sub"
        private const val PREMIUM_SUB = "up_premium_sub"
    }
}
