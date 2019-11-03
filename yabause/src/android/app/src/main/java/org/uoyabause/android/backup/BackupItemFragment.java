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

package org.uoyabause.android.backup;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import androidx.annotation.NonNull;
import com.google.android.material.snackbar.Snackbar;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.util.Base64;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.PopupMenu;
import android.widget.TextView;

import com.google.android.gms.tasks.Continuation;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.Exclude;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.IgnoreExtraProperties;
import com.google.firebase.database.ValueEventListener;
import com.google.firebase.storage.FirebaseStorage;
import com.google.firebase.storage.OnPausedListener;
import com.google.firebase.storage.OnProgressListener;
import com.google.firebase.storage.StorageMetadata;
import com.google.firebase.storage.StorageReference;
import com.google.firebase.storage.StorageTask;
import com.google.firebase.storage.UploadTask;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.uoyabause.android.AuthFragment;
import org.uoyabause.uranus.R;
import org.uoyabause.android.YabauseRunnable;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class BackupDevice {
    static public int DEVICE_CLOUD = 48;
    public int id_;
    public String name_;
}

@IgnoreExtraProperties
class BackupItem {
    public int index_;
    public String filename;
    public String comment;
    public int language;
    public String savedate;
    public int datasize;
    public int blocksize;
    public String url = "";
    public String key = "";
    //public Map<String, Boolean> stars = new HashMap<>();

    public BackupItem(){
    }
    public BackupItem(
            int index,
            String filename,
            String comment,
            int language,
            String savedate,
            int datasize,
            int blocksize,
            String url

    ){
        index_ = index;
        this.filename = filename;
        this.comment =  comment;
        this.language = language;
        this.savedate = savedate;
        this.datasize = datasize;
        this.blocksize = blocksize;
        this.url = url;
    }

    public int getindex() { return index_; }
    public String getFilename() { return filename; }
    public String getComment() { return comment; }
    public int getLanguage() { return language; }
    public String getSavedate() { return savedate; }
    public int getDatasize() { return datasize; }
    public int getBlocksize() { return blocksize; }
    public String getUrl() { return url; }
    public String getKey() { return key; }

    final String DATE_PATTERN ="yyyy/MM/dd HH:mm";

    @Exclude
    public Map<String, Object> toMap() {
        HashMap<String, Object> result = new HashMap<>();
        result.put("filename", filename);
        result.put("comment", comment);
        result.put("language", language);
        result.put("savedate", savedate);
        result.put("datasize", datasize);
        result.put("blocksize", blocksize);
        result.put("url", url);
        result.put("key", key);

        return result;
    }
}


/**
 * A fragment representing a list of Items.
 * <p/>
 * Activities containing this fragment MUST implement the {@link OnListFragmentInteractionListener}
 * interface.
 */
public class BackupItemFragment extends AuthFragment implements BackupItemRecyclerViewAdapter.OnItemClickListener  {

    static final String TAG = "BackupItemFragment";

    private List<BackupDevice> backup_devices_;
    private OnListFragmentInteractionListener mListener;
    private ArrayList<BackupItem> _items;
    int currentpage_ = 0;
    View root_view_;
    RecyclerView view_;
    TextView sum_;
    BackupItemRecyclerViewAdapter adapter_;
    DatabaseReference database_;
    private int totalsize_ = 0;
    private int freesize_ = 0;
    private static final int RC_SIGN_IN = 123;


    public BackupItemFragment() {
    }

    public static BackupItemFragment getInstance(int position) {
        BackupItemFragment f = new BackupItemFragment();
        Bundle args = new Bundle();
        args.putInt("position", position);
        f.setArguments(args);
        return f;
    }

    public static BackupItemFragment newInstance(int columnCount) {
        BackupItemFragment fragment = new BackupItemFragment();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            updateSaveList(currentpage_);
        }else {
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            currentpage_ = getArguments().getInt("position");
        }
        String jsonstr;
        jsonstr = YabauseRunnable.getDevicelist();
        backup_devices_ = new ArrayList<BackupDevice>();
        try {

            JSONObject json = new JSONObject(jsonstr);
            JSONArray array = json.getJSONArray("devices");
            for (int i = 0; i < array.length(); i++) {
                JSONObject data = array.getJSONObject(i);
                BackupDevice tmp = new BackupDevice();
                tmp.name_ = data.getString("name");
                tmp.id_= data.getInt("id");
                backup_devices_.add(tmp);
            }

            BackupDevice tmp = new BackupDevice();
            tmp.name_ = "cloud";
            tmp.id_= BackupDevice.DEVICE_CLOUD;
            backup_devices_.add(tmp);

        }catch(JSONException e){
            Log.e(TAG, "Fail to convert to json", e);
        }

        if( backup_devices_.size() == 0 ){
            Log.e(TAG, "Can't find device");
        }

    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_backupitem_list, container, false);
        view_ = (RecyclerView) view.findViewById(R.id.list);
        sum_ = (TextView) view.findViewById(R.id.tvSum);
         Context context = view.getContext();
        view_.setLayoutManager(new LinearLayoutManager(context));
        root_view_ = view;
        updateSaveList(currentpage_);
        return view;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    @Override
    protected void OnAuthAccepted(){
        updateSaveListCloud();
    }

    void updateSaveListCloud(){
        FirebaseAuth auth = checkAuth();
        if( auth == null ){
            return;
        }
        if( auth.getCurrentUser() == null ) {
            return;
        }
        _items = new ArrayList<BackupItem>();
        DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
        String baseurl = "/user-posts/" + auth.getCurrentUser().getUid()  + "/backup/";
        database_ =  baseref.child(baseurl);
        if( database_ == null ){
            return;
        }

        ValueEventListener DataListener = new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {

                if( dataSnapshot.hasChildren() ) {
                    _items.clear();
                    int index = 0;
                    for (DataSnapshot child : dataSnapshot.getChildren()) {
                        BackupItem newitem = child.getValue(BackupItem.class);
                        newitem.index_ = index;
                        index++;
                        _items.add(newitem);
                    }

                    if( view_ != null ) {
                        adapter_ = new BackupItemRecyclerViewAdapter( BackupItemFragment.this.currentpage_,_items, BackupItemFragment.this);
                        view_.setAdapter(adapter_);
                    }

                    if( sum_ != null ){
                        sum_.setText( "" );
                    }

                }else{
                    Log.e("CheatEditDialog", "Bad Data " + dataSnapshot.getKey() );
                }
            }
            @Override
            public void onCancelled(DatabaseError databaseError) {
            }
        };
        //database_.addListenerForSingleValueEvent(DataListener);
        database_.addValueEventListener(DataListener);
    }

    void updateSaveList( int device ){

        if( backup_devices_ == null ){
            return;
        }

        if( backup_devices_.get(device).id_ == BackupDevice.DEVICE_CLOUD ){
            updateSaveListCloud();
            return;
        }

        String jsonstr = YabauseRunnable.getFilelist(device);
        _items = new ArrayList<BackupItem>();
        try {
            JSONObject json = new JSONObject(jsonstr);
            totalsize_ = json.getJSONObject("status").getInt("totalsize");
            freesize_ = json.getJSONObject("status").getInt("freesize");
            JSONArray array = json.getJSONArray("saves");
            for (int i = 0; i < array.length(); i++) {
                JSONObject data = array.getJSONObject(i);
                BackupItem tmp = new BackupItem();
                tmp.index_ = data.getInt("index");

                byte [] bfilename = Base64.decode( data.getString("filename"), 0 );
                try {
                    tmp.filename = new String(bfilename, "MS932");
                }catch(Exception e){
                    tmp.filename = data.getString("filename");
                }
                bfilename = Base64.decode( data.getString("comment"), 0 );
                try {
                    tmp.comment = new String(bfilename, "MS932");
                }catch(Exception e){
                    tmp.comment = data.getString("comment");
                }
                tmp.datasize = data.getInt("datasize");
                tmp.blocksize = data.getInt("blocksize");

                String datestr = "";
                datestr += String.format("%04d",data.getInt("year")+1980);
                datestr += String.format("%02d",data.getInt("month"));
                datestr += String.format("%02d",data.getInt("day"));
                datestr += " ";
                datestr += String.format("%02d",data.getInt("hour")) + ":";
                datestr += String.format("%02d",data.getInt("minute")) + ":00";
                tmp.savedate = datestr;
                _items.add(tmp);
            }

            if( view_ != null ) {
                adapter_ = new BackupItemRecyclerViewAdapter(currentpage_,_items,this);
                view_.setAdapter(adapter_);
            }

            if( sum_ != null ){
                sum_.setText( String.format("%,d Byte free of %,d Byte",freesize_,totalsize_) );
            }

        }catch( JSONException e ){
            Log.e(TAG, "Fail to convert to json", e);
        }
    }

    public static boolean isImmersiveAvailable() {
        return android.os.Build.VERSION.SDK_INT >= 19;
    }

    public void setFullscreen(Activity activity) {
        if (Build.VERSION.SDK_INT > 10) {
            int flags = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_FULLSCREEN;

            if (isImmersiveAvailable()) {
                flags |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            }

            activity.getWindow().getDecorView().setSystemUiVisibility(flags);
        } else {
            activity.getWindow()
                    .setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
    }

    public void exitFullscreen(Activity activity) {
        if (Build.VERSION.SDK_INT > 10) {
            activity.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
        } else {
            activity.getWindow()
                    .setFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN,
                            WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
        }
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p/>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnListFragmentInteractionListener {
        // TODO: Update argument type and name
        void onItemClickx(int currentpage, int position, BackupItem backupitem, View v );
    }

    @Override
    public void onItemClick(int currentpage, int position, BackupItem backupitem, View v ) {
        PopupMenu popup = new PopupMenu(getActivity(), v);
        MenuInflater inflate = popup.getMenuInflater();
        inflate.inflate(R.menu.backup, popup.getMenu());
        final BackupItem backupitemi = backupitem;
        final int position_ = position;

        Menu popupMenu = popup.getMenu();

        popupMenu.findItem(R.id.copy_to_internal).setVisible(false);
        popupMenu.findItem(R.id.copy_to_cloud).setVisible(false);
        popupMenu.findItem(R.id.copy_to_external).setVisible(false);
        for( int i=0; i< backup_devices_.size(); i++ ){

            if( currentpage != i ) {
                if( backup_devices_.get(i).id_ == 0 ){
                    popupMenu.findItem(R.id.copy_to_internal).setVisible(true);
                }
                else if( backup_devices_.get(i).id_ == BackupDevice.DEVICE_CLOUD ){
                    popupMenu.findItem(R.id.copy_to_cloud).setVisible(true);
                }
                else{
                    popupMenu.findItem(R.id.copy_to_external).setVisible(true);
                }

            }

        }

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {

            @Override
            public boolean onMenuItemClick(MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.copy_to_internal:
                        if( backup_devices_.get(currentpage_).id_ == BackupDevice.DEVICE_CLOUD ){
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(0);
                            downalodfromCloud(bk.id_, backupitemi);
                            break;
                        }
                        if( BackupItemFragment.this.backup_devices_.size() >= 1 ) {
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(0);
                            YabauseRunnable.copy(bk.id_, backupitemi.index_ );
                        }

                        break;
                    case R.id.copy_to_external:
                        if( backup_devices_.get(currentpage_).id_ == BackupDevice.DEVICE_CLOUD ){
                            if( BackupItemFragment.this.backup_devices_.size() < 3 ){
                                Snackbar
                                        .make(root_view_, "Internal device is not found.", Snackbar.LENGTH_SHORT)
                                        .show();
                                break;
                            }
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(1);
                            downalodfromCloud(bk.id_, backupitemi);
                            break;
                        }
                        if( BackupItemFragment.this.backup_devices_.size() >= 2 ) {
                            BackupDevice bk = BackupItemFragment.this.backup_devices_.get(1);
                            YabauseRunnable.copy(bk.id_, backupitemi.index_ );
                        }

                        break;
                    case R.id.copy_to_cloud:

                        uploadToCloud(backupitemi);
/*
                        new AlertDialog.Builder(getActivity())
                                .setTitle("Error")
                                .setMessage(R.string.this_func_not_yet)
                                .setPositiveButton("OK", null)
                                .show();
*/
                        break;

                    case R.id.delete:

                        new AlertDialog.Builder(getActivity())
                                .setTitle(R.string.deleting_file)
                                .setMessage(R.string.sure_delete)
                                .setNegativeButton(R.string.no,null)
                                .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {

                                        if( backup_devices_.get(currentpage_).id_ == BackupDevice.DEVICE_CLOUD ){
                                            deleteCloudItem(backupitemi);
                                        }else {
                                            YabauseRunnable.deletefile(backupitemi.index_);
                                        }
                                        BackupItemFragment.this.view_.removeViewAt(position_);
                                        BackupItemFragment.this._items.remove(position_);
                                        BackupItemFragment.this.adapter_.notifyItemRemoved(position_);
                                        BackupItemFragment.this.adapter_.notifyItemRangeChanged(position_, BackupItemFragment.this._items.size());
                                    }
                                })
                                .show();
                        break;
                    default:
                        return false;
                }
                return false;
            }
        });
        popup.show();
    }

    void downalodfromCloud( final int deviceid, final BackupItem backupitemi ){

        if( backupitemi.getUrl().equals("") ){
            return;
        }

        FirebaseStorage storage = FirebaseStorage.getInstance();
        StorageReference httpsReference = storage.getReferenceFromUrl(backupitemi.getUrl());
        final long ONE_MEGABYTE = 1024 * 1024;
        httpsReference.getBytes(ONE_MEGABYTE).addOnSuccessListener(new OnSuccessListener<byte[]>() {
            @Override
            public void onSuccess(byte[] bytes) {

                String jsonstr = YabauseRunnable.getFilelist(deviceid);
                int freesize = 0;
                try {
                    JSONObject json = new JSONObject(jsonstr);
                    freesize = json.getJSONObject("status").getInt("freesize");
                }catch( Exception e ){
                    return;
                }

                if( backupitemi.getDatasize() >= freesize ){
                    Snackbar
                            .make(root_view_, "Not enough free space in the target device", Snackbar.LENGTH_SHORT)
                            .show();
                    return;
                }

                String str = new String(bytes);
                YabauseRunnable.putFile(str);
                Snackbar
                        .make(root_view_, "Finished!", Snackbar.LENGTH_SHORT)
                        .show();
            }
        }).addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception exception) {
                Snackbar
                        .make(root_view_, "Fail to download from cloud " + exception.getLocalizedMessage(), Snackbar.LENGTH_SHORT)
                        .show();
            }
        });

    }

    void deleteCloudItem(final BackupItem backupitemi){
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            return;
        }

        if( backupitemi.key.equals("") ){
            return;
        }

        FirebaseStorage storage = FirebaseStorage.getInstance();
        StorageReference storageRef = storage.getReference();
        StorageReference base = storageRef.child(auth.getCurrentUser().getUid());
        StorageReference backup = base.child("backup");
        StorageReference fileref = backup.child(backupitemi.key);
        fileref.delete();
        database_.child(backupitemi.key).removeValue();
    }

    void uploadToCloud( final BackupItem backupitemi ){
        String jsonstr = YabauseRunnable.getFile(backupitemi.index_);
        FirebaseAuth auth = FirebaseAuth.getInstance();
        if (auth.getCurrentUser() == null) {
            return;
        }
        // Managmaent part
        String baseurl = "/user-posts/" + auth.getCurrentUser().getUid()  + "/backup/";

        if( database_ == null ) {
            DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
            database_ = baseref.child(baseurl);
            if (database_ == null) {
                return;
            }
        }

        final String dbref;
        if( !backupitemi.key.equals("") ) {
            Map<String, Object> postValues = backupitemi.toMap();
            dbref = baseurl + backupitemi.key;
            database_.child(backupitemi.key).setValue(postValues);
        }else {
            DatabaseReference newPostRef = database_.push();
            dbref = baseurl + newPostRef.getKey();
            backupitemi.key = newPostRef.getKey();
            Map<String, Object> postValues = backupitemi.toMap();
            newPostRef.setValue(postValues);
        }

        // data part
        FirebaseStorage storage = FirebaseStorage.getInstance();
        StorageReference storageRef = storage.getReference();
        StorageReference base = storageRef.child(auth.getCurrentUser().getUid());
        StorageReference backup = base.child("backup");
        final StorageReference fileref = backup.child(backupitemi.key);

        byte[] data = jsonstr.getBytes();

        StorageMetadata metadata = new StorageMetadata.Builder()
                .setContentType("text/json")
                .setCustomMetadata("dbref", dbref )
                .setCustomMetadata("uid",auth.getCurrentUser().getUid())
                .setCustomMetadata("filename",backupitemi.filename)
                .setCustomMetadata("comment",backupitemi.comment)
                .setCustomMetadata("size",String.format("%,dByte",backupitemi.datasize))
                .setCustomMetadata("date",backupitemi.savedate)
                .build();

        UploadTask uploadTask = fileref.putBytes(data,metadata);


        // Listen for state changes, errors, and completion of the upload.
        StorageTask<UploadTask.TaskSnapshot> message = uploadTask.addOnProgressListener(new OnProgressListener<UploadTask.TaskSnapshot>() {
            @Override
            public void onProgress(UploadTask.TaskSnapshot taskSnapshot) {
                double progress = (100.0 * taskSnapshot.getBytesTransferred()) / taskSnapshot.getTotalByteCount();
                System.out.println("Upload is " + progress + "% done");
            }
        }).addOnPausedListener(new OnPausedListener<UploadTask.TaskSnapshot>() {
            @Override
            public void onPaused(UploadTask.TaskSnapshot taskSnapshot) {
                System.out.println("Upload is paused");
            }
        }).addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception exception) {
                Snackbar
                        .make(root_view_, "Failed to upload " + exception.getLocalizedMessage(), Snackbar.LENGTH_SHORT)
/*
                        .setAction("UNDO", new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                Log.d("Snackbar.onClick", "UNDO Clicked");
                            }
                        })
*/
                        .show();
            }
        }).addOnSuccessListener(new OnSuccessListener<UploadTask.TaskSnapshot>() {
            @Override
            public void onSuccess(UploadTask.TaskSnapshot taskSnapshot) {
                // Handle successful uploads on complete
                //Uri downloadUrl = taskSnapshot.getMetadata().getDownloadUrl();
                Snackbar
                        .make(root_view_, "Success to upload ", Snackbar.LENGTH_SHORT)
                        .show();

                String dbref = taskSnapshot.getMetadata().getCustomMetadata("dbref");

                //DatabaseReference database;
                //database = FirebaseDatabase.getInstance().getReference();
                //DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
                //DatabaseReference ref = baseref.child(dbref + "/url");
                //Uri url = taskSnapshot.getMetadata().getDownloadUrl();
                //ref.setValue(url.toString());

              }
        });

        Task<Uri> urlTask = uploadTask.continueWithTask(new Continuation<UploadTask.TaskSnapshot, Task<Uri>>() {
            @Override
            public Task<Uri> then(@NonNull Task<UploadTask.TaskSnapshot> task) throws Exception {
                if (!task.isSuccessful()) {
                    throw task.getException();
                }

                // Continue with the task to get the download URL
                return fileref.getDownloadUrl();
            }
        }).addOnCompleteListener(new OnCompleteListener<Uri>() {
            @Override
            public void onComplete(@NonNull Task<Uri> task) {
                if (task.isSuccessful()) {
                    Uri downloadUri = task.getResult();
                    DatabaseReference baseref  = FirebaseDatabase.getInstance().getReference();
                    DatabaseReference ref = baseref.child(dbref + "/url");
                    ref.setValue(downloadUri.toString());
                } else {
                    // Handle failures
                    // ...
                }
            }
        });
    }

}
