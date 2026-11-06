package dev.r15cluster.r15_companion

import android.Manifest
import android.app.*
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.*
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.*

/** Started foreground lifetime owns one BleBond; binding only observes it. */
class ClusterSyncService : Service() {
    companion object {
        const val CONNECT = "dev.r15cluster.CONNECT"
        const val STOP_GUIDANCE = "dev.r15cluster.STOP_GUIDANCE"
        const val STOP = "dev.r15cluster.STOP"
        private const val CHANNEL = "cluster-sync"
        private const val NOTIFICATION = 15
    }
    inner class LocalBinder : Binder() { val service get() = this@ClusterSyncService }
    private val binder = LocalBinder()
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var link: BleBond
    private val prefs get() = getSharedPreferences("ble-selection", MODE_PRIVATE)
    private var wanted = false
    private var foreground = false
    private var retryScheduled = false
    private var retryAttempt = 0
    private var destroyed = false
    private var noticeState = ""
    private var serviceError: String? = null
    private var starting = false
    private var mapsInfoAt = -30000L
    private var mapsAvailable = false
    private var mapsVersionValue: String? = null
    private var guidanceWanted=false
    private var guidanceStopReason=1
    private var hadListener=false
    private var lastSourceRefresh=0L
    private val sourceObserver: () -> Unit = { guidanceSourceChanged() }
    private val guidanceClock=object: Runnable {
        override fun run(){
            if(destroyed)return
            guidanceSourceChanged()
            if(guidanceWanted&&MapsNotifications.bound&&SystemClock.elapsedRealtime()-lastSourceRefresh>=2000){lastSourceRefresh=SystemClock.elapsedRealtime();MapsNotifications.refresh()}
            if(observer!=null)observer?.invoke(snapshot())
            handler.postDelayed(this,if(wanted||observer!=null)1000 else 5000)
        }
    }
    var observer: ((Map<String, Any?>) -> Unit)? = null
    private val retry = Runnable {
        retryScheduled = false
        if (!wanted || !eligible()) { stopSync("Bluetooth access or the saved bond is unavailable. Open the app to reconnect."); return@Runnable }
        val address = prefs.getString("address", null) ?: return@Runnable
        val name = prefs.getString("name", "R15 Cluster") ?: "R15 Cluster"
        connect(address, name, true)
    }
    private val radio = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.action == BluetoothAdapter.ACTION_STATE_CHANGED &&
                intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1) != BluetoothAdapter.STATE_ON && wanted) {
                stopSync("Bluetooth turned off. Open the app to reconnect after enabling it.")
            }
            if (wanted && intent.action == android.bluetooth.BluetoothDevice.ACTION_BOND_STATE_CHANGED &&
                intent.getIntExtra(android.bluetooth.BluetoothDevice.EXTRA_BOND_STATE, -1) == android.bluetooth.BluetoothDevice.BOND_NONE) {
                val device = intent.getParcelableExtra<android.bluetooth.BluetoothDevice>(android.bluetooth.BluetoothDevice.EXTRA_DEVICE)
                try {
                    if (device?.address == prefs.getString("address", null)) stopSync("The saved Android bond was removed. Pair explicitly again.")
                } catch (_: SecurityException) { stopSync("Bluetooth permission was revoked. Restore access and reconnect.") }
            }
        }
    }
    override fun onCreate() {
        super.onCreate()
        LegacyNavigationCleanup.start(applicationContext)
        link = BleBond(applicationContext) { changed(it) }
        MapsNotifications.observer=sourceObserver
        handler.post(guidanceClock)
        if (Build.VERSION.SDK_INT >= 26) {
            getSystemService(NotificationManager::class.java).createNotificationChannel(
                NotificationChannel(CHANNEL, "Cluster connection", NotificationManager.IMPORTANCE_LOW).apply {
                    description = "Shows active cluster sync and lets you stop it."
                })
        }
        val filter = IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED).apply { addAction(android.bluetooth.BluetoothDevice.ACTION_BOND_STATE_CHANGED) }
        if (Build.VERSION.SDK_INT >= 33) registerReceiver(radio, filter, RECEIVER_EXPORTED) else registerReceiver(radio, filter)
    }
    override fun onBind(intent: Intent): IBinder = binder
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if(intent?.action==STOP_GUIDANCE){stopGuidance();return START_NOT_STICKY}
        if (intent?.action == STOP) { stopSync(); return START_NOT_STICKY }
        // No boot receiver or blind restart from a persisted "connected" flag.
        if (intent?.action != CONNECT) { stopSync(); return START_NOT_STICKY }
        val address = intent.getStringExtra("address")
        if (!permitted() || address == null || !BluetoothAdapter.checkBluetoothAddress(address)) {
            stopSync("Restore Nearby devices access and Bluetooth before connecting."); return START_NOT_STICKY
        }
        val before = link.snapshot()
        if (wanted && before["state"] in listOf("connected", "connecting", "pairing") &&
            (before["address"] == address || before["state"] != "connected")) return START_NOT_STICKY
        handler.removeCallbacks(retry); retryScheduled = false; retryAttempt = 0
        wanted = true; serviceError = null
        prefs.edit().putBoolean("stopped", false).apply()
        try {
            showNotification("Connecting")
            link.setDiagnostics(intent.getBooleanExtra("diagnostics", false))
            connect(address, intent.getStringExtra("name") ?: "R15 Cluster", intent.getBooleanExtra("saved", false))
        } catch (_: Exception) { stopSync("Android could not start cluster sync. Open the app and retry.") }
        return START_NOT_STICKY
    }
    private fun permitted(): Boolean = try {
        val access = if (Build.VERSION.SDK_INT >= 31)
            checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED &&
                checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
        else checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
        access && (getSystemService(BLUETOOTH_SERVICE) as BluetoothManager).adapter?.isEnabled == true
    } catch (_: Exception) { false }
    private fun eligible() = permitted() && link.snapshot()["bonded"] == true && prefs.getString("address", null) != null
    private fun connect(address: String, name: String, saved: Boolean) {
        starting = true
        try { link.connect(address, name, saved) { } }
        finally { starting = false }
        changed(link.snapshot())
    }
    fun snapshot(): Map<String, Any?> = link.snapshot() + mapOf(
        "guidance" to guidanceSnapshot(),
        "serviceActive" to wanted, "reconnecting" to retryScheduled,
        "stopped" to prefs.getBoolean("stopped", false),
        "notificationVisible" to (Build.VERSION.SDK_INT < 24 || getSystemService(NotificationManager::class.java).areNotificationsEnabled()),
    ) + (serviceError?.let { mapOf("error" to it) } ?: emptyMap())
    private fun changed(value: Map<String, Any?>) {
        if (destroyed) return
        guidanceSourceChanged(false)
        if (wanted) {
            if (!permitted() || (value["state"] == "connected" && value["bonded"] != true)) {
                stopSync("Bluetooth access or the saved bond is unavailable. Open the app to reconnect."); return
            }
            when (value["state"]) {
                "connected" -> {
                    retryAttempt = 0; retryScheduled = false; handler.removeCallbacks(retry)
                    showNotification("Connected")
                }
                "disconnected" -> if (!starting) {
                    if (value["retryAllowed"] == false || !eligible()) {
                        stopSync(value["error"] as? String ?: "Connection stopped. Open the app to retry."); return
                    }
                    if (!retryScheduled) {
                        val delay = if (retryAttempt < 4) 1000L shl retryAttempt++ else { retryAttempt = 0; 30000L }
                        retryScheduled = true
                        handler.postDelayed(retry, delay)
                        android.util.Log.i("R15BleLink", "reconnect attempt=$retryAttempt delayMs=$delay")
                    }
                    showNotification("Reconnecting")
                }
            }
        }
        observer?.invoke(snapshot())
    }
    private fun showNotification(state: String) {
        val notice=state+if(guidanceWanted) " · Maps guidance" else ""
        if (noticeState == notice && foreground) return
        noticeState = notice
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        val open = PendingIntent.getActivity(this, 0, Intent(this, MainActivity::class.java), flags)
        val stop = PendingIntent.getService(this, 1, Intent(this, ClusterSyncService::class.java).setAction(STOP), flags)
        val builder = if (Build.VERSION.SDK_INT >= 26) Notification.Builder(this, CHANNEL) else Notification.Builder(this)
        if(guidanceWanted){
            val stopGuidance=PendingIntent.getService(this,2,Intent(this,ClusterSyncService::class.java).setAction(STOP_GUIDANCE),flags)
            builder.addAction(Notification.Action.Builder(null,"Stop guidance",stopGuidance).build())
        }
        val notification = builder.setSmallIcon(R.drawable.ic_cluster_sync).setContentTitle("R15 Cluster · $notice")
            .setContentText("Tap to open the app. Use Stop sync to disconnect.")
            .setContentIntent(open).setOngoing(true).setOnlyAlertOnce(true)
            .setCategory(Notification.CATEGORY_SERVICE).setVisibility(Notification.VISIBILITY_PRIVATE)
            .addAction(Notification.Action.Builder(null, "Stop sync", stop).build()).build()
        if (Build.VERSION.SDK_INT >= 29) startForeground(NOTIFICATION, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
        else startForeground(NOTIFICATION, notification)
        foreground = true
    }
    fun refresh() { if (wanted) link.refreshMetadata();guidanceSourceChanged();observer?.invoke(snapshot()) }
    fun enableGuidance(){
        if(guidanceWanted){refresh();return}
        check(wanted&&link.snapshot()["state"]=="connected"){"Connect the cluster before enabling guidance"}
        check(getSharedPreferences("maps-guidance",MODE_PRIVATE).getBoolean("consent",false)){"Review notification access first"}
        check(MapsNotifications.granted(this)){"Grant notification access first"}
        check(MapsNotifications.bound){"Notification listener is starting; wait and retry Enable"}
        guidanceWanted=true;guidanceStopReason=1;hadListener=true;link.retryGuidance()
        MapsNotifications.setCapture(true);MapsNotifications.refresh();guidanceSourceChanged()
        showNotification("Connected");observer?.invoke(snapshot())
    }
    fun retryGuidance(){check(guidanceWanted){"Enable guidance first"};link.retryGuidance();guidanceSourceChanged()}
    fun stopGuidance(reason: Int=1){
        guidanceWanted=false;guidanceStopReason=reason;hadListener=false
        MapsNotifications.setCapture(false);link.updateGuidance(null,false,reason)
        if(reason!=5&&wanted&&foreground&&permitted())showNotification(if(link.snapshot()["state"]=="connected")"Connected" else "Reconnecting")
        observer?.invoke(snapshot())
    }
    private fun guidanceSnapshot(): Map<String,Any?> {
        refreshMapsInfo()
        return MapsNotifications.snapshot(this)+mapOf("enabled" to guidanceWanted,"stopReason" to guidanceStopReason,
            "consent" to getSharedPreferences("maps-guidance",MODE_PRIVATE).getBoolean("consent",false),
            "transport" to link.guidance.snapshot(SystemClock.elapsedRealtime()),"mapsInstalled" to mapsAvailable,
            "mapsVersion" to mapsVersionValue,"androidApi" to Build.VERSION.SDK_INT)
    }
    @Suppress("DEPRECATION")
    private fun refreshMapsInfo(){
        val now=SystemClock.elapsedRealtime();if(now-mapsInfoAt<30000)return
        mapsInfoAt=now
        mapsAvailable=try {packageManager.getLaunchIntentForPackage(MapsNotifications.PACKAGE)!=null}catch(_: Exception){false}
        mapsVersionValue=try {packageManager.getPackageInfo(MapsNotifications.PACKAGE,0).versionName?.take(80)}catch(_: Exception){null}
    }
    private fun guidanceSourceChanged(publish: Boolean=true){
        if(destroyed)return
        if(guidanceWanted&&(!MapsNotifications.granted(this)||(hadListener&&!MapsNotifications.bound))){
            stopGuidance(if(!MapsNotifications.granted(this))3 else 6);return
        }
        if(guidanceWanted&&MapsNotifications.bound)hadListener=true
        val selected=if(guidanceWanted&&MapsNotifications.bound&&!MapsNotifications.replacementPending())MapsNotifications.observation else null
        link.updateGuidance(selected,guidanceWanted,if(guidanceWanted)2 else guidanceStopReason)
        if(publish)observer?.invoke(snapshot())
    }
    fun stopSync(error: String? = null) {
        stopGuidance(5)
        wanted = false; serviceError = error
        handler.removeCallbacks(retry); retryScheduled = false; retryAttempt = 0
        prefs.edit().putBoolean("stopped", true).apply()
        link.disconnect()
        if (foreground) {
            if (Build.VERSION.SDK_INT >= 24) stopForeground(STOP_FOREGROUND_REMOVE) else stopForeground(true)
        }
        foreground = false; noticeState = ""
        observer?.invoke(snapshot())
        stopSelf()
    }
    fun forget() { stopSync(); link.forgetSelection(); prefs.edit().putBoolean("stopped", true).apply(); observer?.invoke(snapshot()) }
    override fun onDestroy() {
        destroyed = true; wanted = false;guidanceWanted=false;observer = null
        if(MapsNotifications.observer===sourceObserver)MapsNotifications.observer=null
        MapsNotifications.setCapture(false)
        handler.removeCallbacksAndMessages(null)
        unregisterReceiver(radio)
        link.dispose()
        super.onDestroy()
    }
}
