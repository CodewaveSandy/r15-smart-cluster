package dev.r15cluster.r15_companion

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest
import java.util.zip.CRC32
import kotlin.math.*

/** Exact phase-1 bytes; no Bluetooth, Flutter, clocks or retained GPS history. */
internal object MapWire {
    fun get(b: ByteArray, p: Int, n: Int = 4): Long {
        var v = 0L
        for (i in 0 until n) v = v or ((b[p + i].toLong() and 255) shl (8 * i))
        return v
    }
    fun put(b: ByteArray, p: Int, v: Long, n: Int = 4) { for (i in 0 until n) b[p + i] = (v ushr (8 * i)).toByte() }
    fun crc(b: ByteArray, end: Int = b.size): Long = CRC32().apply { update(b, 0, end) }.value
    fun valid(b: ByteArray, kind: Char, size: Int): Boolean = b.size == size &&
        b.copyOfRange(0, 4).contentEquals("R15$kind".toByteArray(Charsets.US_ASCII)) &&
        b[4].toInt() == 1 && get(b, size - 4) == crc(b, size - 4)
    fun zero(b: ByteArray, p: Int, n: Int) = (p until p + n).all { b[it].toInt() == 0 }
    fun forward(value: Long, previous: Long) = value != 0L && (previous == 0L || ((value - previous) and 0xffffffffL) in 1..0x7fffffffL)
    data class Context(val vehicle: Long, val connection: Long, val display: Long)
    fun context(b: ByteArray) = Context(get(b, 8), get(b, 12), get(b, 16))
    data class Caps(val context: Context, val ready: Boolean, val maxBytes: Int, val maxRoads: Int, val maxValue: Int)
    fun capabilities(b: ByteArray): Caps? {
        if (!valid(b, 'K', 64) || b[5].toInt() != 64 || b[6].toInt() !in 0..7 || b[7].toInt() != 0 || !zero(b, 40, 20) ||
            get(b, 20) !in 64..65536 || get(b, 24, 2) !in 1..2048 || b[26].toInt() != 8 || b[27].toInt() != 23 ||
            get(b, 28, 2) !in 93..240 || get(b, 30, 2) != 96L || get(b, 32, 2) !in 1..4096 || get(b, 34, 2) != 500L || get(b, 36, 2) != 2000L || get(b, 38, 2) != 5000L) return null
        val c = context(b)
        if (c.vehicle == 0L || c.connection == 0L || (b[6].toInt() != 0 && c.display == 0L)) return null
        return Caps(c, b[6].toInt() == 7, get(b, 20).toInt(), get(b, 24, 2).toInt(), get(b, 28, 2).toInt())
    }
    data class Status(val context: Context, val state: Int, val error: Int, val generation: Long, val sequence: Long,
        val offset: Int, val total: Int, val active: Long, val displayNow: Long, val t0: Long, val db: Long, val location: Int, val credit: Boolean)
    fun status(b: ByteArray): Status? {
        if (!valid(b, 'S', 80) || b[5].toInt() != 80 || b[6].toInt() !in 0..6 || b[7].toInt() !in 0..10 ||
            b[68].toInt() !in 0..7 || b[69].toInt() !in 0..1 || !zero(b, 70, 2) || get(b, 28) > get(b, 32) || get(b, 32) > 65536 ||
            get(b, 40, 8) < 0 || get(b, 48, 8) < 0 || get(b, 56, 8) < 0 || get(b, 56, 8) > get(b, 40, 8)) return null
        return Status(context(b), b[6].toInt(), b[7].toInt(), get(b, 20), get(b, 24), get(b, 28).toInt(), get(b, 32).toInt(), get(b, 36), get(b, 40, 8), get(b, 48, 8), get(b, 56, 8), b[68].toInt(), b[69].toInt() == 1)
    }
    fun command(c: Context, type: Int, sequence: Long, generation: Long = 0, body: ByteArray = byteArrayOf()): ByteArray {
        require(sequence != 0L && body.size <= 204)
        val b = ByteArray(36 + body.size)
        "R15X".toByteArray(Charsets.US_ASCII).copyInto(b); b[4] = 1; b[5] = type.toByte(); b[6] = 32
        put(b, 8, c.vehicle); put(b, 12, c.connection); put(b, 16, c.display); put(b, 20, generation); put(b, 24, sequence); put(b, 28, body.size.toLong(), 2)
        body.copyInto(b, 32); put(b, b.size - 4, crc(b, b.size - 4)); return b
    }
    data class Region(val bytes: ByteArray, val sha: ByteArray, val roads: Int, val south: Int, val west: Int, val north: Int, val east: Int) {
        fun contains(lat: Double, lon: Double) = lat * 1e7 in south.toDouble()..north.toDouble() && lon * 1e7 in west.toDouble()..east.toDouble()
    }
    fun region(b: ByteArray, expectedSha: String): Region {
        require(b.size in 64..65536 && b.copyOfRange(0, 8).contentEquals(byteArrayOf(82,49,53,66,1,0,64,0))) { "Unsupported map package" }
        val sha = MessageDigest.getInstance("SHA-256").digest(b)
        require(sha.joinToString("") { "%02x".format(it.toInt() and 255) } == expectedSha && get(b, 36) == (b.size - 64).toLong() &&
            CRC32().apply { update(b, 64, b.size - 64) }.value == get(b, 44) && get(b, 34, 2) == 0L && zero(b, 48, 16)) { "Map integrity check failed" }
        val ints = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)
        val lat = ints.getInt(8); val lon = ints.getInt(12); val s = ints.getInt(16); val w = ints.getInt(20); val n = ints.getInt(24); val e = ints.getInt(28)
        require(s >= -850000000 && n <= 850000000 && w >= -1800000000 && e <= 1800000000 && s < n && w < e && lat in s..n && lon in w..e)
        val k = 6371008.8 * PI / 180 / 1e7
        fun east(x: Int) = (x.toDouble() - lon) * k * cos(lat * PI / 180 / 1e7)
        fun north(y: Int) = (y.toDouble() - lat) * k
        require(listOf(east(w), east(e), north(s), north(n)).all { abs(it) <= 5000.5 })
        val count = get(b, 32, 2).toInt(); require(count in 1..2048)
        var offset = 64
        repeat(count) {
            require(offset + 4 <= b.size)
            val points = b[offset + 1].toInt() and 255; val names = b[offset + 2].toInt() and 255
            require(b[offset].toInt() in 0..4 && points in 2..8 && names in 0..23 && b[offset + 3].toInt() == 0 && offset + 4 + names + points * 4 <= b.size)
            require((0 until names).all { b[offset + 4 + it].toInt() in 32..126 } && (names == 0 || (0 until names).any { b[offset + 4 + it].toInt() != 32 }))
            val coords = offset + 4 + names
            var distinct = false
            repeat(points) { i ->
                val x = ints.getShort(coords + i * 4).toInt(); val y = ints.getShort(coords + i * 4 + 2).toInt()
                require(x in -20000..20000 && y in -20000..20000 && x >= roundAway(east(w)) - 1 && x <= roundAway(east(e)) + 1 && y >= roundAway(north(s)) - 1 && y <= roundAway(north(n)) + 1)
                distinct = distinct || x != ints.getShort(coords).toInt() || y != ints.getShort(coords + 2).toInt()
            }
            require(distinct); offset = coords + points * 4
        }
        require(offset == b.size)
        return Region(b, sha, count, s, w, n, e)
    }
    private fun roundAway(v: Double) = (sign(v) * floor(abs(v) + 0.5)).toInt()
}

internal data class PhoneFix(val sampled: Long = 0, val lat: Double = 0.0, val lon: Double = 0.0, val accuracy: Double? = null,
    val bearing: Double? = null, val bearingAccuracy: Double? = null, val speed: Double? = null, val precise: Boolean = false,
    val mock: Boolean = false, val problem: String = "waiting") {
    fun quality(now: Long): String = when {
        problem != "fix" -> problem
        mock || !lat.isFinite() || !lon.isFinite() || lat !in -85.0..85.0 || lon !in -180.0..180.0 || sampled > now -> "invalid"
        now - sampled >= 2000 -> "stale"
        !precise || accuracy == null || !accuracy.isFinite() || accuracy < 0 || accuracy > 50 -> "accuracyLow"
        else -> "fresh"
    }
    fun packet(now: Long): ByteArray {
        val b = ByteArray(40); val q = quality(now)
        MapWire.put(b, 8, now, 8); MapWire.put(b, 24, 0xffffffffL); MapWire.put(b, 28, 65535, 2); MapWire.put(b, 30, 65535, 2); MapWire.put(b, 32, 65535, 2)
        b[34] = ((if (precise) 1 else 0) or (if (mock) 2 else 0)).toByte()
        if (q == "fresh") {
            MapWire.put(b, 0, sampled, 8); MapWire.put(b, 16, round(lat * 1e7).toLong()); MapWire.put(b, 20, round(lon * 1e7).toLong()); MapWire.put(b, 24, ceil(accuracy!! * 100).toLong()); b[35] = 1
            bearing?.takeIf { it.isFinite() && it >= 0 && it < 360 }?.let { MapWire.put(b, 28, (it * 100).toLong(), 2) }
            bearingAccuracy?.takeIf { it.isFinite() && it in 0.0..180.0 }?.let { MapWire.put(b, 30, ceil(it * 100).toLong(), 2) }
            speed?.takeIf { it.isFinite() && it >= 0 }?.let { MapWire.put(b, 32, (it * 100).toLong().coerceAtMost(65534), 2) }
        } else if (q == "accuracyLow" && !mock) { b[34] = (b[34].toInt() or 4).toByte(); b[35] = 3 }
        else b[35] = (if (q == "stale") 2 else if (q == "invalid") 3 else 0).toByte()
        return b
    }
    fun snapshot(now: Long, region: MapWire.Region?): Map<String, Any?> {
        val q = quality(now)
        return mapOf("quality" to q, "latitude" to lat.takeIf { q == "fresh" }, "longitude" to lon.takeIf { q == "fresh" },
            "accuracyMeters" to accuracy.takeIf { q == "fresh" }, "bearing" to bearing.takeIf { q == "fresh" && bearingAccuracy != null && bearingAccuracy <= 45 && speed != null && speed >= 2 },
            "insideRegion" to (q == "fresh" && region?.contains(lat, lon) == true))
    }
}
