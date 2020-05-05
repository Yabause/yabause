package org.uoyabause.android

import android.annotation.SuppressLint
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.firebase.ui.auth.AuthUI
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import io.reactivex.Observable
import io.reactivex.ObservableEmitter
import io.reactivex.Observer
import io.reactivex.Single
import io.reactivex.SingleOnSubscribe
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.Disposable
import io.reactivex.internal.util.HalfSerializer.onError
import io.reactivex.internal.util.HalfSerializer.onNext
import io.reactivex.observers.DisposableSingleObserver
import io.reactivex.schedulers.Schedulers
import okhttp3.MediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import org.json.JSONObject
import org.uoyabause.uranus.R
import java.util.*
import java.util.concurrent.TimeUnit


class ShowPinInFragment : DialogFragment() {

  private var progress_emitter: ObservableEmitter<String>? = null
  private var observer: Observer<String>? = null
  private var authchecker: DisposableSingleObserver<FirebaseUser>? = null
  lateinit var rootView: View
  private var pinNumber: String? = null
  lateinit private var presenter_: GameSelectPresenter

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    val user = FirebaseAuth.getInstance().currentUser
    if (user == null) {
      authchecker = object : DisposableSingleObserver<FirebaseUser>() {
        override fun onError(e: Throwable) {
          this@ShowPinInFragment.dismiss()
        }

        override fun onSuccess(value: FirebaseUser) {
          getIpdToken(value)
        }
      }
      presenter_.signIn(authchecker)
      return
    } else {
      getIpdToken(user)
    }
  }

  val TIMER_DURATION = 30
  fun getIpdToken(user: FirebaseUser) {
    val baseref = FirebaseDatabase.getInstance().reference
    val baseurl = "/user-posts/" + user.getUid() + "/android_token"
    baseref.child(baseurl).addListenerForSingleValueEvent(
      object : ValueEventListener {
        override fun onCancelled(p0: DatabaseError) {
          Log.d(javaClass.name, p0.message)
        }

        override fun onDataChange(dataSnapshot: DataSnapshot) {
          val ipdtoken = dataSnapshot.getValue() as String
          observer = object : Observer<String> {
            override fun onSubscribe(d: Disposable) {}
            override fun onNext(response: String) {
              val tv = rootView.findViewById<TextView>(R.id.pinin)
              pinNumber = response
              tv.text = response

              val message = rootView.findViewById<TextView>(R.id.message)
              message.text = getString(R.string.enter_pinin)

            }

            @SuppressLint("SetTextI18n")
            override fun onError(e: Throwable) {
              val tv = rootView.findViewById<TextView>(R.id.pinin)
              tv.text = "Error:${e.localizedMessage}"
              pinNumber = null
            }

            override fun onComplete() {
              checkTimeout()
            }
          }
          this@ShowPinInFragment.getPinin(observer!!, ipdtoken)
        }
      }
    )

  }

  // Force close on time out
  @SuppressLint("CheckResult")
  fun checkTimeout() {
    Observable.timer(60L, TimeUnit.SECONDS)
      .observeOn(AndroidSchedulers.mainThread())
      .subscribe {
        Log.d(javaClass.name, "&&&& on timer")
        this.dismiss()
      }
  }

  fun removePinin(observer: Observer<String>, pinNumber: String) {
    Observable.create<String> { emitter ->
      val url = activity!!.getString(R.string.url_getTokenAndDelete)
      val key = activity!!.getString(R.string.key_getTokenAndDelete)
      if (key == "") {
        emitter.onError(Throwable("NO!"))
      } else {
        val client = OkHttpClient()
        val mime = MediaType.parse("application/json; charset=utf-8")
        val requestBody = RequestBody.create(mime, "{ \"key\":\"${pinNumber}\" }")
        val request: Request = Request.Builder()
          .url(url)
          .post(requestBody)
          .addHeader("x-api-key", key)
          .addHeader("Content-Type", "application/json")
          .build()
        val response = client.newCall(request).execute()
        if (response.isSuccessful) {
          emitter.onComplete()
        } else {
          emitter.onError(Throwable(response.message()))
        }
      }
    }.observeOn(AndroidSchedulers.mainThread())
      .subscribeOn(Schedulers.io())
      .subscribe(observer)

  }

  fun getPinin(observer: Observer<String>, token: String) {

    if (progress_emitter != null) {
      return
    }

    pinNumber = null

    Observable.create<String> { emitter ->
      val url = activity!!.getString(R.string.url_getLoginPinIn)
      val key = activity!!.getString(R.string.key_getLoginPinIn)
      if (key == "") {
        emitter.onError(Throwable("NO!"))
      } else {
        progress_emitter = emitter
        val client = OkHttpClient()
        val mime = MediaType.parse("application/json; charset=utf-8")
        val requestBody = RequestBody.create(mime, "{ \"token\":\"" + token + "\" }")
        val request: Request = Request.Builder()
          .url(url)
          .post(requestBody)
          .addHeader("x-api-key", key)
          .addHeader("Content-Type", "application/json")
          .build()
        val response = client.newCall(request).execute()
        if (response.isSuccessful) {

          try {
            val jsonData: String = response.body()!!.string()
            Log.d(javaClass.name, jsonData)
            val Jobject = JSONObject(jsonData)
            val pinin = Jobject.getString("pinin")
            emitter.onNext(pinin)
          } catch (e: Exception) {
            emitter.onError(e)
          }

        } else {
          Log.d(javaClass.name, response.message())
          emitter.onError(Throwable(response.message()))
        }
        progress_emitter = null
        emitter.onComplete()
      }
    }.observeOn(AndroidSchedulers.mainThread())
      .subscribeOn(Schedulers.io())
      .subscribe(observer)
  }

  override fun onCreateView(
    inflater: LayoutInflater, container: ViewGroup?,
    savedInstanceState: Bundle?
  ): View? {
    // Inflate the layout for this fragment
    rootView = inflater.inflate(R.layout.fragment_show_pin_in, container, false)
    val cancel = rootView.findViewById<Button>(R.id.cancel)
    cancel.setOnClickListener {

      if (pinNumber != null) {
        observer = object : Observer<String> {
          override fun onSubscribe(d: Disposable) {}
          override fun onNext(response: String) {}
          override fun onError(e: Throwable) {
            this@ShowPinInFragment.dismiss()
          }

          override fun onComplete() {
            this@ShowPinInFragment.dismiss()
          }
        }
        removePinin(observer!!, pinNumber!!)
        pinNumber = null
      } else {
        this.dismiss()
      }

    }
    return rootView
  }

  override fun onDestroy() {
    if (pinNumber != null) {
      observer = object : Observer<String> {
        override fun onSubscribe(d: Disposable) {}
        override fun onNext(response: String) {}
        override fun onError(e: Throwable) {}
        override fun onComplete() {}
      }
      removePinin(observer!!, pinNumber!!)
    }
    super.onDestroy()
  }

  override fun onResume() {
    super.onResume()
    val window = getDialog()?.getWindow()
    if (window == null) return
    val params = window.getAttributes()
    params.width = 600
    params.height = 600
    window.setAttributes(params)
  }

  companion object {
    @JvmStatic
    fun newInstance(presenter: GameSelectPresenter) =
      ShowPinInFragment().apply {
        presenter_ = presenter
      }
  }
}

