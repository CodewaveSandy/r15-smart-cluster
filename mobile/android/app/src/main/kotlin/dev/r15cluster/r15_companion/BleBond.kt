package dev.r15cluster.r15_companion

import android.app.Activity
import android.bluetooth.*
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.Handler
import android.os.Looper

/** One bounded OS-owned numeric-comparison bonding operation. No application data. */
@Suppress("DEPRECATION")
class BleBond(private val activity: Activity, private val complete: (Boolean, String?) -> Unit) {
    private val handler = Handler(Looper.getMainLooper())
    private var gatt: BluetoothGatt? = null
    private var target: BluetoothDevice? = null
    private var registered = false
    private var done = false
    private val timeout = Runnable { finish(false, "Pairing timed out. Open Pair phone on the cluster, then try again.") }
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val device = intent.getParcelableExtra<BluetoothDevice>(BluetoothDevice.EXTRA_DEVICE)
            if (done || device?.address != target?.address) return
            when (intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.BOND_NONE)) {
                BluetoothDevice.BOND_BONDED -> {
                    try { if (target?.bondState == BluetoothDevice.BOND_BONDED) finish(true, null) }
                    catch (_: SecurityException) { finish(false, "Bluetooth permission was revoked.") }
                }
                BluetoothDevice.BOND_NONE -> finish(false, "Pairing was rejected or cancelled. Check the cluster enrollment window and try again.")
            }
        }
    }
    fun start(device: BluetoothDevice) {
        target = device
        try {
            if (device.bondState == BluetoothDevice.BOND_BONDED) { finish(true, null); return }
            val filter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
            if (Build.VERSION.SDK_INT >= 33) activity.registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED)
            else activity.registerReceiver(receiver, filter)
            registered = true
            handler.postDelayed(timeout, 45000)
            gatt = device.connectGatt(activity, false, object : BluetoothGattCallback() {
                override fun onConnectionStateChange(connection: BluetoothGatt, status: Int, newState: Int) {
                    handler.post {
                        if (done || gatt !== connection) return@post
                        try {
                            if (status != BluetoothGatt.GATT_SUCCESS || newState == BluetoothProfile.STATE_DISCONNECTED) {
                                finish(false, "Cluster disconnected during pairing. Enable Pair phone on the cluster and compare both codes.")
                            } else if (newState == BluetoothProfile.STATE_CONNECTED && device.bondState == BluetoothDevice.BOND_NONE) {
                                if (!device.createBond() && device.bondState == BluetoothDevice.BOND_NONE) {
                                    finish(false, "Android could not start pairing. Try again from the cluster enrollment window.")
                                }
                            }
                        } catch (_: SecurityException) { finish(false, "Bluetooth permission was revoked. Allow Nearby devices access and retry.") }
                    }
                }
            }, BluetoothDevice.TRANSPORT_LE)
            if (gatt == null) finish(false, "The BLE connection could not start.")
        } catch (_: Exception) { finish(false, "Pairing is unavailable. Check Bluetooth and Nearby devices access.") }
    }
    fun cancel() { finish(false, "Pairing stopped. Any bond already saved by Android is retained.") }
    private fun finish(bonded: Boolean, error: String?) {
        if (done) return
        done = true
        handler.removeCallbacks(timeout)
        if (registered) {
            try { activity.unregisterReceiver(receiver) } catch (_: IllegalArgumentException) { }
            registered = false
        }
        try { gatt?.disconnect() } catch (_: SecurityException) { }
        try { gatt?.close() } catch (_: SecurityException) { }
        gatt = null
        complete(bonded, error)
    }
}
