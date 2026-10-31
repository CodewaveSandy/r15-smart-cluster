package dev.r15cluster.r15_companion

import android.bluetooth.BluetoothAdapter
import android.content.*
import android.os.*
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.TimeZone

/** Current phone observations only. No GATT ownership, persistence or private logging. */
class PhoneMetadataSource(private val context: Context, private val adapter: () -> BluetoothAdapter?) {
    private var registered = false
    private var changed: (() -> Unit)? = null
    private var battery: Int? = null
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (!registered) return
            if (intent.action == Intent.ACTION_BATTERY_CHANGED) {
                val sampled = batteryPercent(intent)
                if (sampled == battery) return
                battery = sampled
            }
            changed?.invoke()
        }
    }

    fun start(onChange: () -> Unit) {
        stop()
        changed = onChange
        val filter = IntentFilter().apply {
            addAction(Intent.ACTION_BATTERY_CHANGED)
            addAction(Intent.ACTION_TIME_CHANGED)
            addAction(Intent.ACTION_TIMEZONE_CHANGED)
            addAction(Intent.ACTION_DATE_CHANGED)
        }
        try {
            // These are OS broadcasts. Re-sample actual observations, never trust time extras.
            registered = true
            val sticky = if (Build.VERSION.SDK_INT >= 33)
                context.registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED)
            else context.registerReceiver(receiver, filter)
            battery = batteryPercent(sticky)
        } catch (_: Exception) { registered = false; battery = null }
    }

    fun stop() {
        if (registered) {
            try { context.unregisterReceiver(receiver) } catch (_: IllegalArgumentException) { }
        }
        registered = false
        changed = null
        battery = null
    }

    fun label(): String {
        fun supported(value: String?) = value?.trim()?.takeIf {
            it.isNotEmpty() && it.all { c -> c.code in 32..126 }
        }?.take(32)
        val localName = try { adapter()?.name } catch (_: SecurityException) { null }
        return supported(localName) ?: supported("${Build.MANUFACTURER} ${Build.MODEL}") ?: "Android phone"
    }

    fun snapshot(): ByteArray {
        // Sticky battery is sampled again at send time, including after resume/reconnect.
        battery = try { batteryPercent(context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))) }
            catch (_: Exception) { null }
        val now = System.currentTimeMillis()
        val seconds = Math.floorDiv(now, 1000L)
        val offset = TimeZone.getDefault().getOffset(now) / 60000
        val validTime = seconds in 1577836800L..4102444799L && offset in -720..840
        return encode(label(), battery, if (validTime) seconds else null, if (validTime) offset else 0)
    }

    companion object {
        private fun batteryPercent(intent: Intent?): Int? {
            if (intent == null || !intent.getBooleanExtra(BatteryManager.EXTRA_PRESENT, true)) return null
            val level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
            val scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1)
            if (scale <= 0 || level !in 0..scale) return null
            return ((level.toLong() * 100) / scale).toInt().takeIf { it in 0..100 }
        }

        /** Normative 44-byte full snapshot. Invalid inputs never reach Android GATT. */
        fun encode(label: String?, battery: Int?, utcSeconds: Long?, offsetMinutes: Int): ByteArray {
            require(label == null || (label.length in 1..32 && label.all { it.code in 32..126 } && label.any { it != ' ' }))
            require(battery == null || battery in 0..100)
            require(utcSeconds == null || utcSeconds in 1577836800L..4102444799L)
            require(if (utcSeconds == null) offsetMinutes == 0 else offsetMinutes in -720..840)
            return ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN).apply {
                put(1.toByte()); put((label?.length ?: 0).toByte()); put((battery ?: 255).toByte()); put(0.toByte())
                putInt((utcSeconds ?: 0).toInt()); putShort(offsetMinutes.toShort()); putShort(0.toShort())
                if (label != null) put(label.toByteArray(Charsets.US_ASCII))
            }.array()
        }
    }
}
