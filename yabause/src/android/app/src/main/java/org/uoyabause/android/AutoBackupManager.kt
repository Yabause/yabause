package org.uoyabause.android

import android.util.Log
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.storage.FirebaseStorage
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.util.Date
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import java.util.zip.ZipOutputStream
import android.util.Base64;
import java.text.SimpleDateFormat
import java.util.TimeZone

class AutoBackupManager(
    listener: AutoBackupManagerListener,
) {
    interface AutoBackupManagerListener {
        fun enable() : Boolean
        fun getTitle() : String
        fun askConflict( onResult: ( result : ConflictResult ) -> Unit )
        fun onStartSyncBackUp()
        fun onFinish(result: AutoBackupManager.SyncResult,message: String, onMainThread: () -> Unit )
    }

    val listener_: AutoBackupManagerListener

    var database_ : FirebaseDatabase
    var storage_ : FirebaseStorage

    //private val mFirebaseAnalytics: FirebaseAnalytics
    private val TAG = "AutoBackupManager"
    private var backupReference: DatabaseReference? = null
    private var backupListener: ValueEventListener? = null

    var isOnSubscription = false

    enum class BackupSyncState {
        IDLE,
        CHECKING_DOWNLOAD,
        CHECKING_UPLOAD
    }

    enum class SyncResult {
        SUCCESS,
        FAIL,
        DONOTING
    }

    enum class ConflictResult {
        LOCAL,
        CLOUD
    }

    init {
        listener_ = listener
        //mFirebaseAnalytics = FirebaseAnalytics.getInstance(target_.requireActivity())
        database_ = FirebaseDatabase.getInstance()
        storage_ = FirebaseStorage.getInstance()
    }

    fun setDatabase( ndatabase : FirebaseDatabase){
        database_ = ndatabase
    }

    fun setStorage( nstorage : FirebaseStorage){
        storage_ = nstorage
    }


    var syncState = BackupSyncState.IDLE
        private set

    fun onPause(){
        if( backupReference != null && backupListener != null ){
            backupReference!!.removeEventListener(backupListener!!)
            backupReference = null
            backupListener = null
        }
    }

    fun onResume(){
        val auth = FirebaseAuth.getInstance()
        if (auth.currentUser != null && backupReference == null && backupListener == null ) {
            startSubscribeBackupMemory(auth.currentUser!!)
        }
    }

    fun calculateMD5(file: File): String {
        val md = MessageDigest.getInstance("MD5")
        val inputStream = file.inputStream()
        inputStream.use { input ->
            val buffer = ByteArray(4096)
            var bytesRead = input.read(buffer)
            while (bytesRead != -1) {
                md.update(buffer, 0, bytesRead)
                bytesRead = input.read(buffer)
            }
        }
        val bytes = md.digest()
        return Base64.encodeToString(bytes,Base64.NO_WRAP)
    }


    fun zip(sourceFile: File, zipFile: File) : Int {
        Log.d(TAG,"zip time = ${unixTimeToDateString(sourceFile.lastModified())}")
        val buffer = ByteArray(4096)
        try {
            val outputStream = FileOutputStream(zipFile.absolutePath)
            ZipOutputStream(outputStream).use { zipOut ->
                FileInputStream(sourceFile).use { fileIn ->
                    val entry = ZipEntry(sourceFile.name)
                    entry.setTime(sourceFile.lastModified())
                    zipOut.putNextEntry(entry)
                    var len: Int
                    while (fileIn.read(buffer).also { len = it } > 0) {
                        zipOut.write(buffer, 0, len)
                    }
                    zipOut.closeEntry()
                }
            }
        }catch( e:Exception ){
            Log.e(TAG,"${e.message}")
            return -1
        }
        return 0
    }

    fun unzip(zipFile: File, destinationDirectory: File, setTime: Long?) : Int {
        try {
            val buffer = ByteArray(4096)
            val zipIn = ZipInputStream(zipFile.inputStream())
            var entry = zipIn.nextEntry
            while (entry != null) {
                val outputFile = File(destinationDirectory, entry.name)
                if (entry.isDirectory) {
                    outputFile.mkdirs()
                } else {
                    outputFile.parentFile?.mkdirs()
                    val fos = FileOutputStream(outputFile)
                    var len = zipIn.read(buffer)
                    while (len > 0) {
                        fos.write(buffer, 0, len)
                        len = zipIn.read(buffer)
                    }
                    fos.close()
                }
                // ファイルの日付情報を設定
                if( setTime != null) {
                    Log.d(TAG, "unzip time = ${unixTimeToDateString(setTime)}")
                    outputFile.setLastModified(setTime)
                }
                entry = zipIn.nextEntry
            }
            zipIn.closeEntry()
            zipIn.close()
        }catch( e : Exception ){
            Log.e(TAG,"${e.message}")
            return -1
        }
        return 0
    }

    fun unixTimeToDateString(unixTime: Long): String {
        val date = Date(unixTime)
        val formatter = SimpleDateFormat("yyyy-MM-dd HH:mm:ss")
        formatter.timeZone = TimeZone.getDefault()
        return formatter.format(date)
    }

    fun escapeFileName(fileName: String): String {
        return fileName.replace("/", "_")
    }

    fun rollBackMemory( downloadFilename : String, key : String,onFinish: () -> Unit ){
        val auth = FirebaseAuth.getInstance()
        val currentUser = auth.currentUser
        if (currentUser != null) {

            val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
            downloadBackupMemory(
                downloadFilename,
                currentUser,
                destinationDirectory,
                null
            ) {
                val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
                val localFile = File(mem)
                val localUpdateTime = localFile.lastModified()
                val backupReference =
                    database_.getReference("user-posts").child(currentUser.uid)
                        .child("backupHistory")

                val dataref = backupReference.child(key)

                dataref.child("date").setValue(localUpdateTime)
                    .addOnCompleteListener { task ->
                        if (task.isSuccessful) {
                            // データの更新が成功した場合に実行される処理
                            //Log.d(TAG, "Data updated successfully.")
                            syncBackup()
                        } else {
                            // データの更新が失敗した場合に実行される処理
                            //Log.w(TAG, "Failed to update data.", task.exception)
                        }
                        onFinish()
                    }


            }
        }
    }

    fun downloadBackupMemory(
        downloadFileName: String,
        currentUser: FirebaseUser,
        destinationDirectory: File,
        setTime: Long?,
        onSccess: () -> Unit
    )
    {
/*
        mFirebaseAnalytics.logEvent(
            TAG
        ){
            param("event", "downloadBackupMemory")
        }
*/
        if( isOnSubscription == false ){
            return
        }

        if( listener_.enable() == false ){
            return
        }


        val storageRef = storage_.reference.child(currentUser.uid).child("${downloadFileName}")
        val memzip = YabauseStorage.storage.getMemoryPath("${downloadFileName}")
        val localZipFile = File(memzip)

        storageRef.getFile(localZipFile)
            .addOnFailureListener(){
                    exception ->
                Log.d(TAG, "Upload Fail ${exception.message}")
                localZipFile.delete()
                listener_.onFinish(SyncResult.FAIL, "Fail to upload backup data to cloud" ){
                    syncState = BackupSyncState.IDLE
                }

            }
            .addOnCompleteListener { task ->
            if (task.isSuccessful) {

                Log.d(TAG, "Download OK")

                // ダウンロード成功時の処理
                if( unzip(localZipFile, destinationDirectory,setTime) != 0 ){
                    localZipFile.delete()
                    listener_.onFinish(SyncResult.FAIL, "Fail to unzip backup data from cloud"){
                        syncState = BackupSyncState.IDLE
                    }

                }else {
                    localZipFile.delete()
                    listener_.onFinish(SyncResult.SUCCESS,
                        "Success to download backup data from cloud"){
                        syncState = BackupSyncState.IDLE
                    }

                        listener_.onFinish(
                            SyncResult.SUCCESS,
                            "Success to download backup data from cloud"
                        ) {
                            syncState = BackupSyncState.IDLE
                            onSccess()
                        }
                }

            } else {
                // ダウンロード失敗時の処理
                listener_.onFinish(SyncResult.FAIL, "Fail to download backup data from cloud"){
                    syncState = BackupSyncState.IDLE
                }

            }


        }

    }




    fun uploadBackupMemory(
        localFile: File,
        currentUser: FirebaseUser,
        onSccess: () -> Unit
    ) {
/*
        mFirebaseAnalytics.logEvent(
            TAG
        ){
            param("event", "uploadBackupMemory")
        }
*/
        if( isOnSubscription == false ){
            return
        }

        if( listener_.enable() == false ){
            return
        }

        listener_.enable()

        val localUpdateTime = localFile.lastModified()
        val lmd5 = escapeFileName(calculateMD5(localFile))
        val memzip = YabauseStorage.storage.getMemoryPath("$lmd5}.zip")
        val localZipFile = File(memzip)
        if( zip(localFile, localZipFile) != 0 ){
            listener_.onFinish(SyncResult.FAIL, "Fail to zip backup data"){
                syncState = BackupSyncState.IDLE
            }
            return
        }

        val md5 = calculateMD5(localZipFile)
        val storageRef = storage_.reference.child(currentUser.uid).child("${lmd5}.zip")

        // クラウドのファイルが古い場合、ローカルのファイルをアップロード
        val inputStream = FileInputStream(localZipFile)
        storageRef.putStream(inputStream).addOnFailureListener{
                exception ->
            Log.d(TAG, "Upload Fail ${exception.message}")
            localZipFile.delete()
            listener_.onFinish(SyncResult.FAIL, "Fail to upload backup data to cloud" ){
                syncState = BackupSyncState.IDLE
            }
        }
            .addOnSuccessListener  {
                // アップロード成功時の処理
             Log.d(TAG, "Upload OK")
                val backupReference = database_.getReference("user-posts").child(currentUser.uid)
                    .child("backupHistory")


                val lastPlayGameName = listener_.getTitle()


                val key = backupReference.push().key
                if (key != null) {

                    val data = hashMapOf(
                        "deviceName" to android.os.Build.MODEL,
                        "date" to localUpdateTime,
                        "md5" to lmd5,
                        "filename" to "${lmd5}.zip",
                        "lastPlayGameName" to "${lastPlayGameName}"
                    )

                    backupReference.child(key).setValue(data, object : DatabaseReference.CompletionListener {
                        override fun onComplete(error: DatabaseError?, ref: DatabaseReference) {
                            if (error != null) {
                                // setValue()がエラーで終了した場合の処理を記述
                                localZipFile.delete()
                                listener_.onFinish(SyncResult.FAIL, "Fail to upload backup data to cloud ${error.message}" ){
                                    syncState = BackupSyncState.IDLE
                                }
                            } else {
                                Log.d(TAG, "path = user-posts/${currentUser.uid}/backupHistory/${key}")
                                Log.d(TAG, "data = ${data}")
                                localZipFile.delete()
                                listener_.onFinish(SyncResult.SUCCESS, "Success to upload backup data to cloud" ){
                                    syncState = BackupSyncState.IDLE
                                }
                                checkAndRemoveLastData(currentUser)
                                onSccess()
                            }
                        }
                    })
                }

        }
    }

    fun startSubscribeBackupMemory( currentUser: FirebaseUser){
/*
        mFirebaseAnalytics.logEvent(
            TAG
        ){
            param("event", "startSubscribeBackupMemory")
        }
*/
        if( isOnSubscription == false ){
            return
        }

        if( listener_.enable() == false ) {
            return
        }


        if( backupReference != null ) return
        if( backupListener != null ) return

        // 更新履歴情報にアクセスする
        backupReference = database_.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")

        backupListener = backupReference!!
            .orderByChild("date")
            .limitToLast(1)
            // 最新の更新履歴を取得する
            .addValueEventListener(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if( dataSnapshot.value != null ) {
                        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
                        val localFile = File(mem)
                        if (localFile.exists()) {
                            val latestData = dataSnapshot.children.first().value as Map<*, *>
                            val cloudUpdateTime = latestData["date"] as Long
                            val localUpdateTime = localFile.lastModified()

                            // クラウドのほうが新しい場合ダウンロード
                            if (cloudUpdateTime > localUpdateTime) {
                                syncBackup()
                            }
                        }
                    }
                }

                override fun onCancelled(error: DatabaseError) {

                }
            })



    }

    fun checkAndRemoveLastData(currentUser: FirebaseUser){

        if( isOnSubscription == false ){
            return
        }

        if( listener_.enable() == false ) {
            return
        }

        // 更新履歴情報にアクセスする
        val backupReference = database_.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")

        Log.d(TAG, "checkAndRemoveLastData")

        backupReference.addListenerForSingleValueEvent(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val numRefs = snapshot.childrenCount

                Log.d(TAG, "Backup count is ${numRefs}")

                // １０世代以上の物は削除する
                if( numRefs > 10 ){

                    Log.d(TAG, "Removing the oldest file")

                    val query = backupReference.orderByChild("date").limitToFirst(1)
                    query.addListenerForSingleValueEvent(object : ValueEventListener {
                        override fun onDataChange(snapshot: DataSnapshot) {
                            for (childSnapshot in snapshot.children) {
                                // 一番古いデータのリファレンスを取得します
                                val oldestRef = childSnapshot.ref

                                val latestData = childSnapshot.value as Map<*, *>
                                val downloadFilename = latestData["filename"] as String

                                val fileRef = storage_.reference.child(currentUser.uid).child("${downloadFilename}")
                                fileRef.delete()
                                    .addOnSuccessListener {
                                        // リファレンスを削除します
                                        Log.d(TAG, "Success remove old data")
                                        oldestRef.removeValue()

                                    }
                                    .addOnFailureListener { e ->
                                        // 削除に失敗した場合の処理を実装します
                                        Log.d(TAG, "Fail to delete storage file ${e.message}")
                                    }

                            }
                        }

                        override fun onCancelled(error: DatabaseError) {
                            // エラーが発生した場合の処理を実装します
                            Log.d(TAG, "Fail to delete storage file ${error.message}")
                        }
                    })
                }
            }
            override fun onCancelled(error: DatabaseError) {
                // エラーが発生した場合の処理を実装します
                Log.d(TAG, "Fail to delete storage file ${error.message}")
            }
        })


    }

    fun syncBackup() {

        if (isOnSubscription == false) {
            return
        }

        if( listener_.enable() == false ) {
            return
        }

        // 同期中は何もしない
        if (syncState == BackupSyncState.CHECKING_DOWNLOAD) {
            return
        }


        syncState = BackupSyncState.CHECKING_DOWNLOAD
        listener_.onStartSyncBackUp()


        val auth = FirebaseAuth.getInstance()
        val currentUser = auth.currentUser
        if (currentUser == null) {
            syncState = BackupSyncState.IDLE
            Log.d(TAG, "Fail to get currentUser even if Auth is successed!")
            listener_.onFinish(SyncResult.DONOTING, ""){

            }
            return
        }

        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")

        val localFile = File(mem)
        var storageFileNmae = "memory.zip"

        val storageRef = storage_.reference.child(currentUser.uid).child("memory.zip")
        val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
        val memzip = YabauseStorage.storage.getMemoryPath("memory.zip")
        val localZipFile = File(memzip)


        // 更新履歴情報にアクセスする
        val backupReference = database_.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")

        Log.d(TAG, "syncBackup")

        // 最新の更新履歴を取得する
        backupReference.orderByChild("date")
            .limitToLast(2)
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        val latestData = dataSnapshot.children.last().value
                        // ローカルにファイルが存在しない場合 => 強制ダウンロード
                        if (localFile.exists() == false) {
                            Log.d(TAG, "Local file not exits force download")

                            val latestData = dataSnapshot.children.first().value as Map<*, *>
                            val downloadFilename = latestData["filename"] as String

                            downloadBackupMemory(
                                downloadFilename,
                                currentUser,
                                destinationDirectory,
                                latestData["date"] as Long?
                            ) {
                                startSubscribeBackupMemory(currentUser)
                            }


                        // ローカルにファイルが存在する場合、クラウドのほうが新しかったらダウンロード
                        } else {

                            val latestData = dataSnapshot.children.last().value as Map<*, *>
                            val cloudUpdateTime = latestData["date"] as Long
                            val localUpdateTime = localFile.lastModified()

                            //Log.d(TAG, "path = user-posts/${currentUser.uid}/${key}")
                            Log.d(TAG, "latestData = ${latestData}")
                            Log.d(
                                TAG,
                                "localUpdateTime = ${unixTimeToDateString(localUpdateTime)} cloudUpdateTime = ${unixTimeToDateString(cloudUpdateTime)}"
                            )


                            // クラウドのほうが古い => アップロード
                            if (cloudUpdateTime < localUpdateTime) {
                                Log.d(TAG, "Local file is newer than cloud")

                                uploadBackupMemory(
                                    localFile,
                                    currentUser
                                ) {
                                    startSubscribeBackupMemory(currentUser)
                                }
                            }

                            // クラウドのほうが新しい => ダウンロード
                            else if (cloudUpdateTime > localUpdateTime) {

                                // コンフリクトの検出
                                // 最新の日付よりも
                                //repeat(dataSnapshot.children.count()) {
                                val preLastData =
                                    dataSnapshot.children.first().value as Map<*, *>
                                val predate = preLastData["date"] as Long
                                if (predate < localUpdateTime) {

                                    listener_.askConflict(){ result ->
                                        when(result){
                                            ConflictResult.LOCAL -> {
                                                localFile.setLastModified(Date().time)
                                                uploadBackupMemory(
                                                    localFile,
                                                    currentUser
                                                ) {
                                                    startSubscribeBackupMemory(currentUser)
                                                }
                                            }

                                            ConflictResult.CLOUD -> {
                                                val downloadFilename =
                                                    latestData["filename"] as String

                                                // Download
                                                downloadBackupMemory(
                                                    downloadFilename,
                                                    currentUser,
                                                    destinationDirectory,
                                                    predate
                                                ) {
                                                    startSubscribeBackupMemory(currentUser)
                                                }

                                            }
                                        }

                                    }
                                    return
                                }
                                //}

                                val downloadFilename = latestData["filename"] as String

                                Log.d(TAG, "Local file is older than cloud")
                                downloadBackupMemory(
                                    downloadFilename,
                                    currentUser,
                                    destinationDirectory,
                                    cloudUpdateTime
                                ) {
                                    startSubscribeBackupMemory(currentUser)
                                }

                            } else {
                                Log.d(TAG, "Local file is same as cloud ")
                                listener_.onFinish(SyncResult.DONOTING, ""){
                                    syncState = BackupSyncState.IDLE
                                }
                            }
                        }

                    } else {
                        // データが存在しない場合の処理
                        Log.d(TAG, "backup history does not exits on cloud history")
                        uploadBackupMemory(
                            localFile,
                            currentUser
                        ) {
                            startSubscribeBackupMemory(currentUser)
                        }
                    }
                }

                override fun onCancelled(databaseError: DatabaseError) {
                    // データの読み取りに失敗した場合の処理
                    Log.d(TAG, "Fail to access ${backupReference}")
                    uploadBackupMemory(
                        localFile,
                        currentUser
                    ) {
                        startSubscribeBackupMemory(currentUser)
                    }
                }
            })
    }

}