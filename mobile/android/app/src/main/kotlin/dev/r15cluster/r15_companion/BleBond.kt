package dev.r15cluster.r15_companion

import android.bluetooth.*
import android.content.*
import android.os.*
import java.util.UUID

/** Sole owner of the physical GATT. All operations/callbacks are serialized on main. */
@Suppress("DEPRECATION")
class BleBond(private val context: Context, private val changed: (Map<String, Any?>) -> Unit) {
    private enum class Operation { NONE, DISCOVERY, ENROLLMENT, MTU, METADATA, CAPABILITIES, SUBSCRIBE, BASELINE, HEALTH }
    private val serviceId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c010")
    private val nameId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c011")
    private val metadataId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c012")
    private val handler = Handler(Looper.getMainLooper())
    private val prefs = context.getSharedPreferences("ble-selection", Context.MODE_PRIVATE)
    private val adapter get() = (context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter
    private val phone = PhoneMetadataSource(context.applicationContext) { adapter }
    private val capabilitiesId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c013")
    private val telemetryId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c014")
    private val healthId = UUID.fromString("813b1240-8fd9-4d67-a53b-9f0a2531c015")
    private val cccdId = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    private var capabilitiesCharacteristic: BluetoothGattCharacteristic? = null
    private var telemetryCharacteristic: BluetoothGattCharacteristic? = null
    private var healthCharacteristic: BluetoothGattCharacteristic? = null
    private var telemetry: TelemetrySession? = null
    private var telemetryState = "notConnected"
    private var capabilityAttempted = false
    private var capabilityRetries = 0
    private var capabilityRetryAt = 0L
    private var subscribed = false
    private var subscribeAttempted = false
    private var baselineDue = false
    private var healthDue = false
    private var diagnostics = false
    private var readDispatch = 0L
    private var rediscoverPending = false
    private data class Incoming(val connection: BluetoothGatt, val characteristic: BluetoothGattCharacteristic, val bytes: ByteArray, val receivedAt: Long)
    private val notificationLock = Any()
    private var pendingNotification: Incoming? = null
    private var notificationPosted = false
    private val deliverNotification = Runnable {
        val incoming = synchronized(notificationLock) {
            val latest = pendingNotification
            pendingNotification = null; notificationPosted = false
            latest
        }
        if (incoming != null && gatt === incoming.connection && !blocked &&
            incoming.characteristic === telemetryCharacteristic && subscribeAttempted) {
            telemetry?.accept(incoming.bytes, incoming.receivedAt)
            if (telemetry?.sessionFault == true) fail("Telemetry session changed. Reconnecting.")
            else { handler.removeCallbacks(telemetryTick); handler.post(telemetryTick) }
        }
    }
    private val resumePump = Runnable { optional { pumpMetadata() } }
    private val telemetryTick = object : Runnable {
        override fun run() {
            if (!ready || telemetry == null) return
            publish()
            handler.postDelayed(this, telemetry?.nextCheckDelay(SystemClock.elapsedRealtime()) ?: 200)
        }
    }
    private val baselineRefresh = object : Runnable {
        override fun run() {
            if (!ready || !subscribed) return
            baselineDue = true
            optional { pumpMetadata() }
            handler.postDelayed(this, 25000)
        }
    }
    private val healthRefresh = object : Runnable {
        override fun run() {
            if (!ready || !subscribed || !diagnostics) return
            healthDue = true
            optional { pumpMetadata() }
            val value = telemetry?.snapshot(SystemClock.elapsedRealtime())
            android.util.Log.i("R15BleLink", "telemetry ageMs=${value?.get("sourceAgeBoundMs")} receiptMs=${value?.get("receiptAgeMs")} maxGapMs=${value?.get("maxReceiptGapMs")} rejected=${value?.get("rejected")} rttMs=${value?.get("baselineRttMs")} uncertaintyMs=${value?.get("clockUncertaintyMs")}")
            handler.postDelayed(this, 10000)
        }
    }
    fun setDiagnostics(enabled: Boolean) { diagnostics = enabled }
    private var gatt: BluetoothGatt? = null
    private var target: BluetoothDevice? = null
    private var registered = false
    private var connected = false
    private var ready = false
    private var bondRequested = false
    private var enrollmentDone = false
    private var observedBonding = false
    private var retryAllowed = true
    private var completion: ((String?) -> Unit)? = null
    private var selectedName = "R15 Cluster"
    private var selectedAddress = ""
    private var state = "disconnected"
    private var failure: String? = null
    private var nameStatus = "notSent"
    private var operation = Operation.NONE
    private var blocked = false
    private var servicesKnown = false
    private var metadataCharacteristic: BluetoothGattCharacteristic? = null
    private var mtuAttempted = false
    private var mtu = 23
    private var metadataActive = false
    private var metadataPending = false
    private var metadataRetries = 0
    private var metadataNotBefore = 0L
    private var lastAttempt = -1100L
    private var lastAccepted: Long? = null
    private var metadataState = "notConnected"
    private val timeout = Runnable { fail("Connection timed out. Open Settings → Phone → Pair phone for a new bond; disconnect other BLE apps and retry.") }
    private val enrollmentTimeout = Runnable { guarded { enrollmentFinished() } }
    private val operationTimeout = Runnable {
        // Never launch another GATT operation while an earlier callback is missing.
        blocked = true
        android.util.Log.i("R15BleLink", "operation timeout=${operation.ordinal}")
        fail("Bluetooth operation timed out. Reconnecting through the single connection owner.")
    }
    private val sendMetadata = Runnable { optional { pumpMetadata() } }
    private val periodicMetadata = object : Runnable {
        override fun run() {
            if (!metadataActive) return
            queueMetadata()
            handler.postDelayed(this, 30000)
        }
    }
    private val metadataExpiry = Runnable {
        if (lastAccepted?.let { SystemClock.elapsedRealtime() - it >= 90000 } == true) {
            setMetadataState("unavailable")
        }
    }
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val device = intent.getParcelableExtra<BluetoothDevice>(BluetoothDevice.EXTRA_DEVICE) ?: return
            if (intent.action != BluetoothDevice.ACTION_BOND_STATE_CHANGED || device.address != target?.address) return
            guarded {
                val bond = target?.bondState
                android.util.Log.i("R15BleLink", "bond state=$bond")
                when (bond) {
                    BluetoothDevice.BOND_BONDED -> succeedIfReady()
                    BluetoothDevice.BOND_BONDING -> observedBonding = true
                    BluetoothDevice.BOND_NONE -> if (observedBonding) fail("Pairing rejected or bond unavailable. Open Pair phone on the cluster. If the cluster was forgotten, remove the old Android bond in Bluetooth settings first.")
                }
            }
        }
    }

    fun snapshot(): Map<String, Any?> {
        val address = prefs.getString("address", null)
        val bonded = try { address != null && adapter?.getRemoteDevice(address)?.bondState == BluetoothDevice.BOND_BONDED } catch (_: Exception) { false }
        return mapOf("state" to state, "address" to address, "name" to prefs.getString("name", null),
            "bonded" to bonded, "error" to failure, "nameStatus" to nameStatus,
            "retryAllowed" to retryAllowed, "metadataState" to (if (metadataState == "synced" &&
                lastAccepted?.let { SystemClock.elapsedRealtime() - it >= 90000 } == true) "unavailable" else metadataState),
            "telemetry" to (telemetry?.snapshot(SystemClock.elapsedRealtime()) ?: mapOf("state" to telemetryState)))
    }
    private fun publish() { changed(snapshot()) }
    private fun setMetadataState(value: String) {
        if (metadataState != value) { metadataState = value; publish() }
    }

    fun connect(address: String, name: String, saved: Boolean, complete: (String?) -> Unit) {
        if (completion != null) { complete("A connection is already in progress."); return }
        close()
        completion = complete
        selectedName = name.take(248); selectedAddress = address
        retryAllowed = true; failure = null; state = "connecting"; nameStatus = "notSent"
        guarded {
            val device = adapter?.getRemoteDevice(address) ?: error("Bluetooth unavailable")
            if (saved && (prefs.getString("address", null) != address || device.bondState != BluetoothDevice.BOND_BONDED)) {
                retryAllowed = false
                fail("The saved Android bond is unavailable. Forget the app selection and pair explicitly again."); return@guarded
            }
            target = device
            val filter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
            if (Build.VERSION.SDK_INT >= 33) context.registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED)
            else context.registerReceiver(receiver, filter)
            registered = true
            handler.postDelayed(timeout, 45000)
            gatt = device.connectGatt(context, false, callbacks, BluetoothDevice.TRANSPORT_LE)
            if (gatt == null) fail("Android could not open the BLE link.") else publish()
        }
    }

    private val callbacks = object : BluetoothGattCallback() {
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
                        if (target?.bondState == BluetoothDevice.BOND_BONDED) {
                            enrollmentDone = true
                            succeedIfReady()
                        } else {
                            // Protect the three-second enrollment-label window. MTU waits until later.
                            handler.postDelayed(enrollmentTimeout, 2500)
                            discover()
                        }
                    }
                }
            }
        }
        override fun onServicesDiscovered(connection: BluetoothGatt, status: Int) {
            handler.post {
                if (gatt !== connection || blocked || operation != Operation.DISCOVERY) return@post
                endOperation()
                servicesKnown = true
                optional {
                    val service = if (status == BluetoothGatt.GATT_SUCCESS) connection.getService(serviceId) else null
                    metadataCharacteristic = service?.getCharacteristic(metadataId)?.takeIf {
                        it.properties and BluetoothGattCharacteristic.PROPERTY_WRITE != 0
                    }
                    capabilitiesCharacteristic = service?.getCharacteristic(capabilitiesId)?.takeIf { it.properties and BluetoothGattCharacteristic.PROPERTY_READ != 0 }
                    telemetryCharacteristic = service?.getCharacteristic(telemetryId)?.takeIf {
                        it.properties and BluetoothGattCharacteristic.PROPERTY_READ != 0 && it.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY != 0
                    }
                    healthCharacteristic = service?.getCharacteristic(healthId)?.takeIf { it.properties and BluetoothGattCharacteristic.PROPERTY_READ != 0 }
                    val enrollment = service?.getCharacteristic(nameId)
                    if (!enrollmentDone && target?.bondState == BluetoothDevice.BOND_NONE && enrollment != null) {
                        beginOperation(Operation.ENROLLMENT)
                        if (!write(connection, enrollment, phone.label().toByteArray(Charsets.US_ASCII))) {
                            endOperation(); nameStatus = "unavailable"; enrollmentFinished()
                        }
                    } else {
                        if (!enrollmentDone) { nameStatus = "unavailable"; enrollmentFinished() }
                    }
                    pumpMetadata()
                }
            }
        }
        override fun onMtuChanged(connection: BluetoothGatt, negotiatedMtu: Int, status: Int) {
            handler.post {
                if (gatt !== connection || blocked || operation != Operation.MTU) return@post
                endOperation()
                android.util.Log.i("R15BleLink", "metadata mtu=$negotiatedMtu status=$status")
                if (status == BluetoothGatt.GATT_SUCCESS && negotiatedMtu >= 47) {
                    mtu = negotiatedMtu
                    optional { pumpMetadata() }
                } else {
                    stopMetadata(if (metadataCharacteristic == null) "unsupported" else "unavailable")
                    telemetryState = "unsupported"
                    publish()
                    optional { pumpTelemetry() }
                }
            }
        }
        override fun onCharacteristicWrite(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            handler.post {
                if (gatt !== connection || blocked) return@post
                if (rediscoverPending && operation in listOf(Operation.ENROLLMENT, Operation.METADATA)) {
                    endOperation(); optional { pumpMetadata() }; return@post
                }
                if (characteristic.uuid == nameId && operation == Operation.ENROLLMENT) {
                    endOperation()
                    android.util.Log.i("R15BleLink", "label write status=$status")
                    nameStatus = if (status == BluetoothGatt.GATT_SUCCESS) "sent" else "refused:$status"
                    guarded { enrollmentFinished() }
                    optional { pumpMetadata() }
                } else if (characteristic === metadataCharacteristic && operation == Operation.METADATA) {
                    endOperation()
                    lastAttempt = SystemClock.elapsedRealtime()
                    android.util.Log.i("R15BleLink", "connected metadata status=$status")
                    if (status == BluetoothGatt.GATT_SUCCESS) {
                        lastAccepted = lastAttempt
                        metadataRetries = 0
                        metadataNotBefore = 0
                        handler.removeCallbacks(metadataExpiry)
                        handler.postDelayed(metadataExpiry, 90000)
                        setMetadataState("synced")
                        if (metadataPending) scheduleMetadata(1100)
                    } else metadataFailed()
                    optional { pumpMetadata() }
                }
            }
        }
        override fun onServiceChanged(connection: BluetoothGatt) {
            handler.post {
                if (gatt !== connection || blocked) return@post
                // Finish the current ATT operation before rediscovering. New
                // callbacks/notifications cannot use invalid characteristic refs.
                resetTelemetry()
                metadataCharacteristic = null; capabilitiesCharacteristic = null
                telemetryCharacteristic = null; healthCharacteristic = null
                stopMetadata("syncing")
                servicesKnown = false; rediscoverPending = true
                optional { pumpMetadata() }
            }
        }
        override fun onCharacteristicRead(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray, status: Int) {
            receiveRead(connection, characteristic, value.copyOf(), status)
        }
        override fun onCharacteristicRead(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            if (Build.VERSION.SDK_INT < 33) receiveRead(connection, characteristic, characteristic.value?.copyOf() ?: byteArrayOf(), status)
        }
        override fun onCharacteristicChanged(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            receiveTelemetry(connection, characteristic, value.copyOf())
        }
        override fun onCharacteristicChanged(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < 33) receiveTelemetry(connection, characteristic, characteristic.value?.copyOf() ?: byteArrayOf())
        }
        override fun onDescriptorWrite(connection: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            handler.post {
                if (gatt === connection && !blocked && rediscoverPending && operation == Operation.SUBSCRIBE) {
                    endOperation(); optional { pumpMetadata() }; return@post
                }
                if (gatt !== connection || blocked || operation != Operation.SUBSCRIBE ||
                    descriptor.uuid != cccdId || descriptor.characteristic !== telemetryCharacteristic) return@post
                endOperation()
                android.util.Log.i("R15BleLink", "cccd status=$status")
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    subscribed = true; baselineDue = true; healthDue = true
                    telemetry?.state = "subscribed"
                    handler.postDelayed(baselineRefresh, 25000)
                    if (diagnostics) handler.postDelayed(healthRefresh, 10000)
                } else { telemetryState = "unavailable"; telemetry?.state = "unavailable" }
                publish(); optional { pumpMetadata() }
            }
        }
    }

    private fun receiveTelemetry(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, bytes: ByteArray) {
        val receivedAt = SystemClock.elapsedRealtime()
        synchronized(notificationLock) {
            pendingNotification = Incoming(connection, characteristic, bytes, receivedAt)
            if (!notificationPosted) { notificationPosted = true; handler.post(deliverNotification) }
        }
    }
    private fun receiveRead(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, bytes: ByteArray, status: Int) {
        val receivedAt = SystemClock.elapsedRealtime()
        handler.post {
            if (gatt !== connection || blocked) return@post
            val active = operation
            val expected = when (active) {
                Operation.CAPABILITIES -> capabilitiesCharacteristic
                Operation.BASELINE -> telemetryCharacteristic
                Operation.HEALTH -> healthCharacteristic
                else -> null
            }
            // A service-change invalidates refs, but still consumes its outstanding callback.
            if (rediscoverPending && active in listOf(Operation.CAPABILITIES, Operation.BASELINE, Operation.HEALTH)) {
                endOperation(); optional { pumpMetadata() }; return@post
            }
            if (expected == null || characteristic !== expected) return@post
            endOperation()
            android.util.Log.i("R15BleLink", "read operation=${active.ordinal} status=$status bytes=${bytes.size}")
            when (active) {
                Operation.CAPABILITIES -> {
                    val caps = if (status == BluetoothGatt.GATT_SUCCESS) TelemetryWire.capabilities(bytes) else null
                    if (caps != null) {
                        telemetry = TelemetrySession(caps)
                        telemetryState = "initializing"
                        handler.removeCallbacks(telemetryTick); handler.post(telemetryTick)
                        if (caps.features and 2L == 0L || mtu < 67 || telemetryCharacteristic == null) {
                            telemetryState = "unsupported"; telemetry?.state = "unsupported"
                        }
                    } else if (status != BluetoothGatt.GATT_SUCCESS && capabilityRetries < 3) {
                        capabilityRetryAt = SystemClock.elapsedRealtime() + (1000L shl capabilityRetries++)
                        capabilityAttempted = false
                        handler.postDelayed(resumePump, capabilityRetryAt - SystemClock.elapsedRealtime())
                    } else { telemetryState = "unsupported" }
                }
                Operation.BASELINE -> {
                    if (status == BluetoothGatt.GATT_SUCCESS) telemetry?.accept(bytes, receivedAt, readDispatch)
                    // Failed/slow baselines retain stale readings and retry at a
                    // bounded cadence without competing ATT operations.
                    val timing = telemetry?.snapshot(receivedAt)?.get("timingReady") == true
                    if (!timing) {
                        handler.removeCallbacks(baselineRefresh)
                        handler.postDelayed(baselineRefresh, 2000)
                    }
                }
                Operation.HEALTH -> if (status == BluetoothGatt.GATT_SUCCESS) {
                    if (telemetry?.acceptHealth(bytes) == true && diagnostics) {
                        val health = telemetry?.health?.filterKeys { it !in listOf("boot", "connection", "uptime") }
                        android.util.Log.i("R15BleLink", "health $health")
                    }
                }
                else -> Unit
            }
            if (telemetry?.sessionFault == true) { fail("Telemetry session changed. Reconnecting."); return@post }
            publish()
            if (telemetry != null) { handler.removeCallbacks(telemetryTick); handler.post(telemetryTick) }
            optional { pumpMetadata() }
        }
    }

    private fun beginOperation(next: Operation) {
        check(operation == Operation.NONE && !blocked)
        operation = next
        handler.postDelayed(operationTimeout, 8000)
    }
    private fun endOperation() { handler.removeCallbacks(operationTimeout); operation = Operation.NONE }
    private fun discover() {
        if (operation != Operation.NONE || blocked) return
        val current = gatt ?: return
        beginOperation(Operation.DISCOVERY)
        if (!current.discoverServices()) {
            endOperation(); servicesKnown = true; metadataCharacteristic = null
            if (!enrollmentDone) enrollmentFinished()
            setMetadataState("unavailable")
        }
    }
    private fun write(current: BluetoothGatt, characteristic: BluetoothGattCharacteristic, bytes: ByteArray): Boolean {
        return if (Build.VERSION.SDK_INT >= 33) {
            current.writeCharacteristic(characteristic, bytes, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
        } else {
            characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            characteristic.value = bytes
            current.writeCharacteristic(characteristic)
        }
    }
    private fun enrollmentFinished() {
        if (enrollmentDone || gatt == null) return
        enrollmentDone = true; handler.removeCallbacks(enrollmentTimeout)
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
    private fun succeedIfReady() {
        if (!connected || ready || target?.bondState != BluetoothDevice.BOND_BONDED) return
        ready = true; state = "connected"; failure = null
        prefs.edit().putString("address", selectedAddress).putString("name", selectedName).apply()
        clearAttempt()
        publish()
        val callback = completion; completion = null; callback?.invoke(null)
        optional { pumpMetadata() }
    }

    private fun pumpMetadata() {
        if (!ready || !connected || blocked || operation != Operation.NONE) return
        if (rediscoverPending) { rediscoverPending = false; discover(); return }
        if (!servicesKnown) { discover(); return }
        if (!mtuAttempted) {
            mtuAttempted = true
            beginOperation(Operation.MTU)
            if (gatt?.requestMtu(96) != true) { endOperation(); stopMetadata("unavailable"); pumpTelemetry() }
            return
        }
        if (metadataCharacteristic == null) {
            if (metadataState != "unsupported") stopMetadata("unsupported")
        } else if (mtu >= 47 && !metadataActive) {
            metadataActive = true
            setMetadataState("syncing")
            phone.start { queueMetadata() }
            metadataPending = true
            handler.postDelayed(periodicMetadata, 30000)
        }
        if (metadataActive && metadataPending) {
            if (SystemClock.elapsedRealtime() >= maxOf(lastAttempt + 1100, metadataNotBefore)) { writeMetadata(); return }
            scheduleMetadata(0)
        }
        pumpTelemetry()
    }
    private fun pumpTelemetry() {
        if (!ready || blocked || operation != Operation.NONE) return
        val current = gatt ?: return
        if (!capabilityAttempted) {
            val characteristic = capabilitiesCharacteristic
            if (characteristic == null) { telemetryState = "unsupported"; capabilityAttempted = true; publish(); return }
            if (SystemClock.elapsedRealtime() < capabilityRetryAt) return
            capabilityAttempted = true
            read(current, characteristic, Operation.CAPABILITIES)
            return
        }
        val session = telemetry ?: return
        if (session.state == "unsupported" || session.state == "unavailable") return
        if (!subscribeAttempted) {
            val characteristic = telemetryCharacteristic ?: return
            val descriptor = characteristic.getDescriptor(cccdId)
            subscribeAttempted = true
            if (descriptor == null || !current.setCharacteristicNotification(characteristic, true)) {
                session.state = "unavailable"; publish(); return
            }
            beginOperation(Operation.SUBSCRIBE)
            val started = if (Build.VERSION.SDK_INT >= 33) {
                current.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE) == BluetoothStatusCodes.SUCCESS
            } else {
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                current.writeDescriptor(descriptor)
            }
            if (!started) { endOperation(); session.state = "unavailable"; publish() }
            return
        }
        if (subscribed && baselineDue) {
            baselineDue = false
            telemetryCharacteristic?.let { read(current, it, Operation.BASELINE) }
        } else if (subscribed && healthDue) {
            healthDue = false
            if (mtu >= 83 && session.capabilities.features and 4L != 0L) {
                healthCharacteristic?.let { read(current, it, Operation.HEALTH) }
            }
        }
    }
    private fun read(current: BluetoothGatt, characteristic: BluetoothGattCharacteristic, next: Operation) {
        beginOperation(next)
        readDispatch = SystemClock.elapsedRealtime()
        if (!current.readCharacteristic(characteristic)) {
            endOperation()
            if (next == Operation.CAPABILITIES) { telemetryState = "unavailable"; publish() }
            if (next == Operation.BASELINE) {
                handler.removeCallbacks(baselineRefresh); handler.postDelayed(baselineRefresh, 2000)
            }
        }
    }
    private fun resetTelemetry() {
        synchronized(notificationLock) { pendingNotification = null }
        handler.removeCallbacks(telemetryTick); handler.removeCallbacks(baselineRefresh); handler.removeCallbacks(resumePump); handler.removeCallbacks(healthRefresh)
        telemetry = null; telemetryState = "notConnected"; capabilityAttempted = false; capabilityRetries = 0
        capabilityRetryAt = 0; subscribed = false; subscribeAttempted = false; baselineDue = false; healthDue = false
    }
    fun refreshMetadata() { if (metadataActive) queueMetadata() }
    private fun queueMetadata() {
        if (!metadataActive) return
        metadataPending = true
        if (operation == Operation.NONE) scheduleMetadata(0)
    }
    private fun scheduleMetadata(delay: Long) {
        handler.removeCallbacks(sendMetadata)
        val spacing = (maxOf(lastAttempt + 1100, metadataNotBefore) - SystemClock.elapsedRealtime()).coerceAtLeast(0)
        handler.postDelayed(sendMetadata, maxOf(spacing, delay))
    }
    private fun writeMetadata() {
        if (!metadataActive || !ready || blocked || operation != Operation.NONE) return
        if (target?.bondState != BluetoothDevice.BOND_BONDED) { stopMetadata("unavailable"); return }
        val current = gatt ?: return
        val characteristic = metadataCharacteristic ?: return
        metadataPending = false
        // Construct at dispatch time; no saved snapshot can cross connections or retries.
        val bytes = phone.snapshot()
        beginOperation(Operation.METADATA)
        lastAttempt = SystemClock.elapsedRealtime()
        if (!write(current, characteristic, bytes)) { endOperation(); metadataFailed() }
    }
    private fun metadataFailed() {
        val fresh = lastAccepted?.let { SystemClock.elapsedRealtime() - it < 90000 } == true
        if (!fresh) setMetadataState("unavailable")
        if (metadataRetries < 3) {
            val delay = 1000L shl metadataRetries++
            metadataNotBefore = SystemClock.elapsedRealtime() + delay
            metadataPending = true
            scheduleMetadata(delay)
        } else {
            metadataRetries = 0
            metadataPending = false
            // Wait for the next normal observation/30-second refresh, no pairing loop.
        }
    }
    private fun stopMetadata(value: String) {
        metadataActive = false; metadataPending = false
        handler.removeCallbacks(sendMetadata); handler.removeCallbacks(periodicMetadata); handler.removeCallbacks(metadataExpiry)
        phone.stop()
        setMetadataState(value)
    }
    private fun optional(block: () -> Unit) {
        try { block() }
        catch (_: SecurityException) { retryAllowed = false; fail("Bluetooth permission was revoked. Restore Nearby devices access and retry.") }
        catch (_: Exception) { fail("Bluetooth operation unavailable. Reconnecting through the single owner.") }
    }
    private fun guarded(block: () -> Unit) {
        try { block() }
        catch (_: SecurityException) { retryAllowed = false; fail("Bluetooth permission was revoked. Restore Nearby devices access and retry.") }
        catch (_: Exception) { fail("Bluetooth is unavailable. Check the radio and permissions, then retry.") }
    }
    private fun clearAttempt() {
        handler.removeCallbacks(timeout); handler.removeCallbacks(enrollmentTimeout)
        if (registered) { try { context.unregisterReceiver(receiver) } catch (_: IllegalArgumentException) {} ; registered = false }
    }
    private fun close() {
        clearAttempt()
        handler.removeCallbacks(operationTimeout)
        stopMetadata("notConnected")
        resetTelemetry()
        val old = gatt; gatt = null
        try { old?.disconnect() } catch (_: Exception) {}
        try { old?.close() } catch (_: Exception) {}
        target = null; connected = false; ready = false; bondRequested = false; enrollmentDone = false; observedBonding = false
        operation = Operation.NONE; blocked = false; servicesKnown = false; metadataCharacteristic = null
        capabilitiesCharacteristic = null; telemetryCharacteristic = null; healthCharacteristic = null; rediscoverPending = false
        mtuAttempted = false; mtu = 23; metadataRetries = 0; metadataNotBefore = 0; lastAttempt = -1100; lastAccepted = null
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
