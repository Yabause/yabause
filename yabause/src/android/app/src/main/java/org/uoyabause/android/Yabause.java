/*  Copyright 2011-2013 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
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


package org.uoyabause.android;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.Runnable;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.io.File;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import android.app.ProgressDialog;
import android.app.UiModeManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.hardware.input.InputManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.*;

import com.firebase.ui.auth.AuthUI;
import com.firebase.ui.auth.IdpResponse;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.material.snackbar.Snackbar;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.AppCompatActivity;
import androidx.transition.Fade;

import android.util.Base64;
import android.util.Log;
import android.app.Dialog;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONObject;
import org.uoyabause.android.backup.TabBackupFragment;
import com.activeandroid.query.Select;
import com.activeandroid.util.IOUtils;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.analytics.Tracker;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.google.firebase.analytics.FirebaseAnalytics;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.FirebaseDatabase;
import com.google.android.material.navigation.NavigationView;
import com.google.firebase.storage.FileDownloadTask;
import com.google.firebase.storage.FirebaseStorage;
import com.google.firebase.storage.StorageReference;
import com.google.firebase.storage.UploadTask;
import org.uoyabause.android.cheat.TabCheatFragment;
import org.uoyabause.uranus.BuildConfig;
import org.uoyabause.uranus.R;
import org.uoyabause.uranus.StartupActivity;
import io.reactivex.Observable;
import io.reactivex.Observer;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.ObservableOnSubscribe;
import io.reactivex.ObservableEmitter;
import io.reactivex.schedulers.Schedulers;

import static android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
import static android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
import static android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
import static org.uoyabause.android.SelInputDeviceFragment.PLAYER1;
import static org.uoyabause.android.SelInputDeviceFragment.PLAYER2;

class YabauseHandler extends Handler {
  private Yabause yabause;

  public YabauseHandler(Yabause yabause) {
    this.yabause = yabause;
  }

  public void handleMessage(Message msg) {
    yabause.showDialog(msg.what, msg.getData());
  }
}

//AppCompatActivity
public class Yabause extends AppCompatActivity implements
    FileDialog.FileSelectedListener,
    NavigationView.OnNavigationItemSelectedListener,
    SelInputDeviceFragment.InputDeviceListener,
    PadTestFragment.PadTestListener,
    InputSettingFragment.InputSettingListener,
    InputManager.InputDeviceListener,
    PadManager.ShowMenuListener {
  private static final String TAG = "Yabause";
  private YabauseRunnable yabauseThread;
  private YabauseHandler handler;
  private YabauseAudio audio;
  private String biospath;
  private String gamepath;
  private int carttype;
  private PadManager padm;
  private int video_interface;
  private boolean waiting_reault = false;
  private Tracker mTracker;
  private int tray_state = 0;
  DrawerLayout mDrawerLayout;
  NavigationView mNavigationView;
  AdView adView = null;
  private FirebaseAnalytics mFirebaseAnalytics;
  final int PAD_SETTING = 0;
  InputManager mInputManager;
  final int RC_SIGN_IN = 0x8010;

  private ProgressDialog mProgressDialog;
  private Boolean isShowProgress;
  private String gameCode;
  private String testCase = "";

  void print(String msg) {
    StackTraceElement calledClass = Thread.currentThread().getStackTrace()[3];
    Log.d(calledClass.getFileName() + ":"
        + calledClass.getLineNumber(), msg);
  }

  public void showDialog() {
    mProgressDialog = new ProgressDialog(this);
    mProgressDialog.setMessage("Sending...");
    mProgressDialog.show();
    waiting_reault = true;
    isShowProgress = true;
  }

  public void dismissDialog() {
    mProgressDialog.dismiss();
    mProgressDialog = null;
    isShowProgress = false;
    waiting_reault = false;
    final DrawerLayout layout = (DrawerLayout) findViewById(R.id.drawer_layout);
    switch (_report_status) {
      case REPORT_STATE_INIT:
        Snackbar.make(layout, "Fail to send your report. internal error", Snackbar.LENGTH_SHORT).show();
        break;
      case REPORT_STATE_SUCCESS:
        Snackbar.make(layout, "Success to send your report. Thank you for your collaboration.", Snackbar.LENGTH_SHORT).show();
        break;
      case REPORT_STATE_FAIL_DUPE:
        Snackbar.make(layout, "Fail to send your report. You've sent a report for same game, same device and same vesion.", Snackbar.LENGTH_SHORT).show();
        break;
      case REPORT_STATE_FAIL_CONNECTION:
        Snackbar.make(layout, "Fail to send your report. Server is down.", Snackbar.LENGTH_SHORT).show();
        break;
      case REPORT_STATE_FAIL_AUTH:
        Snackbar.make(layout, "Fail to send your report. Authorizing is failed.", Snackbar.LENGTH_SHORT).show();
        break;

    }
    toggleMenu();
  }

  /**
   * Called when the activity is first created.
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
    boolean lock_landscape = sharedPref.getBoolean("pref_landscape", false);
    if (lock_landscape == true) {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    } else {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
    }

    mInputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);

    System.gc();

    mFirebaseAnalytics = FirebaseAnalytics.getInstance(this);

    YabauseApplication application = (YabauseApplication) getApplication();
    mTracker = application.getDefaultTracker();

    setContentView(R.layout.main);
    try {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        getWindow().setSustainedPerformanceMode(true);
      }
    } catch (Exception e) {
      // Do Nothing
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
    }
    if( sharedPref.getBoolean("pref_immersive_mode", false)) {
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
    updateViewLayout(getResources().getConfiguration().orientation);

    mNavigationView = (NavigationView) findViewById(R.id.nav_view);
    mNavigationView.setNavigationItemSelectedListener(this);
    Menu menu = mNavigationView.getMenu();

    if(BuildConfig.BUILD_TYPE != "debug" ) {
      MenuItem rec = menu.findItem(R.id.record);
      if( rec != null ){
        rec.setVisible(false);
      }
      MenuItem play = menu.findItem(R.id.play);
      if( play != null ){
        play.setVisible(false);
      }
    }

    DrawerLayout.DrawerListener drawerListener = new DrawerLayout.DrawerListener() {
      @Override
      public void onDrawerSlide(View view, float v) {

      }

      @Override
      public void onDrawerOpened(View view) {
        if (!menu_showing) {
          menu_showing = true;
          YabauseRunnable.pause();
          audio.mute(audio.SYSTEM);
          String name = YabauseRunnable.getGameTitle();
          TextView tx = (TextView) findViewById(R.id.menu_title);
          if (tx != null) {
            tx.setText(name);
          }
          if( !BuildConfig.BUILD_TYPE.equals("pro") ) {
            SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
            Boolean hasDonated = prefs.getBoolean("donated", false);
            if (hasDonated == false) {
              if (adView != null) {
                LinearLayout lp = (LinearLayout) findViewById(R.id.navilayer);
                if (lp != null) {
                  final int mCount = lp.getChildCount();
                  boolean find = false;
                  for (int i = 0; i < mCount; ++i) {
                    final View mChild = lp.getChildAt(i);
                    if (mChild == adView) {
                      find = true;
                    }
                  }
                  if (!find) {
                    lp.addView(adView);
                  }
                  AdRequest adRequest = new AdRequest.Builder().addTestDevice("303A789B146C169D4BDB5652D928FF8E").build();
                  adView.loadAd(adRequest);
                }
              }
            }
          }
        }
      }

      @Override
      public void onDrawerClosed(View view) {

        if (waiting_reault == false && menu_showing == true) {
          menu_showing = false;
          YabauseRunnable.resume();
          audio.unmute(audio.SYSTEM);
        }

      }

      @Override
      public void onDrawerStateChanged(int i) {
      }
    };
    this.mDrawerLayout.setDrawerListener(drawerListener);

    audio = new YabauseAudio(this);
    Intent intent = getIntent();

    Bundle bundle = intent.getExtras();
    if (bundle != null) {
      for (String key : bundle.keySet()) {
        Log.e(TAG, key + " : " + (bundle.get(key) != null ? bundle.get(key) : "NULL"));
      }
    }

    String game = intent.getStringExtra("org.uoyabause.android.FileName");
    if (game != null && game.length() > 0) {
      YabauseStorage storage = YabauseStorage.getStorage();
      gamepath = storage.getGamePath(game);
    } else
      gamepath = "";

    String exgame = intent.getStringExtra("org.uoyabause.android.FileNameEx");
    if (exgame != null) {
      gamepath = exgame;
    }

    Log.d(TAG,"File is " + gamepath );

    String gameCode = intent.getStringExtra("org.uoyabause.android.gamecode");
    if (gameCode != null) {
      this.gameCode = gameCode;
    }else {
      GameInfo gameinfo = GameInfo.getFromFileName(gamepath);
      if( gameinfo != null ) {
        this.gameCode = gameinfo.product_number;
      }else{
        if (gamepath.toUpperCase().endsWith("CUE")) {
          gameinfo = GameInfo.genGameInfoFromCUE(gamepath);
        } else if (gamepath.toUpperCase().endsWith("MDS")) {
          gameinfo = GameInfo.genGameInfoFromMDS(gamepath);
        } else if (gamepath.toUpperCase().endsWith("CCD")) {
          gameinfo = GameInfo.genGameInfoFromMDS(gamepath);
        } else if (gamepath.toUpperCase().endsWith("CHD")) {
          gameinfo = GameInfo.genGameInfoFromCHD(gamepath);
        } else {
          gameinfo = GameInfo.genGameInfoFromIso(gamepath);
        }
        if( gameinfo != null ) {
          this.gameCode = gameinfo.product_number;
        }
      }
    }

    testCase = intent.getStringExtra("TestCase");

    PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
    if( this.gameCode != null ) {
      readPreferences(this.gameCode);
    }

    padm = PadManager.getPadManager();
    padm.loadSettings();
    padm.setShowMenulistener(this);
    waiting_reault = false;

    handler = new YabauseHandler(this);
    yabauseThread = new YabauseRunnable(this);

    UiModeManager uiModeManager = (UiModeManager) getSystemService(Context.UI_MODE_SERVICE);
    if (uiModeManager.getCurrentModeType() != Configuration.UI_MODE_TYPE_TELEVISION && !BuildConfig.BUILD_TYPE.equals("pro") ) {
      SharedPreferences prefs = getSharedPreferences("private", Context.MODE_PRIVATE);
      Boolean hasDonated = prefs.getBoolean("donated", false);
      if (hasDonated) {
        adView = null;
      } else {
        adView = new AdView(this);
        adView.setAdUnitId(getString(R.string.banner_ad_unit_id2));
        adView.setAdSize(AdSize.BANNER);
        AdRequest adRequest = new AdRequest.Builder().build();
        adView.loadAd(adRequest);
        adView.setAdListener(new AdListener() {
          @Override
          public void onAdOpened() {
            // Save app state before going to the ad overlay.
          }
        });
      }
    } else {
      adView = null;
    }
  }

  void updateViewLayout( int orientation ){

    getWindow().setStatusBarColor(getResources().getColor(R.color.black));

    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
    int immersiveFlags = 0;
    if(sharedPref.getBoolean("pref_immersive_mode", false)){
      immersiveFlags = (View.SYSTEM_UI_FLAG_HIDE_NAVIGATION  | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    View decorView = getWindow().getDecorView();
    if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | immersiveFlags
                        | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }else if (orientation == Configuration.ORIENTATION_PORTRAIT) {
        decorView.setSystemUiVisibility( View.SYSTEM_UI_FLAG_LAYOUT_STABLE | immersiveFlags  );
    }
  }

  public void onConfigurationChanged(Configuration _newConfig) {
    updateViewLayout(_newConfig.orientation);
    super.onConfigurationChanged(_newConfig);
  }

  ObservableEmitter<FirebaseUser> loginEmitter;

  int checkAuth( Observer<FirebaseUser> loginObserver ){

    Observable.create( new ObservableOnSubscribe<FirebaseUser>() {
      @Override
      public void subscribe(ObservableEmitter<FirebaseUser> emitter) throws Exception {

        loginEmitter = null;
        FirebaseAuth auth = FirebaseAuth.getInstance();
        FirebaseUser user = auth.getCurrentUser();
        if (user != null) {
          emitter.onNext(user);
          emitter.onComplete();
          return;
        }

        loginEmitter = emitter;
        startActivityForResult(
                AuthUI.getInstance()
                        .createSignInIntentBuilder()
                        .setAvailableProviders( Arrays.asList(
                                new AuthUI.IdpConfig.GoogleBuilder().build()))
                        .build(),
                RC_SIGN_IN);
      }
    })
            .subscribeOn(AndroidSchedulers.mainThread())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(loginObserver);

    return 0;
  }


  public void setSaveStateObserver( Observer<String> saveStateObserver ){

    Observable.create( new ObservableOnSubscribe<String>() {
      @Override
      public void subscribe( final ObservableEmitter<String> emitter) throws Exception {
        FirebaseAuth auth = FirebaseAuth.getInstance();
        FirebaseUser user = auth.getCurrentUser();
        if (user == null) {
          emitter.onError( new Exception("not login") );
          return;
        }
        String save_path = YabauseStorage.getStorage().getStateSavePath();
        String current_gamecode = YabauseRunnable.getCurrentGameCode();
        File save_root = new File(YabauseStorage.getStorage().getStateSavePath(), current_gamecode);
        if (!save_root.exists()) save_root.mkdir();
        final String save_filename = YabauseRunnable.savestate_compress(save_path + current_gamecode);
        if (!save_filename.equals("")) {
          FirebaseStorage storage = FirebaseStorage.getInstance();
          StorageReference storage_ref = storage.getReference();
          StorageReference base = storage_ref.child(user.getUid());
          StorageReference backup = base.child("state");
          StorageReference fileref = backup.child(current_gamecode);
          Uri file = Uri.fromFile(new File(save_filename));
          UploadTask tsk = fileref.putFile(file);
          tsk.addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception exception) {
              File file = new File(save_filename);
              if (file.exists()) {
                file.delete();
              }
              emitter.onError( exception );
            }
          }).addOnSuccessListener(new OnSuccessListener<UploadTask.TaskSnapshot>() {
            @Override
            public void onSuccess(UploadTask.TaskSnapshot taskSnapshot) {
              File file = new File(save_filename);
              if (file.exists()) {
                file.delete();
              }
              emitter.onNext("OK");
              emitter.onComplete();
            }
          });
        }
      }
    }).subscribeOn(Schedulers.computation())
      .observeOn(AndroidSchedulers.mainThread())
      .subscribe(saveStateObserver);
  }

  public void setLoadStateObserver( Observer<String> loadStateObserver ){

    Observable.create( new ObservableOnSubscribe<String>() {
      @Override
      public void subscribe( final ObservableEmitter<String> emitter) throws Exception {
        FirebaseAuth auth = FirebaseAuth.getInstance();
        FirebaseUser user = auth.getCurrentUser();
        if( user == null ) {
          emitter.onError( new Exception("not login") );
          return;
        }
        String save_path = YabauseStorage.getStorage().getStateSavePath();
        String current_gamecode = YabauseRunnable.getCurrentGameCode();
        FirebaseStorage storage = FirebaseStorage.getInstance();
        StorageReference storage_ref = storage.getReference();
        StorageReference base = storage_ref.child(user.getUid());
        StorageReference backup = base.child("state");
        StorageReference fileref = backup.child(current_gamecode);
        try{
          final File localFile = File.createTempFile("currentstate", "bin.z");
          fileref.getFile(localFile).addOnSuccessListener(new OnSuccessListener<FileDownloadTask.TaskSnapshot>() {
            @Override
            public void onSuccess(FileDownloadTask.TaskSnapshot taskSnapshot) {
              try{
                if (localFile.exists()) {
                  YabauseRunnable.loadstate_compress(localFile.getAbsolutePath());
                  localFile.delete();
                }
                emitter.onNext("OK");
                emitter.onComplete();

              }catch(Exception e){
                emitter.onError( e );
                return;
              }
            }
          }).addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception exception) {
              localFile.delete();
              emitter.onError( exception );
              return;
            }
          });
        } catch (IOException e) {
          emitter.onError( e );
        }

      }
    }).subscribeOn(Schedulers.computation())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe(loadStateObserver);
  }

  @Override
  public boolean onNavigationItemSelected(MenuItem item) {
    // Handle action bar item clicks here. The action bar will
    // automatically handle clicks on the Home/Up button, so long
    // as you specify a parent activity in AndroidManifest.xml.
    int id = item.getItemId();

    Bundle bundle = new Bundle();
    String title = item.getTitle().toString();
    bundle.putString(FirebaseAnalytics.Param.ITEM_ID, "MENU");
    bundle.putString(FirebaseAnalytics.Param.ITEM_NAME, title);
    mFirebaseAnalytics.logEvent(
        FirebaseAnalytics.Event.SELECT_CONTENT, bundle
    );

    switch (id) {
            /*
            case R.id.save_screen:{
                DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
                Date date = new Date();

                String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
                        + YabauseRunnable.getCurrentGameCode() +
                        dateFormat.format(date) + ".png";

                if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
                    break;
                }

                waiting_reault = true;
                Intent shareIntent = new Intent();
                shareIntent.setAction(Intent.ACTION_SEND);
                ContentResolver cr = getContentResolver();
                ContentValues cv = new ContentValues();
                cv.put(MediaStore.Images.Media.TITLE, YabauseRunnable.getCurrentGameCode());
                cv.put(MediaStore.Images.Media.DISPLAY_NAME, YabauseRunnable.getCurrentGameCode());
                cv.put(MediaStore.Images.Media.DATE_TAKEN, System.currentTimeMillis());
                cv.put(MediaStore.Images.Media.MIME_TYPE, "image/png");
                cv.put(MediaStore.Images.Media.DATA, screen_shot_save_path);
                Uri uri = cr.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, cv);
                shareIntent.putExtra(Intent.EXTRA_STREAM, uri);
                shareIntent.setType("image/png");
                startActivityForResult(Intent.createChooser(shareIntent, "share screenshot to"), 0x01);
            }
            break;
            */
      case R.id.reset:
        YabauseRunnable.reset();
        break;
      case R.id.report:
        startReport();
        break;
      case R.id.gametitle: {
        String save_path = YabauseStorage.getStorage().getScreenshotPath();
        String current_gamecode = YabauseRunnable.getCurrentGameCode();
        String screen_shot_save_path = save_path + current_gamecode + ".png";
        if (YabauseRunnable.screenshot(screen_shot_save_path) == 0) {
          try {
            GameInfo gi = new Select().from(GameInfo.class).where("product_number = ?", current_gamecode).executeSingle();
            if (gi != null) {
              gi.image_url = screen_shot_save_path;
              gi.save();
            }
          } catch (Exception e) {
            Log.e(TAG, e.getLocalizedMessage());
          }
        }
      }
      break;
      case R.id.save_state: {
        String save_path = YabauseStorage.getStorage().getStateSavePath();
        String current_gamecode = YabauseRunnable.getCurrentGameCode();
        File save_root = new File(YabauseStorage.getStorage().getStateSavePath(), current_gamecode);
        if (!save_root.exists()) save_root.mkdir();
        String save_filename = YabauseRunnable.savestate(save_path + current_gamecode);
        if ( !save_filename.equals("") ) {
          int point = save_filename.lastIndexOf(".");
          if (point != -1) {
            save_filename = save_filename.substring(0, point);
          }
          String screen_shot_save_path = save_filename + ".png";
          if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
            Snackbar.make(this.mDrawerLayout, "Failed to save the current state", Snackbar.LENGTH_SHORT).show();
          } else {
            Snackbar.make(this.mDrawerLayout, "Current state is saved as " + save_filename, Snackbar.LENGTH_LONG).show();
          }
        } else {
          Snackbar.make(this.mDrawerLayout, "Failed to save the current state", Snackbar.LENGTH_SHORT).show();
        }

        StateListFragment.checkMaxFileCount(save_path + current_gamecode);

      }
      break;

      case R.id.save_state_cloud: {

        if( YabauseApplication.checkDonated(this) != 0 ){
          break;
        }
        waiting_reault = true;
        Observer loginobserver = new Observer<FirebaseUser>() {

          @Override
          public void onSubscribe(Disposable d) {

          }

          @Override
          public void onNext(FirebaseUser firebaseUser) {

          }

          @Override
          public void onError(Throwable e) {
            waiting_reault = false;
            toggleMenu();
            Snackbar.make(Yabause.this.mDrawerLayout, "Failed to login \n" + e.getLocalizedMessage(), Snackbar.LENGTH_LONG).show();
          }

          @Override
          public void onComplete() {
            mProgressDialog = new ProgressDialog(Yabause.this);
            mProgressDialog.setMessage("Sending...");
            mProgressDialog.show();
            isShowProgress = true;
            Observer observer = new Observer<String>() {
              @Override
              public void onSubscribe(Disposable d) {
              }

              @Override
              public void onNext(String response) {
              }

              @Override
              public void onError(Throwable e) {
                mProgressDialog.dismiss();
                isShowProgress = false;
                waiting_reault = false;
                toggleMenu();
                Snackbar.make(Yabause.this.mDrawerLayout, "Failed to save the current state to cloud", Snackbar.LENGTH_LONG).show();
              }

              @Override
              public void onComplete() {
                mProgressDialog.dismiss();
                isShowProgress = false;
                waiting_reault = false;
                toggleMenu();
                Snackbar.make(Yabause.this.mDrawerLayout, "Success to save the current state to cloud", Snackbar.LENGTH_SHORT).show();
              }
            };
            setSaveStateObserver(observer);
          }
        };
        checkAuth(loginobserver);
      }
      break;

      case R.id.load_state_cloud: {

        if( YabauseApplication.checkDonated(this) != 0 ){
          break;
        }
        waiting_reault = true;
        Observer loginobserver = new Observer<FirebaseUser>() {

          @Override
          public void onSubscribe(Disposable d) {

          }

          @Override
          public void onNext(FirebaseUser firebaseUser) {

          }

          @Override
          public void onError(Throwable e) {
            waiting_reault = false;
            toggleMenu();
            Snackbar.make(Yabause.this.mDrawerLayout, "Failed to login \n" + e.getLocalizedMessage(), Snackbar.LENGTH_LONG).show();
          }

          @Override
          public void onComplete() {
            mProgressDialog = new ProgressDialog(Yabause.this);
            mProgressDialog.setMessage("Loading...");
            mProgressDialog.show();
            isShowProgress = true;
            Observer observer = new Observer<String>() {
              @Override
              public void onSubscribe(Disposable d) {
              }
              @Override
              public void onNext(String response) {
              }
              @Override
              public void onError(Throwable e) {
                mProgressDialog.dismiss();
                isShowProgress = false;
                waiting_reault = false;
                toggleMenu();
                Snackbar.make(Yabause.this.mDrawerLayout, "Failed to load the state from cloud \n" + e.getLocalizedMessage(), Snackbar.LENGTH_SHORT).show();
              }
              @Override
              public void onComplete() {
                mProgressDialog.dismiss();
                isShowProgress = false;
                waiting_reault = false;
                toggleMenu();
                Snackbar.make(Yabause.this.mDrawerLayout, "Success to load the state from cloud", Snackbar.LENGTH_SHORT).show();
              }
            };
            setLoadStateObserver(observer);
          }
        };
        checkAuth(loginobserver);
      }
      break;

      case R.id.load_state: {
        //String save_path = YabauseStorage.getStorage().getStateSavePath();
        //YabauseRunnable.loadstate(save_path);
        String basepath;
        String save_path = YabauseStorage.getStorage().getStateSavePath();
        String current_gamecode = YabauseRunnable.getCurrentGameCode();
        basepath = save_path + current_gamecode;
        waiting_reault = true;
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        StateListFragment fragment = new StateListFragment();
        fragment.setBasePath(basepath);
        transaction.replace(R.id.ext_fragment, fragment, StateListFragment.TAG);
        transaction.show(fragment);
        transaction.commit();
      }

      case R.id.record: {
        YabauseRunnable.record( YabauseStorage.getStorage().getRecordPath() );
      }
      break;

      case R.id.play: {
        //YabauseRunnable.play( YabauseStorage.getStorage().getRecordPath() );
      }
      break;

      case R.id.menu_item_backup: {
        waiting_reault = true;
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        TabBackupFragment fragment = TabBackupFragment.newInstance("hoge", "hoge");
        //fragment.setBasePath(basepath);
        transaction.replace(R.id.ext_fragment, fragment, TabBackupFragment.TAG);
        transaction.show(fragment);
        transaction.commit();
      }
      break;
      case R.id.menu_item_pad_device: {
        waiting_reault = true;
        SelInputDeviceFragment newFragment = new SelInputDeviceFragment();
        newFragment.setTarget(PLAYER1);
        newFragment.setListener(this);
        newFragment.show(getFragmentManager(), "InputDevice");
      }
      break;
      case R.id.menu_item_pad_setting: {
        waiting_reault = true;
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        if (padm.getPlayer1InputDevice() == -1) { // Using pad?
          FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
          PadTestFragment fragment = PadTestFragment.newInstance("hoge", "hoge");
          fragment.setListener(this);
          transaction.replace(R.id.ext_fragment, fragment, PadTestFragment.TAG);
          transaction.show(fragment);
          transaction.commit();
        } else {
          InputSettingFragment newFragment = new InputSettingFragment();
          newFragment.setPlayerAndFilename(PLAYER1, "keymap");
          newFragment.setListener(this);
          newFragment.show(getFragmentManager(), "InputSettings");
        }
      }
      break;
      case R.id.menu_item_pad_device_p2: {
        waiting_reault = true;
        SelInputDeviceFragment newFragment = new SelInputDeviceFragment();
        newFragment.setTarget(PLAYER2);
        newFragment.setListener(this);
        newFragment.show(getFragmentManager(), "InputDevice");
      }
      break;
      case R.id.menu_item_pad_setting_p2: {
        waiting_reault = true;
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        if (padm.getPlayer2InputDevice() != -1) { // Using pad?
          InputSettingFragment newFragment = new InputSettingFragment();
          newFragment.setPlayerAndFilename(PLAYER2, "keymap_player2");
          newFragment.setListener(this);
          newFragment.show(getFragmentManager(), "InputSettings");
        }
      }
      break;

      case R.id.button_open_cd: {
        if (tray_state == 0) {
          YabauseRunnable.openTray();
          item.setTitle(getString(R.string.close_cd_tray));
          tray_state = 1;
        } else {
          item.setTitle(getString(R.string.open_cd_tray));
          tray_state = 0;
          File file = new File(gamepath);
          String path = file.getParent();
          FileDialog fd = new FileDialog(Yabause.this, path);
          fd.addFileListener(Yabause.this);
          fd.showDialog();
        }
      }
      break;
      case R.id.pad_mode: {
        YabausePad padv = (YabausePad) findViewById(R.id.yabause_pad);
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
        boolean mode = false;
        if ( sharedPref.getBoolean("pref_analog_pad", false) ) {
          item.setChecked(false);
          Yabause.this.padm.setAnalogMode(PadManager.MODE_HAT);
          YabauseRunnable.switch_padmode(PadManager.MODE_HAT);
          padv.setPadMode(PadManager.MODE_HAT);

          mode = false;
        } else {
          item.setChecked(true);
          Yabause.this.padm.setAnalogMode(PadManager.MODE_ANALOG);
          YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG);
          padv.setPadMode(PadManager.MODE_ANALOG);
          mode = true;
        }

        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean("pref_analog_pad", mode);
        editor.apply();
        Yabause.this.toggleMenu();
      }
      break;
      case R.id.pad_mode_p2: {
        boolean mode = false;
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(Yabause.this);
        if ( sharedPref.getBoolean("pref_analog_pad2", false) ) {
          item.setChecked(false);
          Yabause.this.padm.setAnalogMode2(PadManager.MODE_HAT);
          YabauseRunnable.switch_padmode2(PadManager.MODE_HAT);
          mode = false;
        } else {
          item.setChecked(true);
          Yabause.this.padm.setAnalogMode2(PadManager.MODE_ANALOG);
          YabauseRunnable.switch_padmode2(PadManager.MODE_ANALOG);
          mode = true;
        }

        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean("pref_analog_pad2", mode);
        editor.apply();
        Yabause.this.toggleMenu();
      }
      break;
      case R.id.menu_item_cheat: {
        waiting_reault = true;
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        TabCheatFragment fragment = TabCheatFragment.newInstance(YabauseRunnable.getCurrentGameCode(), this.cheat_codes);
        transaction.replace(R.id.ext_fragment, fragment, TabCheatFragment.TAG);
        transaction.show(fragment);
        transaction.commit();
      }
      break;
      case R.id.exit: {
        YabauseRunnable.deinit();
        try {
          Thread.sleep(1000);
        } catch (InterruptedException e) {

        }
        finish();
        android.os.Process.killProcess(android.os.Process.myPid());
      }
      break;
      case R.id.menu_in_game_setting:{
        waiting_reault = true;
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();

        final String currentGameCode = YabauseRunnable.getCurrentGameCode();

        final InGamePreference fragment = new InGamePreference(currentGameCode);

        Observer observer = new Observer<String>() {
          //GithubRepositoryApiCompleteEventEntity eventResult = new GithubRepositoryApiCompleteEventEntity();

          @Override
          public void onSubscribe(Disposable d) {
          }

          @Override
          public void onNext(String response) {
          }

          @Override
          public void onError(Throwable e) {
            waiting_reault = false;
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
          }

          @Override
          public void onComplete() {
            //getSupportFragmentManager().popBackStack();

            YabauseRunnable.lockGL();
            SharedPreferences gamePreference = getSharedPreferences(Yabause.this.gameCode,Context.MODE_PRIVATE);
            YabauseRunnable.enableRotateScreen(gamePreference.getBoolean("pref_rotate_screen", false) ? 1 : 0);
            boolean fps = gamePreference.getBoolean("pref_fps", false);
            YabauseRunnable.enableFPS(fps ? 1 : 0);
            Log.d(TAG, "enable FPS " + fps);

            String sPg = gamePreference.getString("pref_polygon_generation", "0");
            Integer iPg = new Integer(sPg);
            YabauseRunnable.setPolygonGenerationMode(iPg);
            Log.d(TAG, "setPolygonGenerationMode " + iPg.toString());

            boolean frameskip = gamePreference.getBoolean("pref_frameskip", true);
            YabauseRunnable.enableFrameskip(frameskip ? 1 : 0);
            Log.d(TAG, "enable enableFrameskip " + frameskip);

            Integer sKa = new Integer(gamePreference.getString("pref_polygon_generation", "0"));
            YabauseRunnable.setPolygonGenerationMode(sKa);

            YabauseRunnable.setAspectRateMode(new Integer(gamePreference.getString("pref_aspect_rate", "0")));


            Integer resolution_setting = new Integer(gamePreference.getString("pref_resolution", "0"));
            YabauseRunnable.setResolutionMode(resolution_setting);

            YabauseRunnable.enableComputeShader(gamePreference.getBoolean("pref_use_compute_shader", false) ? 1 : 0);
            Integer rbg_resolution_setting = new Integer(gamePreference.getString("pref_rbg_resolution", "0"));
            YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting);
            YabauseRunnable.unlockGL();

            // Recreate Yabause View
            View v = findViewById(R.id.yabause_view);
            FrameLayout layout = findViewById(R.id.content_main);
            layout.removeView(v);
            YabauseView yv = new YabauseView(Yabause.this);
            yv.setId(R.id.yabause_view);
            layout.addView(yv,0);


            Fade exitFade = new Fade();
            exitFade.setDuration(500);
            fragment.setExitTransition(exitFade);

            FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
            transaction.remove(fragment);
            transaction.commitNow();


            waiting_reault = false;
            menu_showing = false;
            View mainview = (View) findViewById(R.id.yabause_view);
            mainview.requestFocus();
            YabauseRunnable.resume();
            audio.unmute(audio.SYSTEM);
          }
        };

        fragment.setonEndObserver(observer);

        transaction.setCustomAnimations(R.anim.slide_in_up, R.anim.slide_out_up,R.anim.slide_in_up, R.anim.slide_out_up);
        transaction.replace(R.id.ext_fragment, fragment, InGamePreference.TAG);
        //transaction.addToBackStack(InGamePreference.TAG);
        transaction.commit();

      }
      break;
    }
    DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
    drawer.closeDrawer(GravityCompat.START);
    return true;
  }

  @Override
  // after disc change event
  public void fileSelected(File file) {

    if (file != null) {
      gamepath = file.getAbsolutePath();
    }
    Button btn = (Button) findViewById(R.id.button_open_cd);
    YabauseRunnable.closeTray();
  }

  @Override
  public void onPause() {
    super.onPause();
    YabauseRunnable.pause();
    audio.mute(audio.SYSTEM);
    mInputManager.unregisterInputDeviceListener(this);
  }

  @Override
  public void onResume() {
    super.onResume();
    if (mTracker != null) {
      mTracker.setScreenName(TAG);
      mTracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    if (waiting_reault == false) {
      audio.unmute(audio.SYSTEM);
      YabauseRunnable.resume();
        }
    mInputManager.registerInputDeviceListener(this, null);
  }

  @Override
  public void onDestroy() {

    Log.v(TAG, "this is the end...");
    yabauseThread.destroy();
    super.onDestroy();
  }

  @Override
  public Dialog onCreateDialog(int id, Bundle args) {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setMessage(args.getString("message"))
        .setCancelable(false)
        .setNegativeButton(R.string.exit, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int id) {
            Yabause.this.finish();
          }
        })
        .setPositiveButton(R.string.ignore, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int id) {
            dialog.cancel();
          }
        });
    AlertDialog alert = builder.create();
    return alert;
  }

  void startReport() {
    waiting_reault = true;
    ReportDialog newFragment = new ReportDialog();
    newFragment.show(getFragmentManager(), "Report");

    // The device is smaller, so show the fragment fullscreen
    //android.app.FragmentTransaction transaction = getFragmentManager().beginTransaction();
    // For a little polish, specify a transition animation
    //transaction.setTransition(android.app.FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
    // To make it fullscreen, use the 'content' root view as the container
    // for the fragment, which is always the root view for the activity
    //transaction.add(R.id.ext_fragment, newFragment)
    //    .addToBackStack(null).commit();

    //android.app.FragmentTransaction transaction = getFragmentManager().beginTransaction();
    //transaction.replace(R.id.ext_fragment, newFragment, StateListFragment.TAG);
    //transaction.show(newFragment);
    //transaction.commit();


  }

  public void onFinishReport() {

  }

  @Override
  public void onInputDeviceAdded(int i) {
    updateInputDevice();
  }

  @Override
  public void onInputDeviceRemoved(int i) {
    updateInputDevice();
  }

  @Override
  public void onInputDeviceChanged(int i) {

  }

  @Override
  public void show() {
    if( YabauseRunnable.getRecordingStatus() == YabauseRunnable.RECORDING ) {
      YabauseRunnable.screenshot("");
    }else {
      this.toggleMenu();
    }
  }


  class ReportContents {
    public int _rating;
    public String _message;
    public boolean _screenshot;
    public String _screenshot_base64;
    public String _screenshot_save_path;
    public String _state_base64;
    public String _state_save_path;
  }

  public ReportContents current_report;
  public JSONObject current_game_info;

  static final int REPORT_STATE_INIT = 0;
  static final int REPORT_STATE_SUCCESS = 1;
  static final int REPORT_STATE_FAIL_DUPE = -1;
  static final int REPORT_STATE_FAIL_CONNECTION = -2;
  static final int REPORT_STATE_FAIL_AUTH = -3;
  public int _report_status = REPORT_STATE_INIT;

  String[] cheat_codes = null;

  public void updateCheatCode(String[] cheat_codes) {
    this.cheat_codes = cheat_codes;
    if (cheat_codes == null || cheat_codes.length == 0) {
      YabauseRunnable.updateCheat(null);
    } else {
      int index = 0;
      ArrayList<String> send_codes = new ArrayList<String>();
      for (int i = 0; i < cheat_codes.length; i++) {
        String[] tmp = cheat_codes[i].split("\n", -1);
        for (int j = 0; j < tmp.length; j++) {
          send_codes.add(tmp[j]);
        }
      }
      String[] cheat_codes_array = (String[]) send_codes.toArray(new String[0]);
      YabauseRunnable.updateCheat(cheat_codes_array);
    }

    if (waiting_reault) {
      waiting_reault = false;
      menu_showing = false;
      View mainview = (View) findViewById(R.id.yabause_view);
      mainview.requestFocus();
      YabauseRunnable.resume();
      audio.unmute(audio.SYSTEM);
    }
  }

  public void cancelStateLoad() {
    if (waiting_reault) {
      waiting_reault = false;
      menu_showing = false;
      View mainview = (View) findViewById(R.id.yabause_view);
      mainview.requestFocus();
      YabauseRunnable.resume();
      audio.unmute(audio.SYSTEM);
    }
  }

  public void loadState(String filename) {

    YabauseRunnable.loadstate(filename);

    Fragment fg = getSupportFragmentManager().findFragmentByTag(StateListFragment.TAG);
    if (fg != null) {
      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      transaction.remove(fg);
      transaction.commit();
    }

    if (waiting_reault) {
      waiting_reault = false;
      menu_showing = false;
      View mainview = (View) findViewById(R.id.yabause_view);
      mainview.requestFocus();
      YabauseRunnable.resume();
      audio.unmute(audio.SYSTEM);
    }
  }

  private void createZip(ZipOutputStream zos, File[] files) throws IOException {
    byte[] buf = new byte[1024];
    InputStream is = null;
    try {
      for (File file : files) {
        ZipEntry entry = new ZipEntry(file.getName());
        zos.putNextEntry(entry);
        is = new BufferedInputStream(new FileInputStream(file));
        int len = 0;
        while ((len = is.read(buf)) != -1) {
          zos.write(buf, 0, len);
        }
      }
    } finally {
      IOUtils.closeQuietly(is);
    }
  }

  private String current_report_filename_;

  public String getCurrentReportfilename() {
    return current_report_filename_;
  }

  private String current_screenshot_filename_;

  public String getCurrentScreenshotfilename() {
    return current_screenshot_filename_;
  }

  void doReportCurrentGame(int rating, String message, boolean screenshot) {

    current_report = new ReportContents();
    current_report._rating = rating;
    current_report._message = message;
    current_report._screenshot = screenshot;
    _report_status = REPORT_STATE_INIT;

    String gameinfo = YabauseRunnable.getGameinfo();
    if (gameinfo == null) {
      return;
    }

    showDialog();

    DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
    Date date = new Date();

    String zippath = YabauseStorage.getStorage().getScreenshotPath()
        + YabauseRunnable.getCurrentGameCode() +
        dateFormat.format(date) + ".zip";

    String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
        + "screenshot.png";

    if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
      dismissDialog();
      return;
    }

    String save_path = YabauseStorage.getStorage().getStateSavePath();
    String current_gamecode = YabauseRunnable.getCurrentGameCode();
    File save_root = new File(YabauseStorage.getStorage().getStateSavePath(), current_gamecode);
    if (!save_root.exists()) save_root.mkdir();
    String save_filename = YabauseRunnable.savestate(save_path + current_gamecode);

    File files[] = new File[1];
    files[0] = new File(save_filename);

    ZipOutputStream zos = null;
    try {
      zos = new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(new File(zippath))));
      createZip(zos, files);
    } catch (IOException e) {
      Log.d(TAG, e.getLocalizedMessage());
      dismissDialog();
      return;
    } finally {
      IOUtils.closeQuietly(zos);
    }
    files[0].delete();
    current_screenshot_filename_ = screen_shot_save_path;
    current_report_filename_ = zippath;

    try {
      current_game_info = new JSONObject(gameinfo);
    } catch (Exception e) {
      Log.e(TAG, e.getLocalizedMessage());
      dismissDialog();
      return;
    }
    AsyncReportv2 asyncTask = new AsyncReportv2(this);
    String url = "https://www.uoyabause.org/api/";
    //url = "http://www.uoyabause.org:3000/api/";
    asyncTask.execute(url, YabauseRunnable.getCurrentGameCode());

  }

  void doReportCurrentGame_old(int rating, String message, boolean screenshot) {
    current_report = new ReportContents();
    current_report._rating = rating;
    current_report._message = message;
    current_report._screenshot = screenshot;
    _report_status = REPORT_STATE_INIT;

    String gameinfo = YabauseRunnable.getGameinfo();
    if (gameinfo != null) {

      try {

        showDialog();
        AsyncReport asyncTask = new AsyncReport(this);
        current_game_info = new JSONObject(gameinfo);

        DateFormat dateFormat = new SimpleDateFormat("_yyyy_MM_dd_HH_mm_ss");
        Date date = new Date();

        String screen_shot_save_path = YabauseStorage.getStorage().getScreenshotPath()
            + YabauseRunnable.getCurrentGameCode() +
            dateFormat.format(date) + ".png";

        if (YabauseRunnable.screenshot(screen_shot_save_path) != 0) {
          dismissDialog();
          return;
        }

        InputStream inputStream = new FileInputStream(screen_shot_save_path);//You can get an inputStream using any IO API
        byte[] bytes;
        byte[] buffer = new byte[8192];
        int bytesRead;
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        try {
          while ((bytesRead = inputStream.read(buffer)) != -1) {
            output.write(buffer, 0, bytesRead);
          }
        } catch (IOException e) {
          e.printStackTrace();
          dismissDialog();
          return;
        }
        bytes = output.toByteArray();
        String encodedString = Base64.encodeToString(bytes, Base64.DEFAULT);


        JSONObject jsonObjimg = new JSONObject();
        jsonObjimg.put("data", encodedString);
        jsonObjimg.put("filename", screen_shot_save_path);
        jsonObjimg.put("content_type", "image/png");
        JSONObject jsonObjgame = current_game_info.getJSONObject("game");
        jsonObjgame.put("title_image", jsonObjimg);

        if (screenshot) {
          current_report._screenshot_base64 = encodedString;
          current_report._screenshot_save_path = screen_shot_save_path;
        }

        //asyncTask.execute("http://192.168.0.7:3000/api/", YabauseRunnable.getCurrentGameCode());
        asyncTask.execute("https://www.uoyabause.org/api/", YabauseRunnable.getCurrentGameCode());

        return;

      } catch (Exception e) {
        e.printStackTrace();
      }
    }
  }

  void cancelReportCurrentGame() {
    waiting_reault = false;
    YabauseRunnable.resume();
    audio.unmute(audio.SYSTEM);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    super.onActivityResult(requestCode, resultCode, data);
    switch (requestCode) {
      case 0x01:
        waiting_reault = false;
        toggleMenu();
        break;
      case RC_SIGN_IN:
        if (requestCode == RC_SIGN_IN) {
          IdpResponse response = IdpResponse.fromResultIntent(data);
          if (resultCode == RESULT_OK) {
            FirebaseUser user = FirebaseAuth.getInstance().getCurrentUser();
            if( loginEmitter != null ){
              loginEmitter.onNext(user);
              loginEmitter.onComplete();
            }
          } else {
            if( loginEmitter != null ){
              loginEmitter.onError(new Exception(response.getError().getLocalizedMessage()));
            }
          }
        }
        break;
    }
  }

  @Override
  public boolean onGenericMotionEvent(MotionEvent event) {

    if (menu_showing) {
      return super.onGenericMotionEvent(event);
    }

    int rtn = padm.onGenericMotionEvent(event);
    if (rtn != 0) {
      return true;
    }
    return super.onGenericMotionEvent(event);
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {

    int action = event.getAction();
    int keyCode = event.getKeyCode();

    if (keyCode == KeyEvent.KEYCODE_BACK) {

      if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {

        InGamePreference fg_ingame = (InGamePreference) getSupportFragmentManager().findFragmentByTag(InGamePreference.TAG);
        if (fg_ingame != null) {
          fg_ingame.onBackPressed();
          return true;
        }

        Fragment fg = getSupportFragmentManager().findFragmentByTag(StateListFragment.TAG);
        if (fg != null) {
          this.cancelStateLoad();
          FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
          transaction.remove(fg);
          transaction.commit();
          View mainv = findViewById(R.id.yabause_view);
          mainv.setActivated(true);
          mainv.requestFocus();
          waiting_reault = false;
          menu_showing = false;
          YabauseRunnable.resume();
          audio.unmute(audio.SYSTEM);
          return true;
        }

        fg = getSupportFragmentManager().findFragmentByTag(TabBackupFragment.TAG);
        if (fg != null) {
          FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
          transaction.remove(fg);
          transaction.commit();
          View mainv = findViewById(R.id.yabause_view);
          mainv.setActivated(true);
          mainv.requestFocus();
          waiting_reault = false;
          menu_showing = false;
          YabauseRunnable.resume();
          audio.unmute(audio.SYSTEM);
          return true;
        }

        PadTestFragment fg2 = (PadTestFragment) getSupportFragmentManager().findFragmentByTag(PadTestFragment.TAG);
        if (fg2 != null) {
          fg2.onBackPressed();
          return true;
        }
        toggleMenu();
      }
      return true;
    }

    if (menu_showing) {
      return super.dispatchKeyEvent(event);
    }

    if (this.waiting_reault) {
      return super.dispatchKeyEvent(event);
    }

    //Log.d("dispatchKeyEvent","device:" + event.getDeviceId() + ",action:" + action +",keyCoe:" + keyCode );
    if (action == KeyEvent.ACTION_UP) {
      int rtn = padm.onKeyUp(keyCode, event);
      if (rtn != 0) {
        return true;
      }
    } else if (action == KeyEvent.ACTION_MULTIPLE) {

    } else if (action == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
      int rtn = padm.onKeyDown(keyCode, event);
      if (rtn != 0) {
        return true;
      }
    }
    return super.dispatchKeyEvent(event);
  }

  @Override
  public void onTrimMemory(int level) {
    super.onTrimMemory(level);
    switch (level) {
      case TRIM_MEMORY_RUNNING_MODERATE:
        break;
      case TRIM_MEMORY_RUNNING_LOW:
        break;
      case TRIM_MEMORY_RUNNING_CRITICAL:
        break;
      case TRIM_MEMORY_UI_HIDDEN:
        break;
      case TRIM_MEMORY_BACKGROUND:
        break;
      case TRIM_MEMORY_COMPLETE:
        break;
    }

  }

  private boolean menu_showing = false;

  private void toggleMenu() {
    if (menu_showing == true) {
      menu_showing = false;
      View mainview = (View) findViewById(R.id.yabause_view);
      mainview.requestFocus();
      YabauseRunnable.resume();
      audio.unmute(audio.SYSTEM);
      this.mDrawerLayout.closeDrawer(GravityCompat.START);
    } else {
      menu_showing = true;
      YabauseRunnable.pause();
      audio.mute(audio.SYSTEM);
      String name = YabauseRunnable.getGameTitle();
      TextView tx = (TextView) findViewById(R.id.menu_title);
      if (tx != null) {
        tx.setText(name);
      }
      if (adView != null) {
        LinearLayout lp = (LinearLayout) findViewById(R.id.navilayer);
        if (lp != null) {
          final int mCount = lp.getChildCount();
          boolean find = false;
          for (int i = 0; i < mCount; ++i) {
            final View mChild = lp.getChildAt(i);
            if (mChild == adView) {
              find = true;
            }
          }
          if (find == false) {
            lp.addView(adView);
          }
          AdRequest adRequest = new AdRequest.Builder().addTestDevice("303A789B146C169D4BDB5652D928FF8E").build();
          adView.loadAd(adRequest);
        }
      }
      this.mDrawerLayout.openDrawer(GravityCompat.START);
    }
  }

  private String errmsg;

  public void errorMsg(String msg) {
    errmsg = msg;
    Log.d(TAG, "errorMsg " + msg);
    runOnUiThread(new Runnable() {
      public void run() {
        final DrawerLayout layout = (DrawerLayout) findViewById(R.id.drawer_layout);
        Snackbar.make(layout, Yabause.this.errmsg, Snackbar.LENGTH_SHORT).show();
      }
    });

  }

  private void readPreferences(String gamecode) {

    org.uoyabause.android.GameSharedPreference.setupInGamePreferences(this,gamecode);

    //------------------------------------------------------------------------------------------------
    // Load per game setting
    SharedPreferences gamePreference = getSharedPreferences(gameCode,Context.MODE_PRIVATE);
    YabauseRunnable.enableComputeShader(gamePreference.getBoolean("pref_use_compute_shader", false) ? 1 : 0);
    YabauseRunnable.enableRotateScreen(gamePreference.getBoolean("pref_rotate_screen", false) ? 1 : 0);
    boolean fps = gamePreference.getBoolean("pref_fps", false);
    YabauseRunnable.enableFPS(fps ? 1 : 0);
    Log.d(TAG, "enable FPS " + fps);

    String sPg = gamePreference.getString("pref_polygon_generation", "0");
    Integer iPg = new Integer(sPg);
    YabauseRunnable.setPolygonGenerationMode(iPg);
    Log.d(TAG, "setPolygonGenerationMode " + iPg.toString());

    boolean frameskip = gamePreference.getBoolean("pref_frameskip", true);
    YabauseRunnable.enableFrameskip(frameskip ? 1 : 0);
    Log.d(TAG, "enable enableFrameskip " + frameskip);

    Integer sKa = new Integer(gamePreference.getString("pref_polygon_generation", "0"));
    YabauseRunnable.setPolygonGenerationMode(sKa);

    // ToDo: list
    //boolean keep_aspectrate = gamePreference.getBoolean("pref_keepaspectrate", true);
    //if(keep_aspectrate) {
    //  YabauseRunnable.setKeepAspect(1);
    //}else {
    //  YabauseRunnable.setKeepAspect(0);
    //}

    Integer resolution_setting = new Integer(gamePreference.getString("pref_resolution", "0"));
    YabauseRunnable.setResolutionMode(resolution_setting);

    Integer rbg_resolution_setting = new Integer(gamePreference.getString("pref_rbg_resolution", "0"));
    YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting);


    //-------------------------------------------------------------------------------------
    // Load common setting
    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);

    boolean extmemory = sharedPref.getBoolean("pref_extend_internal_memory", true);
    YabauseRunnable.enableExtendedMemory(extmemory ? 1 : 0);
    Log.d(TAG, "enable Extended Memory " + extmemory);

    String cpu = sharedPref.getString("pref_cpu", "3");
    Integer icpu = new Integer(cpu);

    String abi = System.getProperty("os.arch");
    if (abi.contains("64")) {
      if (icpu.equals(2)) {
        icpu = 3;
      }
    }
    YabauseRunnable.setCpu(icpu.intValue());
    Log.d(TAG, "cpu " + icpu.toString());

    String sfilter = sharedPref.getString("pref_filter", "0");
    Integer ifilter = new Integer(sfilter);
    YabauseRunnable.setFilter(ifilter);
    Log.d(TAG, "setFilter " + ifilter.toString());

    YabauseRunnable.setAspectRateMode(0);

    boolean audioout = sharedPref.getBoolean("pref_audio", true);
    if (audioout) {
      audio.unmute(audio.USER);
    } else {
      audio.mute(audio.USER);
    }
    Log.d(TAG, "Audio " + audioout);

    String bios = sharedPref.getString("pref_bios", "");
    if (bios.length() > 0) {
      YabauseStorage storage = YabauseStorage.getStorage();
      biospath = storage.getBiosPath(bios);
    } else
      biospath = "";

    Log.d(TAG, "bios " + bios);

    String cart = sharedPref.getString("pref_cart", "");
    if (cart.length() > 0) {
      Integer i = new Integer(cart);
      carttype = i.intValue();
    } else
      carttype = -1;

    Log.d(TAG, "cart " + cart);

    final ActivityManager activityManager = (ActivityManager) getSystemService(this.ACTIVITY_SERVICE);
    final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
    final boolean supportsEs3 = configurationInfo.reqGlEsVersion >= 0x30000;

    String video;

    if (supportsEs3) {
      video = sharedPref.getString("pref_video", "1");
    } else {
      video = sharedPref.getString("pref_video", "2");
    }
    if (video.length() > 0) {
      Integer i = new Integer(video);
      video_interface = i.intValue();
    } else {
      video_interface = -1;
    }

    Log.d(TAG, "video " + video);
    Log.d(TAG, "getGamePath " + getGamePath());
    Log.d(TAG, "getMemoryPath " + getMemoryPath());
    Log.d(TAG, "getCartridgePath " + getCartridgePath());

    String ssound = sharedPref.getString("pref_sound_engine", "1");
    Integer isound = new Integer(ssound);
    YabauseRunnable.setSoundEngine(isound);
    Log.d(TAG, "setSoundEngine " + isound.toString());

    Integer scsp_sync = new Integer(sharedPref.getString("pref_scsp_sync_per_frame", "1"));
    YabauseRunnable.setScspSyncPerFrame(scsp_sync);

    Integer cpu_sync = new Integer(sharedPref.getString("pref_cpu_sync_per_line", "1"));
    YabauseRunnable.setCpuSyncPerLine(cpu_sync);

    Integer scsp_time_sync = new Integer(sharedPref.getString("scsp_time_sync_mode", "1"));
    YabauseRunnable.setScspSyncTimeMode(scsp_time_sync);

    updateInputDevice();

  }

  void updateInputDevice(){
    SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
    NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
    Menu menu = navigationView.getMenu();
    MenuItem nav_pad_device = menu.findItem(R.id.menu_item_pad_device);
    YabausePad pad = (YabausePad) findViewById(R.id.yabause_pad);

    Integer rbg_resolution_setting = new Integer(sharedPref.getString("pref_rbg_resolution", "0"));
    YabauseRunnable.setRbgResolutionMode(rbg_resolution_setting);


    // InputDevice
    String selInputdevice = sharedPref.getString("pref_player1_inputdevice", "65535");
    padm = PadManager.updatePadManager();
    padm.setShowMenulistener(this);
    Log.d(TAG, "input " + selInputdevice);
    // First time
    if (selInputdevice.equals("65535")) {
      // if game pad is connected use it.
      if (padm.getDeviceCount() > 0) {
        padm.setPlayer1InputDevice(null);
        Editor editor = sharedPref.edit();
        editor.putString("pref_player1_inputdevice", padm.getId(0));
        editor.commit();
        selInputdevice = padm.getId(0);
        // if no game pad is detected use on-screen game pad.
      } else {
        Editor editor = sharedPref.edit();
        editor.putString("pref_player1_inputdevice", "-1");
        editor.commit();
        selInputdevice = "-1";
      }
    }
    if (padm.getDeviceCount() > 0 && !selInputdevice.equals("-1")) {
      pad.show(false);
      Log.d(TAG, "ScreenPad Disable");
      padm.setPlayer1InputDevice(selInputdevice);

      for (int inputType = 0; inputType < padm.getDeviceCount(); inputType++) {
        if (padm.getId(inputType).equals(selInputdevice)) {
          nav_pad_device.setTitle(padm.getName(inputType));
        }
      }
      // Enable Swipe
      mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);

    } else {
      pad.show(true);
      Log.d(TAG, "ScreenPad Enable");
      padm.setPlayer1InputDevice(null);

      // Set Menu item
      nav_pad_device.setTitle(getString(R.string.onscreen_pad));

      // Disable Swipe
      mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
    }

    String selInputdevice2 = sharedPref.getString("pref_player2_inputdevice", "65535");
    MenuItem nav_pad_device_p2 = menu.findItem(R.id.menu_item_pad_device_p2);

    padm.setPlayer2InputDevice(null);
    nav_pad_device_p2.setTitle("Disconnected");
    menu.findItem(R.id.pad_mode_p2).setVisible(false);
    menu.findItem(R.id.menu_item_pad_setting_p2).setVisible(false);

    if (!selInputdevice.equals("65535") && !selInputdevice.equals("-1")) {
      for (int inputType = 0; inputType < padm.getDeviceCount(); inputType++) {
        if (padm.getId(inputType).equals(selInputdevice2)) {
          padm.setPlayer2InputDevice(selInputdevice2);
          nav_pad_device_p2.setTitle(padm.getName(inputType));
          menu.findItem(R.id.pad_mode_p2).setVisible(true);
          menu.findItem(R.id.menu_item_pad_setting_p2).setVisible(true);
        }
      }
    }

    boolean analog = sharedPref.getBoolean("pref_analog_pad", false);
    YabausePad padv = (YabausePad) findViewById(R.id.yabause_pad);
    if (analog) {
      Yabause.this.padm.setAnalogMode(PadManager.MODE_ANALOG);
      YabauseRunnable.switch_padmode(PadManager.MODE_ANALOG);
      padv.setPadMode(PadManager.MODE_ANALOG);
    } else {
      Yabause.this.padm.setAnalogMode(PadManager.MODE_HAT);
      YabauseRunnable.switch_padmode(PadManager.MODE_HAT);
      padv.setPadMode(PadManager.MODE_HAT);
    }
    menu.findItem(R.id.pad_mode).setChecked(analog);

    analog = sharedPref.getBoolean("pref_analog_pad2", false);
    if (analog) {
      Yabause.this.padm.setAnalogMode2(PadManager.MODE_ANALOG);
      YabauseRunnable.switch_padmode2(PadManager.MODE_ANALOG);
    } else {
      Yabause.this.padm.setAnalogMode2(PadManager.MODE_HAT);
      YabauseRunnable.switch_padmode2(PadManager.MODE_HAT);
    }
    menu.findItem(R.id.pad_mode_p2).setChecked(analog);
        Integer scsp_time_sync =  new Integer(sharedPref.getString("scsp_time_sync_mode","1"));
        YabauseRunnable.setScspSyncTimeMode(scsp_time_sync);

  }

  public String getBiosPath() {
    return biospath;
  }

  public String getGamePath() {
    return gamepath;
  }

  public String getTestPath() {
    if( testCase == null ){
      return null;
    }
    return YabauseStorage.getStorage().getRecordPath() + this.testCase;
  }

  public String getMemoryPath() {
    return YabauseStorage.getStorage().getMemoryPath("memory.ram");
  }

  public int getCartridgeType() {
    return carttype;
  }

  public int getVideoInterface() {
    return video_interface;
  }

  public int getPlayer2InputDevice() {
    return padm.getPlayer2InputDevice();
  }

  public String getCartridgePath() {
    return YabauseStorage.getStorage().getCartridgePath(Cartridge.getDefaultFilename(carttype));
  }

  static {
    System.loadLibrary("yabause_native");
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus && getSupportFragmentManager().findFragmentById(R.id.ext_fragment) == null) {
      updateViewLayout(getResources().getConfiguration().orientation);
    }
 }

  @Override
  public void onDeviceUpdated(int target) {

  }

  @Override
  public void onSelected(int target, String name, String id) {
    YabausePad pad = (YabausePad) findViewById(R.id.yabause_pad);

    NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
    Menu menu = navigationView.getMenu();
    MenuItem nav_pad_device = menu.findItem(R.id.menu_item_pad_device);
    MenuItem nav_pad_device_p2 = menu.findItem(R.id.menu_item_pad_device_p2);


    padm = PadManager.updatePadManager();
    padm.setShowMenulistener(this);
    if (padm.getDeviceCount() > 0 && !id.equals("-1")) {
      switch (target) {
        case PLAYER1:
          Log.d(TAG, "ScreenPad Disable");
          pad.show(false);
          padm.setPlayer1InputDevice(id);
          nav_pad_device.setTitle(name);
          break;
        case PLAYER2:
          padm.setPlayer2InputDevice(id);
          nav_pad_device_p2.setTitle(name);
          break;
      }
      mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);
    } else {
      if (target == PLAYER1) {
        pad.updateScale();
        pad.show(true);
        nav_pad_device.setTitle(getString(R.string.onscreen_pad));
        Log.d(TAG, "ScreenPad Enable");
        padm.setPlayer1InputDevice(null);
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
      }else if( target == PLAYER2  ){
        padm.setPlayer2InputDevice(null);
        nav_pad_device_p2.setTitle("Disconnected");
      }
    }
    updateInputDevice();
    waiting_reault = false;
    toggleMenu();
  }

  @Override
  public void onCancel(int target) {
    waiting_reault = false;
    toggleMenu();
  }

  @Override
  public void onBackPressed() {

    PadTestFragment fg = (PadTestFragment) getSupportFragmentManager().findFragmentByTag(PadTestFragment.TAG);
    if (fg != null) {
      fg.onBackPressed();
    }

    InGamePreference fg2 = (InGamePreference) getSupportFragmentManager().findFragmentByTag(InGamePreference.TAG);
    if (fg2 != null) {
      fg2.onBackPressed();
    }


  }

  @Override
  public void onFinish() {
    PadTestFragment fg = (PadTestFragment) getSupportFragmentManager().findFragmentByTag(PadTestFragment.TAG);
    if (fg != null) {
      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      transaction.remove(fg);
      transaction.commit();
      YabausePad padv = (YabausePad) findViewById(R.id.yabause_pad);
      padv.updateScale();
    }
    waiting_reault = false;
    toggleMenu();
  }

  @Override
  public void onCancel() {
    PadTestFragment fg = (PadTestFragment) getSupportFragmentManager().findFragmentByTag(PadTestFragment.TAG);
    if (fg != null) {
      FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
      transaction.remove(fg);
      transaction.commit();
    }
    waiting_reault = false;
    toggleMenu();
  }

  @Override
  public void onFinishInputSetting() {
    updateInputDevice();
    waiting_reault = false;
    toggleMenu();
  }

}
