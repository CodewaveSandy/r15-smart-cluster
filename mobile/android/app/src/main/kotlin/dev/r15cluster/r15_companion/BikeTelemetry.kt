package dev.r15cluster.r15_companion

import java.util.zip.CRC32
import kotlin.math.abs
import kotlin.math.ceil

/** Strict, Android-free wire validation. CRC is integrity, not authentication. */
internal object TelemetryWire {
    fun u8(b: ByteArray, p: Int) = b[p].toInt() and 255
    fun u16(b: ByteArray, p: Int) = u8(b, p) or (u8(b, p + 1) shl 8)
    fun u32(b: ByteArray, p: Int): Long = (0..3).fold(0L) { n, i -> n or (u8(b, p + i).toLong() shl (8 * i)) }
    fun delta(a: Long, b: Long) = (a - b) and 0xffffffffL
    private fun magic(b: ByteArray, value: String) = String(b, 0, 4, Charsets.US_ASCII) == value
    private fun crc(b: ByteArray) = CRC32().apply { update(b, 0, b.size - 4) }.value == u32(b, b.size - 4)
    data class Capabilities(val boot: Long, val features: Long)
    fun capabilities(b: ByteArray): Capabilities? {
        if (b.size != 20 || !magic(b, "R15C") || u8(b, 4) != 1 || u8(b, 5) != 0 ||
            u8(b, 6) != 1 || u8(b, 7) != 0 || u32(b, 8) == 0L ||
            u32(b, 12) and 7L.inv() != 0L || u16(b, 16) != 64 || u16(b, 18) != 200) return null
        return Capabilities(u32(b, 8), u32(b, 12))
    }
    data class Packet(val boot: Long, val connection: Long, val sequence: Long, val uptime: Long,
        val sourceAge: Int, val metadataAge: Int, val flags: Int, val qualities: List<Int>,
        val gear: Int, val lamps: List<Boolean>, val trip: Long, val info: Long, val sourceSequence: Long)
    fun packet(b: ByteArray): Packet? {
        if (b.size != 64 || !magic(b, "R15T") || u8(b, 4) != 1 || u8(b, 5) != 64 ||
            u8(b, 6) > 7 || u8(b, 7) != 0 || u32(b, 8) == 0L || u32(b, 12) == 0L ||
            u32(b, 40) != 0L || u32(b, 44) != 0L || !crc(b) || (36..39).any { u8(b, it) != 2 }) return null
        val age = u16(b, 24); val metadataAge = u16(b, 26); val flags = u8(b, 6)
        if ((flags and 4 != 0) != (age != 65535 && age < 1000) ||
            (flags and 2 != 0 && metadataAge >= 90)) return null
        val qualities = listOf(28, 30, 32, 34).map { u8(b, it) }
        if (qualities.any { it !in 0..4 } ||
            (if (qualities[0] == 0) u8(b, 29) > 6 else u8(b, 29) != 255)) return null
        for (p in listOf(30, 32, 34)) {
            if (u8(b, p + 1) > 1 || (u8(b, p) != 0 && u8(b, p + 1) != 0)) return null
        }
        if (age == 65535) {
            if (qualities.any { it != 1 } || (48..59).any { u8(b, it) != 0 }) return null
        } else if (age >= 1000 && qualities.any { it != 4 }) return null
        return Packet(u32(b, 8), u32(b, 12), u32(b, 16), u32(b, 20), age, metadataAge, flags,
            qualities, u8(b, 29), listOf(31, 33, 35).map { u8(b, it) == 1 },
            u32(b, 48), u32(b, 52), u32(b, 56))
    }
    fun health(b: ByteArray): Map<String, Long>? {
        if (b.size != 80 || !magic(b, "R15H") || u8(b, 4) != 1 || u8(b, 5) != 80 ||
            u16(b, 6) != 0 || u32(b, 8) == 0L || u32(b, 12) == 0L || u32(b, 72) != 0L || !crc(b)) return null
        val names = listOf("accepted", "rejected", "invalid", "rateLimited", "expired", "connections", "disconnects",
            "notifyQueued", "notifyCompleted", "notifyErrors", "maxMetadataGapMs", "maxHostGapMs", "maxSourceGapMs")
        return names.mapIndexed { i, name -> name to u32(b, 20 + i * 4) }.toMap() +
            mapOf("boot" to u32(b, 8), "connection" to u32(b, 12), "uptime" to u32(b, 16))
    }
}

/** One latest snapshot and time anchor per GATT; never replay input counters. */
internal class TelemetrySession(val capabilities: TelemetryWire.Capabilities) {
    private var token: Long? = null
    private var latest: TelemetryWire.Packet? = null
    private var receipt = 0L
    private var unwrapped = 0L
    private var anchorUptime: Long? = null
    private var anchorDispatch = 0L
    private var anchorRtt = 0L
    private var observedUncertainty = 0L
    var sessionFault = false; private set
    var rejected = 0L; private set
    var maxReceiptGapMs = 0L; private set
    var health: Map<String, Long>? = null; private set
    var state = "initializing"
    private fun reject(): Boolean { rejected = (rejected + 1).coerceAtMost(0xffffffffL); return false }
    fun accept(bytes: ByteArray, now: Long, dispatch: Long? = null): Boolean {
        val p = TelemetryWire.packet(bytes) ?: return reject()
        if (p.boot != capabilities.boot || (token != null && p.connection != token)) {
            sessionFault = true; return reject()
        }
        val old = latest
        var packetUptime = p.uptime
        var newer = true
        if (old != null) {
            val sequenceDelta = TelemetryWire.delta(p.sequence, old.sequence)
            val uptimeDelta = TelemetryWire.delta(p.uptime, old.uptime)
            newer = sequenceDelta in 1L..0x7fffffffL
            if (newer) {
                if (uptimeDelta >= 0x80000000L) return reject()
                packetUptime = unwrapped + uptimeDelta
            } else {
                // A read may be older than a notification already received. It can
                // anchor timing, but cannot rewind the newest observation/receipt.
                val backwards = TelemetryWire.delta(old.uptime, p.uptime)
                if (dispatch == null || sequenceDelta == 0L || sequenceDelta == 0x80000000L || backwards >= 0x80000000L) return reject()
                packetUptime = unwrapped - backwards
            }
        }
        token = p.connection
        if (dispatch != null && now - dispatch in 0L..500L) {
            val previousAnchor = anchorUptime
            if (previousAnchor != null) {
                // Measured anchor movement includes scheduling/RTT uncertainty.
                val deviation = abs((dispatch - anchorDispatch) - (packetUptime - previousAnchor))
                observedUncertainty = maxOf(observedUncertainty, deviation + anchorRtt + (now - dispatch))
            }
            anchorUptime = packetUptime; anchorDispatch = dispatch; anchorRtt = now - dispatch
        }
        if (newer) {
            if (old != null) maxReceiptGapMs = maxOf(maxReceiptGapMs, now - receipt)
            latest = p; receipt = now; unwrapped = packetUptime
        }
        return true
    }
    fun acceptHealth(bytes: ByteArray): Boolean {
        val h = TelemetryWire.health(bytes) ?: return reject()
        if (h["boot"] != capabilities.boot || (token != null && h["connection"] != token)) {
            sessionFault = true; return reject()
        }
        // Health cannot establish telemetry timing or freshness.
        token = h["connection"]; health = h
        return true
    }
    fun snapshot(now: Long): Map<String, Any?> {
        val p = latest
        val anchor = anchorUptime
        val anchorAge = now - anchorDispatch
        // Provisional 1000ppm clock budget + 2ms quantization, enlarged by actual
        // anchor drift/RTT observations. B1 must validate this engineering bound.
        val margin = 2 + observedUncertainty + ceil(anchorAge.coerceAtLeast(0) / 1000.0).toLong()
        val deliveryAge = if (p == null || anchor == null) null else maxOf(0, anchorAge - (unwrapped - anchor)) + margin
        val age = if (p == null || deliveryAge == null) null else p.sourceAge.toLong() + deliveryAge
        val timed = anchor != null && anchorAge < 30000
        val fresh = p != null && timed && age != null && age < 1000 && now - receipt < 1000
        val noSource = p?.sourceAge == 65535 && timed && now - receipt < 1000
        val qualities = p?.qualities?.map { if (!fresh && !noSource && it != 2) 4 else it } ?: List(4) { 1 }
        return mapOf("state" to (if (state != "subscribed") state else if (fresh) "fresh" else "stale"),
            "gearQuality" to qualities[0], "gear" to p?.gear?.takeIf { qualities[0] == 0 },
            "leftQuality" to qualities[1], "left" to p?.lamps?.get(0)?.takeIf { qualities[1] == 0 },
            "rightQuality" to qualities[2], "right" to p?.lamps?.get(1)?.takeIf { qualities[2] == 0 },
            "highBeamQuality" to qualities[3], "highBeam" to p?.lamps?.get(2)?.takeIf { qualities[3] == 0 },
            "tripPresses" to p?.trip, "infoPresses" to p?.info, "sourceSequence" to p?.sourceSequence,
            "sourceAgeBoundMs" to age, "receiptAgeMs" to p?.let { now - receipt },
            "displayFresh" to (p != null && timed && deliveryAge != null && deliveryAge < 1000 && now - receipt < 1000 && p.flags and 1 != 0),
            "metadataFresh" to (p != null && timed && deliveryAge != null && p.flags and 2 != 0 && p.metadataAge * 1000L + deliveryAge < 90000),
            "timingReady" to timed, "baselineRttMs" to anchorRtt, "clockUncertaintyMs" to margin,
            "rejected" to rejected, "maxReceiptGapMs" to maxReceiptGapMs, "health" to health)
    }
    fun nextCheckDelay(now: Long): Long {
        val value = snapshot(now)
        if (value["state"] != "fresh") return 200
        val sourceRemaining = 1000 - (value["sourceAgeBoundMs"] as Long)
        return minOf(200L, sourceRemaining, 1000 - (now - receipt), 30000 - (now - anchorDispatch)).coerceAtLeast(1)
    }
}
