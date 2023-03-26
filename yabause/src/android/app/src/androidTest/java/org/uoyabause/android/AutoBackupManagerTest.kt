package org.uoyabause.android

import android.os.Build
import android.util.Log
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.google.firebase.storage.FirebaseStorage
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.BeforeClass
import org.junit.Test
import org.junit.runner.RunWith
import org.uoyabause.android.AutoBackupManager.AutoBackupManagerListener
import org.uoyabause.android.phone.placeholder.PlaceholderContent
import java.io.File
import java.io.FileOutputStream
import java.lang.Thread.sleep
import java.util.Random


@RunWith(AndroidJUnit4::class)
class AutoBackupManagerTest {

    var testState = 0

    fun waitAsync(){
        var timecount = 0
        val timeout = 3000
        timecount = 0
        while( testState == 0 ){
            sleep(10)
            timecount += 1
            if( timecount > timeout ){
                assertNotNull("time out",null)
            }
        }
        assertEquals("Fail async function", testState,1)
        testState = 0
    }

    companion object {
        var emuhost = "192.168.11.5"
        lateinit var database : FirebaseDatabase
        lateinit var storage : FirebaseStorage
        @JvmStatic
        @BeforeClass
        fun setUpClass() {

            val context = InstrumentationRegistry.getInstrumentation().targetContext
            if( Build.PRODUCT == "sdk_gphone_x86_64" ){
                emuhost = "10.0.2.2"
            }

            FirebaseAuth.getInstance().useEmulator(emuhost, 9099)
            FirebaseAuth.getInstance()
                .signInWithEmailAndPassword("devmiyax@gmail.com", "testpass01")

            val cu = FirebaseAuth.getInstance().currentUser

            var testState = 0
            database = FirebaseDatabase.getInstance()
            database.useEmulator(emuhost, 9000)
            //database.getReference("user-posts").child(cu!!.uid).removeValue { error, ref ->  testState = 1 }
            //while( testState == 0 ){sleep(10) }

            storage = FirebaseStorage.getInstance()
            storage.useEmulator(emuhost, 9199)
        }
    }

    @Before
    fun setUp() {
    }

    @After
    fun tearDown() {
        runBlocking {
        }
    }

    @Test
    fun downloadBackupMemory() {

        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(AutoBackupManager.ConflictResult.CLOUD)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)
                onMainThread()
            }
        }

        assertNotNull(FirebaseAuth.getInstance().currentUser)
        val currentUser = FirebaseAuth.getInstance().currentUser

        clearTestData()

        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)

        // テストデータを送信
        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)

        var outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()
        var premodtime = localFile.lastModified()

        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        // テストデータのファイル名を取得
        val backupReference = database.getReference("user-posts").child(currentUser!!.uid)
            .child("backupHistory")

        testState = 0
        var filename = ""
        backupReference.orderByChild("date")
            .limitToLast(1)
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        val latestData = dataSnapshot.children.last().value as Map<*, *>
                        assertNotNull(latestData["filename"])
                        filename = latestData["filename"] as String
                        testState = 1

                    }
                }
                override fun onCancelled(error: DatabaseError) {
                    testState = -1

                }
            })

        waitAsync()

        // テスト、テストデータをローカルに取得できることを確認
        val destinationDirectory = File(YabauseStorage.storage.getMemoryPath("/"))
        testTarget.isOnSubscription = true
        testTarget.downloadBackupMemory(
            filename,
            FirebaseAuth.getInstance().currentUser!!,
            destinationDirectory,
            null
        ){

        }
    }


    @Test
    fun uploadBackupMemory() {

        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(AutoBackupManager.ConflictResult.CLOUD)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)

                if( result == AutoBackupManager.SyncResult.FAIL ) {
                    testState = -1
                }

                onMainThread()
            }
        }

        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)

        clearTestData()

        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)


        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)
        val localUpdateTime = localFile.lastModified()

        testTarget.isOnSubscription = true

        testState = 0
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        val backupReference = database.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")

        testState = 0
        backupReference.orderByChild("date")
            .limitToLast(1)
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        val latestData = dataSnapshot.children.last().value as Map<*, *>
                        assertNotNull(latestData["deviceName"])
                        assertEquals(latestData["deviceName"], android.os.Build.MODEL )
                        assertNotNull(latestData["date"])
                        assertEquals(latestData["date"], localUpdateTime )
                        assertNotNull(latestData["md5"])
                        assertNotNull(latestData["filename"])
                        testState = 1

                    }
                }
                override fun onCancelled(error: DatabaseError) {
                    testState = -1

                }
            })

        waitAsync()
    }

    fun generateRandomByteArray(size: Int): ByteArray {
        val byteArray = ByteArray(size)
        Random().nextBytes(byteArray)
        return byteArray
    }

    private fun clearTestData(){
        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)

        testState = 0
        database.getReference("user-posts").child(currentUser!!.uid).removeValue().addOnCompleteListener {
            testState = 1
        }
        waitAsync()

        testState = 0
        val storageRef = storage.reference.child(currentUser.uid)
        storageRef.listAll()
            .addOnSuccessListener { listResult ->
                var delcount = 0
                val sz = listResult.items.size
                if( sz > 0 ) {
                    listResult.items.forEach { item ->
                        item.delete()
                            .addOnSuccessListener {
                                // ファイルの削除に成功した場合の処理
                                delcount++
                                if (delcount >= sz) {
                                    testState = 1
                                }

                            }
                            .addOnFailureListener { exception ->
                                // ファイルの削除に失敗した場合の処理
                                delcount++
                                if (delcount >= sz) {
                                    testState = 1
                                }

                            }
                    }
                }else{
                    testState = 1
                }
            }
            .addOnFailureListener { exception ->
                // ディレクトリ内のファイルの一覧取得に失敗した場合の処理
            }
        waitAsync()

    }


    @Test
    fun syncDownload() {
        /*
          クラウドにあるものが新しい場合アップロードされる
        */
        var testmode = 0
        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(AutoBackupManager.ConflictResult.CLOUD)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)

                if( result == AutoBackupManager.SyncResult.FAIL ) {
                    testState = -1
                    return
                }else {

                    if( testmode == 1) {
                        testState = 1
                    }

                }
                onMainThread()
            }
        }

        clearTestData()
        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)

        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)

        var outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()
        var premodtime = localFile.lastModified()

        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)

        // 現在のデータをアップロードする
        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        // ファイルの日付を古くする
        //var modtime = System.currentTimeMillis()
        outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()
        localFile.setLastModified(premodtime - 105000)

        val lmd5 = testTarget.escapeFileName(testTarget.calculateMD5(localFile))

        // テストしたい処理、クラウドのほうが古いのでダウンロードされるはず
        testState = 0
        testmode = 1
        testTarget.syncBackup()
        waitAsync()

        val dlocalFile = File(mem)
        val localUpdateTime = dlocalFile.lastModified()

        assertEquals("Confirm if same time as download time",localUpdateTime,premodtime)
        //assertEquals("Confirm if same md5",md5,lmd5)

    }

    @Test
    fun syncUpload() {
      /*
        クラウドにあるものが古い場合アップロードされる
      */

        var testmode = 0
        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(AutoBackupManager.ConflictResult.CLOUD)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)

                if( result == AutoBackupManager.SyncResult.FAIL ) {
                    testState = -1
                    return
                }else {

                    if( testmode == 1) {
                        testState = 1
                    }

                }
                onMainThread()
            }
        }

        clearTestData()
        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)


        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)
        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)


        var outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        // 現在のデータをアップロードする
        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        //一秒進める
        sleep(1000)

        outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        // 新しくなったファイルを送信する
        var modtime = System.currentTimeMillis()
        localFile.setLastModified(modtime)
        modtime = localFile.lastModified()
        val lmd5 = testTarget.escapeFileName(testTarget.calculateMD5(localFile))

        // テストしたい処理、クラウドのほうが古いのでアップロードされるはず
        testState = 0
        testmode = 1
        testTarget.syncBackup()
        waitAsync()

        // アップロードされたことを確認する
        val backupReference = database.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")
        testState = 0
        var localUpdateTime = 0L
        var md5 = ""
        backupReference.orderByChild("date")
            .limitToLast(1)
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        val latestData = dataSnapshot.children.last().value as Map<*, *>
                        localUpdateTime = latestData["date"] as Long
                        md5 = latestData["md5"] as String
                        testState = 1
                    }
                }
                override fun onCancelled(error: DatabaseError) {
                    testState = -1

                }
            })

        waitAsync()
        assertEquals("Confirm if same time as upload time",localUpdateTime,modtime)
        assertEquals("Confirm if same md5",md5,lmd5)
    }

    @Test
    fun syncConflictLocal() {
        syncConflict(AutoBackupManager.ConflictResult.LOCAL)
    }

    @Test
    fun syncConflictCloud() {
        syncConflict(AutoBackupManager.ConflictResult.CLOUD)
    }

    fun syncConflict( conflictMode: AutoBackupManager.ConflictResult) {
        /*
          自分のデータがクラウドに上げる前に、ほかのデバイスがクラウドに上げてしまって、自分のデータを残すか、クラウドのデータを取得するか、ユーザーの判断が必要な状況
        */

        var lmd5 = ""
        var testmode = 0
        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(conflictMode)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)

                if( result == AutoBackupManager.SyncResult.FAIL ) {
                    testState = -1
                    return
                }else {

                    if( testmode == 1) {
                        testState = 1
                    }

                }
                onMainThread()
            }
        }

        clearTestData()
        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)


        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)
        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)

        var outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        // 現在のデータをアップロードする
        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        //5秒進める
        sleep(5000)

        // 新しくなったファイルを送信する
        outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        var modtime = localFile.lastModified()
        if( conflictMode == AutoBackupManager.ConflictResult.CLOUD) {
            lmd5 = testTarget.escapeFileName(testTarget.calculateMD5(localFile))
        }


        // テストしたい処理、クラウドのほうが古いのでアップロードされるはず
        testState = 0
        testmode = 1
        testTarget.syncBackup()
        waitAsync()

        // この中間のファイルを作成する

        // 新しくなったファイルを送信する
        outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        // 最新のファイルから3秒戻した時間にする
        localFile.setLastModified(modtime - 3000)

        // テストがローカルモードのときはローカルファイルを正解データとする
        if( conflictMode == AutoBackupManager.ConflictResult.LOCAL) {
            lmd5 = testTarget.escapeFileName(testTarget.calculateMD5(localFile))
            modtime = localFile.lastModified()
        }


        // テストしたい処理、コンフリクトが検出される
        testState = 0
        testmode = 1
        testTarget.syncBackup()
        waitAsync()

        // ローカルのファイルがアップロードされたことを確認する
        val backupReference = database.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")
        testState = 0
        var localUpdateTime = 0L
        var md5 = ""
        backupReference.orderByChild("date")
            .limitToLast(1)
            .addListenerForSingleValueEvent(object : ValueEventListener {
                override fun onDataChange(dataSnapshot: DataSnapshot) {
                    if (dataSnapshot.exists()) {
                        val latestData = dataSnapshot.children.last().value as Map<*, *>
                        localUpdateTime = latestData["date"] as Long
                        md5 = latestData["md5"] as String
                        testState = 1
                    }
                }
                override fun onCancelled(error: DatabaseError) {
                    testState = -1

                }
            })

        waitAsync()
        if( conflictMode == AutoBackupManager.ConflictResult.CLOUD) {
            assertEquals("Confirm if same time as upload time",localUpdateTime,modtime)
            assertEquals("Confirm if same md5",md5,lmd5)
        } else {
            assertNotEquals("Confirm if same time as upload time", localUpdateTime, modtime)
            assertEquals("Confirm if same md5", md5, lmd5)
        }
    }

    @Test
    fun testRollBackMemory(){
        /*
          ロールバックのテスト
        */

        var lmd5 = ""
        var testmode = 0
        val listener = object : AutoBackupManagerListener {
            override fun enable(): Boolean {
                return true
            }

            override fun getTitle(): String {
                return "test title"
            }

            override fun askConflict(onResult: (result: AutoBackupManager.ConflictResult) -> Unit) {
                onResult(AutoBackupManager.ConflictResult.CLOUD)
            }

            override fun onStartSyncBackUp() {

            }

            override fun onFinish(
                result: AutoBackupManager.SyncResult,
                message: String,
                onMainThread: () -> Unit,
            ) {
                assertNotEquals( result, AutoBackupManager.SyncResult.FAIL)

                if( result == AutoBackupManager.SyncResult.FAIL ) {
                    testState = -1
                    return
                }else {

                    if( testmode == 1) {
                        testState = 1
                    }

                }
                onMainThread()
            }
        }

        clearTestData()
        val currentUser = FirebaseAuth.getInstance().currentUser
        assertNotNull(currentUser)

        val mem = YabauseStorage.storage.getMemoryPath("memory.ram")
        val localFile = File(mem)
        val testTarget = AutoBackupManager(
            listener
        )
        testTarget.setDatabase(database)
        testTarget.setStorage(storage)

        var outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()
        var modtime = localFile.lastModified()
        lmd5 = testTarget.escapeFileName(testTarget.calculateMD5(localFile))

        // 現在のデータをアップロードする
        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        //5秒進める
        sleep(5000)

        // 新しくなったファイルを送信する
        outputStream = FileOutputStream(localFile)
        outputStream.write(generateRandomByteArray(512) )
        outputStream.flush()
        outputStream.close()

        testState = 0
        testTarget.isOnSubscription = true
        testTarget.uploadBackupMemory(localFile,currentUser!!){
            testState = 1
        }
        waitAsync()

        testState = 0
        val latestData  = mutableListOf<Map<*, *>>()
        val backupReference = database.getReference("user-posts").child(currentUser.uid)
            .child("backupHistory")

        val query = backupReference.orderByChild("date")
        query.addListenerForSingleValueEvent(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val children = snapshot.children.reversed()
                for (childSnapshot in children) {
                    var mapdata = (childSnapshot.value as Map<*, *>).toMutableMap()
                    mapdata["key"] = childSnapshot.key
                    latestData.add(mapdata)

                }
                testState = 1
            }
            override fun onCancelled(error: DatabaseError) {
                testState = -1
            }
        })
        waitAsync()

        Log.d(this.javaClass.canonicalName,"before ${latestData.toString()}")
        val expectedFilename = latestData.last()["filename"]
        val expectedMd5 = latestData.last()["md5"]

        sleep(5000)

        testState = 0
        testmode = 1
        testTarget.rollBackMemory( latestData.last()["filename"] as String, latestData.last()["key"] as String ){

        }
        waitAsync() // syncBackupまで実行されるはず



        testState = 0
        latestData.clear()
        query.addListenerForSingleValueEvent(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val children = snapshot.children.reversed()
                for (childSnapshot in children) {
                    var mapdata = (childSnapshot.value as Map<*, *>).toMutableMap()
                    mapdata["key"] = childSnapshot.key
                    latestData.add(mapdata)

                }
                testState = 1
            }
            override fun onCancelled(error: DatabaseError) {
                testState = -1
            }
        })
        waitAsync()

        Log.d(this.javaClass.canonicalName,"after ${latestData.toString()}")

        assertEquals("Confirm if last data to be top",expectedFilename,latestData.first()["filename"])
        assertEquals("Confirm if last md5 to be top",expectedMd5,latestData.first()["md5"])

    }



}