package dev.r15cluster.r15_companion

import android.app.Activity
import android.bluetooth.*
import android.content.*
import android.os.*
import java.util.UUID

/** Sole owner of the physical cluster GATT, including after OS bonding. Main-thread confined. */
@Suppress("DEPRECATION")
class BleBond(private val activity: Activity, private val changed: (Map<String, Any?>) -> Unit) {
    private val handler = Handler(Looper.getMainLooper())
    private val prefs = activity.getSharedPreferences("ble-selection", Context.MODE_PRIVATE)
    private val adapter get() = (activity.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter
    private var gatt: BluetoothGatt? = null
    private var target: BluetoothDevice? = null
    private var registered = false
    private var connected = false
    private var ready = false
    private var bondRequested = false
    private var metadataDone = false
    private var retryAllowed = true
    private var observedBonding = false
    private var completion: ((String?) -> Unit)? = null
    private var state = "disconnected"
    private var failure: String? = null
    private var nameStatus = "notSent"
    private val timeout = Runnable { fail("Connection timed out. Open Settings → Phone → Pair phone for a new bond; disconnect other BLE apps and retry.") }
    private val metadataTimeout = Runnable { guarded { metadataFinished() } }
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val device = intent.getParcelableExtra<BluetoothDevice>(BluetoothDevice.EXTRA_DEVICE) ?: return
            if (intent.action != BluetoothDevice.ACTION_BOND_STATE_CHANGED || device.address != target?.address) return
            guarded {
                // Inspect OS truth, never trust the broadcast's claimed transition.
                android.util.Log.i("R15BleLink", "bond state=${target?.bondState}")
                when (target?.bondState) {
                    BluetoothDevice.BOND_BONDED -> succeedIfReady()
                    BluetoothDevice.BOND_BONDING -> { observedBonding = true }
                    BluetoothDevice.BOND_NONE -> if (observedBonding) fail("Pairing rejected or bond unavailable. Open Pair phone on the cluster. If the cluster was forgotten, remove the old Android bond in Bluetooth settings first.")
                }
            }
        }
    }
    fun snapshot(): Map<String, Any?> {
        val address = prefs.getString("address", null)
        val bonded = try { address != null && adapter?.getRemoteDevice(address)?.bondState == BluetoothDevice.BOND_BONDED } catch (_: Exception) { false }
        return mapOf("state" to state, "address" to address, "name" to prefs.getString("name", null), "bonded" to bonded,
            "error" to failure, "nameStatus" to nameStatus, "retryAllowed" to retryAllowed)
    }
    private fun publish() { changed(snapshot()) }
    fun connect(address: String, name: String, saved: Boolean, complete: (String?) -> Unit) {
        if (completion != null) { complete("A connection is already in progress."); return }
        close()
        completion = complete
        retryAllowed = true; failure = null; state = "connecting"; nameStatus = "notSent"
        guarded {
            val device = adapter.getRemoteDevice(address)
            if (saved && (prefs.getString("address", null) != address || device.bondState != BluetoothDevice.BOND_BONDED)) {
                retryAllowed = false
                fail("The saved Android bond is unavailable. Forget the app selection and pair explicitly again."); return@guarded
            }
            target = device
            // A selection is only persisted after successful bonding on a live link.
            val filter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
            if (Build.VERSION.SDK_INT >= 33) activity.registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED)
            else activity.registerReceiver(receiver, filter)
            registered = true
            handler.postDelayed(timeout, 45000)
            gatt = device.connectGatt(activity, false, object : BluetoothGattCallback() {
                override fun onConnectionStateChange(connection: BluetoothGatt, status: Int, newState: Int) {
                    handler.post {
                        if (gatt !== connection) return@post
                        guarded {
                            android.util.Log.i("R15BleLink", "gatt status=$status state=$newState")
                            if (status != BluetoothGatt.GATT_SUCCESS || newState == BluetoothProfile.STATE_DISCONNECTED) {
                                retryAllowed = status != 19 && status != 5 && status != 137
                                fail("Bluetooth link closed (GATT status $status). Check cluster power, Pair phone for new bonds, or stale bonds after cluster Forget.")
                            } else if (newState == BluetoothProfile.STATE_CONNECTED) {
                                connected = true
                                if (device.bondState == BluetoothDevice.BOND_BONDED) {
                                    metadataDone = true; succeedIfReady(name, address)
                                } else {
                                    // Optional label must never block an OS-initiated security procedure.
                                    handler.postDelayed(metadataTimeout, 2500)
                                    if (!connection.discoverServices()) metadataFinished()
                                }
                            }
                        }
                    }
                }
                override fun onServicesDiscovered(connection: BluetoothGatt, status: Int) {
                    handler.post {
                        if (gatt !== connection || metadataDone) return@post
                        guarded {
                            val characteristic = connection.getService(UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c010"))
                                ?.getCharacteristic(UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c011"))
                            if (status != BluetoothGatt.GATT_SUCCESS || characteristic == null || device.bondState != BluetoothDevice.BOND_NONE) {
                                nameStatus = "unavailable"; metadataFinished(); return@guarded
                            }
                            val bytes = phoneLabel().toByteArray(Charsets.US_ASCII)
                            val started = if (Build.VERSION.SDK_INT >= 33) {
                                connection.writeCharacteristic(characteristic, bytes, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
                            } else {
                                characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                                characteristic.value = bytes
                                connection.writeCharacteristic(characteristic)
                            }
                            if (!started) { nameStatus = "unavailable"; metadataFinished() }
                        }
                    }
                }
                override fun onCharacteristicWrite(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
                    handler.post {
                        if (gatt !== connection || metadataDone) return@post
                        android.util.Log.i("R15BleLink", "label write status=$status")
                        nameStatus = if (status == BluetoothGatt.GATT_SUCCESS) "sent" else "refused:$status"
                        guarded { metadataFinished() }
                    }
                }
            }, BluetoothDevice.TRANSPORT_LE)
            // Capture selected label without using remote Device Name as this phone's name.
            selectedName = name.take(248); selectedAddress = address
            if (gatt == null) fail("Android could not open the BLE link.") else publish()
        }
    }
    private var selectedName = "R15 Cluster"
    private var selectedAddress = ""
    private fun phoneLabel(): String {
        fun supported(value: String?) = value?.trim()?.takeIf { it.isNotEmpty() && it.all { c -> c.code in 32..126 } }?.take(32)
        val localName = try { adapter.name } catch (_: SecurityException) { null }
        return supported(localName) ?: supported("${Build.MANUFACTURER} ${Build.MODEL}") ?: "Android phone"
    }
    private fun metadataFinished() {
        if (metadataDone || gatt == null) return
        metadataDone = true; handler.removeCallbacks(metadataTimeout)
        when (target?.bondState) {
            BluetoothDevice.BOND_BONDED -> succeedIfReady()
            BluetoothDevice.BOND_BONDING -> { observedBonding = true; state = "pairing"; publish() }
            BluetoothDevice.BOND_NONE -> {
                state = "pairing"; publish()
                if (!bondRequested) {
                    bondRequested = true
                    if (target?.createBond() != true && target?.bondState == BluetoothDevice.BOND_NONE) fail("Android could not start pairing. Open Pair phone on the cluster and retry.")
                }
            }
        }
    }
    private fun succeedIfReady(name: String = selectedName, address: String = selectedAddress) {
        if (!connected || ready || target?.bondState != BluetoothDevice.BOND_BONDED) return
        ready = true; state = "connected"; failure = null
        prefs.edit().putString("address", address).putString("name", name.take(248)).apply()
        clearAttempt()
        publish()
        val callback = completion; completion = null; callback?.invoke(null)
    }
    private fun guarded(block: () -> Unit) {
        try { block() } catch (_: SecurityException) { fail("Bluetooth permission was revoked. Restore Nearby devices access and retry.") }
        catch (_: Exception) { fail("Bluetooth is unavailable. Check the radio and permissions, then retry.") }
    }
    private fun clearAttempt() {
        handler.removeCallbacks(timeout); handler.removeCallbacks(metadataTimeout)
        if (registered) { try { activity.unregisterReceiver(receiver) } catch (_: IllegalArgumentException) {} ; registered = false }
    }
    private fun close() {
        clearAttempt()
        val old = gatt; gatt = null
        try { old?.disconnect() } catch (_: Exception) {}
        try { old?.close() } catch (_: Exception) {}
        target = null; connected = false; ready = false; bondRequested = false; metadataDone = false; observedBonding = false
    }
    private fun fail(message: String) {
        close(); state = "disconnected"; failure = message; publish()
        val callback = completion; completion = null; callback?.invoke(message)
    }
    fun disconnect() {
        close(); state = "disconnected"; failure = null; publish()
        val callback = completion; completion = null; callback?.invoke("Connection cancelled. Android bonds are retained.")
    }
    fun forgetSelection() { disconnect(); prefs.edit().clear().apply(); publish() }
    fun dispose() { disconnect() }
}
