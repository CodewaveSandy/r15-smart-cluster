package dev.r15cluster.r15_companion

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.*
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.AtomicFile
import java.util.concurrent.Executors
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.EventChannel
import java.io.File
import java.security.KeyStore
import java.util.UUID
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

class MainActivity : FlutterActivity() {
    private val serviceId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c001")
    private val metadataId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c002")
    private val handler = Handler(Looper.getMainLooper())
    private var pending: MethodChannel.Result? = null
    private var scan: ScanCallback? = null
    private var gatt: BluetoothGatt? = null
    private val storage = Executors.newSingleThreadExecutor()
    private var interactive = true
    private var pendingAddress: String? = null
    private var accessOnly = false
    private var linkEvents: EventChannel.EventSink? = null
    private lateinit var bonding: BleBond
    private val found = linkedMapOf<String, Map<String, Any>>()
    private val recentlySeen = linkedMapOf<String, Long>()
    private val adapter get() = (getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        bonding = BleBond(this) { linkEvents?.success(it) }
        EventChannel(flutterEngine.dartExecutor.binaryMessenger, "dev.r15cluster/ble-link").setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, events: EventChannel.EventSink) { linkEvents = events; events.success(bonding.snapshot()) }
            override fun onCancel(arguments: Any?) { linkEvents = null }
        })
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, "dev.r15cluster/nearby").setMethodCallHandler { call, result ->
            try {
                when (call.method) {
                    "stopScan" -> {
                        if (scan != null) finish(found.values.toList())
                        result.success(null)
                    }
                    "linkStatus" -> result.success(bonding.snapshot())
                    "disconnectLink" -> { bonding.disconnect(); result.success(null) }
                    "forgetSelection" -> { bonding.forgetSelection(); result.success(null) }
                    "bond" -> {
                        if (pending != null) { result.error("busy", "A Bluetooth operation is already running.", null); return@setMethodCallHandler }
                        val address = call.argument<String>("address") ?: error("Missing device")
                        val saved = call.argument<Boolean>("saved") == true
                        val name = call.argument<String>("name") ?: "R15 Cluster"
                        val seenAt = recentlySeen[address]
                        if (!saved && (seenAt == null || SystemClock.elapsedRealtime() - seenAt > 20000 || !name.startsWith("R15 Cluster"))) {
                            result.error("device", "Search again before selecting this device.", null); return@setMethodCallHandler
                        }
                        if (accessStatus()["enabled"] != true) { result.error("access", "Restore Bluetooth and Nearby devices access first.", null); return@setMethodCallHandler }
                        pending = result; accessOnly = false
                        bonding.connect(address, name, saved) { error ->
                            if (error == null) finish(bonding.snapshot()) else finishError("bond", error)
                        }
                    }
                    "access" -> result.success(accessStatus())
                    "requestAccess" -> {
                        if (pending != null) { result.error("busy", "A Bluetooth operation is already running.", null); return@setMethodCallHandler }
                        pending = result; accessOnly = true; interactive = true
                        val missing = requiredPermissions().filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
                        if (missing.isNotEmpty()) {
                            getSharedPreferences("ble-access", Context.MODE_PRIVATE).edit().putBoolean("asked", true).apply()
                            requestPermissions(missing.toTypedArray(), 101)
                        } else finishAccess()
                    }
                    "accessSettings" -> {
                        startActivity(Intent(android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS, android.net.Uri.parse("package:$packageName")))
                        result.success(null)
                    }
                    "locationSettings" -> { startActivity(Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS)); result.success(null) }
                    "loadTrust" -> storage.execute { try { val value=loadTrust();handler.post { result.success(value) } } catch (_: Exception) { handler.post { result.error("storage", "Saved pairing could not be read.", null) } } }
                    "saveTrust" -> { val value=call.argument<String>("value") ?: error("Missing trust");storage.execute { try { saveTrust(value);handler.post { result.success(null) } } catch (_: Exception) { handler.post { result.error("storage", "Pairing could not be saved.", null) } } } }
                    "clearTrust" -> storage.execute { try { AtomicFile(File(noBackupFilesDir, "cluster-trust")).delete();handler.post { result.success(null) } } catch (_: Exception) { handler.post { result.error("storage", "Pairing could not be removed.", null) } } }
                    "scan" -> {
                        if (pending != null) { result.error("busy", "Discovery is already running.", null); return@setMethodCallHandler }
                        pending = result
                        accessOnly = false
                        interactive=call.argument<Boolean>("interactive") ?: true
                        val permissions = requiredPermissions()
                        val missing = permissions.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
                        if (missing.isNotEmpty()) { if(interactive) requestPermissions(missing.toTypedArray(), 101) else finishError("permission", "Nearby devices access is needed for rediscovery.") } else startScan()
                    }
                    "read" -> {
                        if (pending != null) { result.error("busy", "Discovery is already running.", null); return@setMethodCallHandler }
                        pending = result
                        accessOnly = false
                        interactive=call.argument<Boolean>("interactive") ?: true
                        val address=call.argument<String>("address") ?: error("Missing device")
                        val wifi=applicationContext.getSystemService(Context.WIFI_SERVICE) as android.net.wifi.WifiManager
                        if (!wifi.isWifiEnabled) {
                            if(!interactive) { finishError("wifi", "Wi-Fi is disabled.");return@setMethodCallHandler }
                            pendingAddress=address
                            startActivityForResult(Intent(if(Build.VERSION.SDK_INT>=29) android.provider.Settings.Panel.ACTION_WIFI else android.provider.Settings.ACTION_WIFI_SETTINGS),103)
                        } else readDevice(address)
                    }
                    else -> result.notImplemented()
                }
            } catch (error: Exception) { if (pending === result) finishError("unavailable", "Nearby connection is unavailable.") else result.error("storage", "Protected pairing storage is unavailable.", null) }
        }
    }
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 101) {
            if (accessOnly) { finishAccess(); return }
            if (grantResults.isNotEmpty() && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) startScan()
            else finishError("permission", "Allow Nearby devices access in app settings to find your cluster.")
        }
    }
    private fun requiredPermissions(): Array<String> = if (Build.VERSION.SDK_INT >= 31)
        arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)

    private fun accessStatus(): Map<String, Any> {
        val supported = packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE) && adapter != null
        val missing = requiredPermissions().filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        val asked = getSharedPreferences("ble-access", Context.MODE_PRIVATE).getBoolean("asked", false)
        val location = getSystemService(Context.LOCATION_SERVICE) as android.location.LocationManager
        val locationEnabled = Build.VERSION.SDK_INT >= 31 || if (Build.VERSION.SDK_INT >= 28) location.isLocationEnabled else
            location.isProviderEnabled(android.location.LocationManager.GPS_PROVIDER) || location.isProviderEnabled(android.location.LocationManager.NETWORK_PROVIDER)
        return mapOf("supported" to supported, "granted" to missing.isEmpty(),
            "enabled" to (supported && missing.isEmpty() && adapter.isEnabled),
            "blocked" to (asked && missing.any { !shouldShowRequestPermissionRationale(it) }),
            "locationEnabled" to locationEnabled, "legacyLocation" to (Build.VERSION.SDK_INT < 31))
    }
    @Suppress("DEPRECATION")
    private fun finishAccess() {
        val status = accessStatus()
        if (status["supported"] == true && status["granted"] == true && status["enabled"] == false) {
            startActivityForResult(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), 102)
        } else { accessOnly = false; finish(status) }
    }
    @Suppress("DEPRECATION")
    private fun startScan() {
        if (adapter == null) { finishError("unsupported", "This phone does not support Bluetooth."); return }
        if (!adapter.isEnabled) { if(!interactive){finishError("bluetooth", "Bluetooth is disabled.");return};startActivityForResult(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), 102); return }
        if (accessStatus()["locationEnabled"] != true) { finishError("location", "Enable Location services for BLE scanning on this Android version."); return }
        found.clear()
        val callback = object : ScanCallback() {
            override fun onScanResult(type: Int, result: ScanResult) {
                handler.post {
                    if (scan !== this) return@post
                    val previous = found[result.device.address]
                    val name = result.scanRecord?.deviceName?.take(248) ?: previous?.get("name") as? String ?: "Unnamed BLE device"
                    if (!name.startsWith("R15 Cluster")) return@post
                    val seenNow = SystemClock.elapsedRealtime()
                    recentlySeen[result.device.address] = seenNow
                    recentlySeen.entries.removeAll { seenNow - it.value > 20000 }
                    while (recentlySeen.size > 64) recentlySeen.remove(recentlySeen.keys.first())
                    val preview = result.scanRecord?.serviceUuids?.contains(ParcelUuid(serviceId)) == true || previous?.get("preview") == true
                    val candidate = name.startsWith("R15 Cluster")
                    val entry = mapOf<String, Any>("address" to result.device.address, "name" to name,
                        "preview" to preview, "candidate" to candidate, "rssi" to result.rssi)
                    if (found.containsKey(result.device.address) || found.size < 32) found[result.device.address] = entry
                    else if (candidate) {
                        val replace = found.entries.firstOrNull { it.value["candidate"] != true }
                        if (replace != null) { found.remove(replace.key); found[result.device.address] = entry }
                    }
                }
            }
            override fun onScanFailed(code: Int) { handler.post { if (scan === this) finishError("scan", "Bluetooth LE search failed. Wait a moment and try again.") } }
        }
        scan = callback
        val scanner = adapter.bluetoothLeScanner
        if (scanner == null) { finishError("bluetooth", "Bluetooth LE scanning is unavailable."); return }
        scanner.startScan(emptyList<ScanFilter>(), ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), callback)
        handler.postDelayed({ if (scan === callback) finish(found.values.sortedWith(compareByDescending<Map<String, Any>> { it["candidate"] == true }.thenByDescending { it["rssi"] as Int })) }, 5000)
    }
    @Suppress("DEPRECATION")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == 103 && pending != null) {
            val address=pendingAddress;pendingAddress=null
            val wifi=applicationContext.getSystemService(Context.WIFI_SERVICE) as android.net.wifi.WifiManager
            if(wifi.isWifiEnabled && address!=null) readDevice(address) else finishError("wifi", "Turn on Wi-Fi to connect to the Mac preview.")
        }
        if (requestCode == 102 && pending != null) {
            if (accessOnly) { accessOnly = false; finish(accessStatus()); return }
            if (adapter.isEnabled) startScan() else finishError("bluetooth", "Turn on Bluetooth to find your cluster.")
        }
    }
    @Suppress("DEPRECATION")
    private fun readDevice(address: String) {
        val callback = object : BluetoothGattCallback() {
            override fun onConnectionStateChange(connection: BluetoothGatt, status: Int, newState: Int) {
                if (status != BluetoothGatt.GATT_SUCCESS) { handler.post { finishError("connect", "Could not read the nearby cluster.") }; return }
                if (newState == BluetoothProfile.STATE_CONNECTED) connection.discoverServices()
            }
            override fun onServicesDiscovered(connection: BluetoothGatt, status: Int) {
                val characteristic = connection.getService(serviceId)?.getCharacteristic(metadataId)
                if (status != BluetoothGatt.GATT_SUCCESS || characteristic == null || !connection.readCharacteristic(characteristic)) handler.post { finishError("service", "Cluster discovery service is unavailable.") }
            }
            override fun onCharacteristicRead(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) { readComplete(connection, characteristic.value, status) }
            override fun onCharacteristicRead(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray, status: Int) { readComplete(connection, value, status) }
            private fun readComplete(connection: BluetoothGatt, value: ByteArray?, status: Int) {
                handler.post {
                    if (gatt !== connection) return@post
                    if (status == BluetoothGatt.GATT_SUCCESS && value != null && value.size <= 512) finish(String(value, Charsets.UTF_8)) else finishError("service", "Invalid cluster discovery data.")
                }
            }
        }
        gatt = adapter.getRemoteDevice(address).connectGatt(this, false, callback, BluetoothDevice.TRANSPORT_LE)
        val current = gatt
        if(current==null){finishError("connect", "Nearby connection could not start.");return}
        handler.postDelayed({ if (gatt === current && pending != null) finishError("timeout", "Nearby cluster did not respond.") }, 8000)
    }
    @Suppress("DEPRECATION")
    private fun cleanup() {
        scan?.let { try { adapter.bluetoothLeScanner?.stopScan(it) } catch (_: Exception) {} }
        scan = null
        gatt?.close(); gatt = null
    }
    private fun finish(value: Any?) { val result = pending; pending = null; cleanup(); result?.success(value) }
    private fun finishError(code: String, message: String) { val result = pending; pending = null; cleanup(); result?.error(code, message, null) }
    private fun key(): SecretKey {
        val store = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
        (store.getKey("r15-pairing", null) as? SecretKey)?.let { return it }
        return KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore").apply {
            init(KeyGenParameterSpec.Builder("r15-pairing", KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT).setBlockModes(KeyProperties.BLOCK_MODE_GCM).setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE).build())
        }.generateKey()
    }
    private fun saveTrust(value: String) {
        require(value.toByteArray().size <= 8192)
        val cipher = Cipher.getInstance("AES/GCM/NoPadding").apply { init(Cipher.ENCRYPT_MODE, key()) }
        val file=AtomicFile(File(noBackupFilesDir,"cluster-trust"))
        val output=file.startWrite()
        try { output.write(cipher.iv + cipher.doFinal(value.toByteArray(Charsets.UTF_8)));file.finishWrite(output) }
        catch(error: Exception) { file.failWrite(output);throw error }
    }
    private fun loadTrust(): String? {
        val file = File(noBackupFilesDir, "cluster-trust")
        if (!file.exists()) return null
        val bytes = AtomicFile(file).readFully(); require(bytes.size in 29..8300)
        val cipher = Cipher.getInstance("AES/GCM/NoPadding").apply { init(Cipher.DECRYPT_MODE, key(), GCMParameterSpec(128, bytes.copyOfRange(0, 12))) }
        return String(cipher.doFinal(bytes.copyOfRange(12, bytes.size)), Charsets.UTF_8)
    }
    override fun onDestroy() { bonding.dispose(); linkEvents = null; finishError("cancelled", "Discovery stopped."); storage.shutdown(); super.onDestroy() }
}
