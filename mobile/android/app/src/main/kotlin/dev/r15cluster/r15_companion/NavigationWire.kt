package dev.r15cluster.r15_companion

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest
import java.util.zip.CRC32
import kotlin.math.*

/** Exact c019/c01a/c01b v2 wire adapter. No Android or UI ownership. */
internal object NavigationWire {
    fun buffer(size: Int) = ByteBuffer.allocate(size).order(ByteOrder.LITTLE_ENDIAN)

    fun u32(b: ByteArray, at: Int) =
        ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).getInt(at).toLong() and 0xffffffffL

    fun crc(b: ByteArray, size: Int = b.size) = CRC32().apply { update(b, 0, size) }.value

    fun sha(b: ByteArray) = MessageDigest.getInstance("SHA-256").digest(b)

    fun valid(b: ByteArray, magic: String, size: Int) =
        b.size == size &&
            b.copyOfRange(0, 4).contentEquals(magic.toByteArray()) &&
            b[4].toInt() == 2 &&
            u32(b, size - 4) == crc(b, size - 4)

    data class Context(
        val vehicle: Long,
        val connection: Long,
        val display: Long,
        val session: Long,
    )

    data class Cap(val context: Context, val window: Int, val value: Int, val rate: Int)

    fun cap(b: ByteArray): Cap? {
        if (
            !valid(b, "R15J", 64) ||
                b[5].toInt() != 64 ||
                b[6].toInt() != 15 ||
                b[7].toInt() !in 1..4 ||
                (8..16 step 4).any { u32(b, it) == 0L } ||
                u32(b, 24) != 32768L ||
                b[28].toInt() != 12 ||
                b[29].toInt() != 4 ||
                b[58].toInt() != 0 ||
                b[59].toInt() != 0
        )
            return null
        val v = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)
        if (
            u32(b, 20) !in 1..1048576L ||
                v.getShort(32).toInt() != 2048 ||
                v.getShort(42).toInt() != 2000 ||
                v.getShort(44).toInt() != 30000 ||
                v.getShort(46).toInt() != 10000 ||
                b[56].toInt() != 6 ||
                b[57].toInt() != 2 ||
                v.getShort(38).toInt() != 96 ||
                v.getShort(30).toInt() != 512 ||
                v.getShort(34).toInt() != 512 ||
                u32(b, 48) and 1L == 0L ||
                u32(b, 52) and 2L == 0L
        )
            return null
        return Cap(
            Context(u32(b, 8), u32(b, 12), u32(b, 16), 0),
            b[7].toInt(),
            v.getShort(36).toInt().coerceAtMost(240),
            v.getShort(40).toInt().coerceAtMost(4096),
        )
    }

    fun packet(
        c: Context,
        type: Int,
        seq: Long,
        id: Long = 0,
        offset: Int = 0,
        body: ByteArray = byteArrayOf(),
    ): ByteArray {
        require(body.size <= 196 && seq != 0L && c.session != 0L)
        val b =
            buffer(44 + body.size)
                .put("R15V".toByteArray())
                .put(2)
                .put(type.toByte())
                .put(40)
                .put(0)
        b.putInt(c.vehicle.toInt())
            .putInt(c.connection.toInt())
            .putInt(c.display.toInt())
            .putInt(c.session.toInt())
            .putInt(seq.toInt())
            .putInt(id.toInt())
            .putInt(offset)
            .putShort(body.size.toShort())
            .putShort(0)
            .put(body)
        b.putInt(crc(b.array(), b.capacity() - 4).toInt())
        return b.array()
    }

    data class Reply(
        val type: Int,
        val seq: Long,
        val id: Long,
        val offset: Int,
        val body: ByteArray,
    )

    fun reply(b: ByteArray, c: Context): Reply? {
        if (
            b.size !in listOf(68, 84, 88) ||
                !valid(b, "R15Y", b.size) ||
                b[6].toInt() != 40 ||
                b[7].toInt() != 0 ||
                b[38].toInt() != 0 ||
                b[39].toInt() != 0
        )
            return null
        val v = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)
        if (
            v.getShort(36).toInt() != b.size - 44 ||
                u32(b, 8) != c.vehicle ||
                u32(b, 12) != c.connection ||
                u32(b, 16) != c.display ||
                u32(b, 20) != c.session
        )
            return null
        val type = b[5].toInt() and 255
        if (
            (type == 64 && b.size != 84) ||
                (type == 65 && b.size != 88) ||
                (type == 66 && b.size != 68) ||
                type !in 64..66
        )
            return null
        return Reply(
            type,
            u32(b, 24),
            u32(b, 28),
            u32(b, 32).toInt(),
            b.copyOfRange(40, b.size - 4),
        )
    }

    fun eligible(f: PhoneFix, now: Long) =
        f.problem == "fix" &&
            !f.mock &&
            f.sampled in 1..now &&
            now - f.sampled < 2000 &&
            f.lat.isFinite() &&
            abs(f.lat) <= 85.0511287 &&
            f.lon.isFinite() &&
            abs(f.lon) <= 180 &&
            f.accuracy?.let { it.isFinite() && it in 0.0..1000000.0 } == true

    fun precise(f: PhoneFix, now: Long) = eligible(f, now) && f.precise && f.accuracy!! <= 50

    fun location(f: PhoneFix, now: Long): ByteArray {
        val ok = eligible(f, now)
        val b = buffer(40)
        b.putLong(if (ok) f.sampled else 0)
            .putLong(now)
            .putInt(if (ok) (f.lat * 1e7).roundToInt() else 0)
            .putInt(if (ok) (f.lon * 1e7).roundToInt() else 0)
            .putInt(if (ok) (f.accuracy!! * 100).roundToInt() else -1)
        val course =
            if (
                ok &&
                    f.speed?.let { it.isFinite() && it >= 2 } == true &&
                    f.bearingAccuracy?.let { it.isFinite() && it in 0.0..45.0 } == true &&
                    f.bearing?.isFinite() == true
            )
                f.bearing
            else null
        b.putShort(
                course?.let {
                    ((it % 360 + 360) % 360 * 100).roundToInt().coerceAtMost(35999).toShort()
                } ?: (-1).toShort()
            )
            .putShort(
                if (course != null) (f.bearingAccuracy!! * 100).roundToInt().toShort()
                else (-1).toShort()
            )
            .putShort(
                if (ok)
                    f.speed
                        ?.takeIf { it.isFinite() && it >= 0 }
                        ?.let { (it * 100).roundToInt().coerceIn(0, 65534).toShort() }
                        ?: (-1).toShort()
                else (-1).toShort()
            )
            .put((if (f.mock) 3 else if (ok) if (f.precise) 1 else 2 else 0).toByte())
            .put(
                (if (ok) 1
                    else if (
                        f.mock ||
                            f.problem == "invalid" ||
                            (f.problem == "fix" &&
                                (f.sampled > now ||
                                    !f.lat.isFinite() ||
                                    abs(f.lat) > 85.0511287 ||
                                    !f.lon.isFinite() ||
                                    abs(f.lon) > 180 ||
                                    f.accuracy?.let { it.isFinite() && it in 0.0..1000000.0 } !=
                                        true))
                    )
                        3
                    else if (f.problem == "fix") 2 else 0)
                    .toByte()
            )
        return b.array()
    }

    data class Object(
        val bytes: ByteArray,
        val key: String,
        val kind: Int,
        val route: Long = 0,
        val revision: Int = 0,
        val start: Int = 0,
        val end: Int = 0,
    ) {
        val hash: ByteArray = sha(bytes)
    }

    fun obj(
        source: ByteArray,
        kind: Int,
        key: String,
        count: Int,
        body: ByteArray,
        cell: MapCell? = null,
        route: Long = 0,
        revision: Int = 0,
        start: Int = 0,
        end: Int = 0,
        distance: Int = 0,
        seconds: Int = 0,
    ): Object {
        require(source.size == 32 && count in 0..512 && body.size + 64 <= 32768)
        val b =
            buffer(64 + body.size)
                .put("R15O".toByteArray())
                .put(2)
                .put(kind.toByte())
                .put(64)
                .put(0)
        b.putInt(u32(source, 0).toInt())
            .putShort(1)
            .putShort(2)
            .putLong(route)
            .putInt(revision)
            .put((cell?.z ?: 0).toByte())
            .put(0)
            .putShort(count.toShort())
            .putInt(cell?.x ?: 0)
            .putInt(cell?.y ?: 0)
            .putInt(start)
            .putInt(end)
            .putInt(distance)
            .putInt(seconds)
            .putInt(crc(body).toInt())
            .putInt(0)
            .put(body)
        return Object(b.array(), key, kind, route, revision, start, end)
    }
}

internal data class GeoPoint(val lat: Double, val lon: Double) {
    init {
        require(lat.isFinite() && lon.isFinite() && abs(lat) <= 85.0511287 && abs(lon) <= 180)
    }

    fun distance(other: GeoPoint): Double {
        val a = (other.lat - lat) * PI / 180
        val b = (other.lon - lon) * PI / 180
        val h =
            sin(a / 2).pow(2) + cos(lat * PI / 180) * cos(other.lat * PI / 180) * sin(b / 2).pow(2)
        return 12742000 * asin(sqrt(h.coerceIn(0.0, 1.0)))
    }

    fun json() = listOf(lon, lat)
}

internal data class MapCell(val z: Int, val x: Int, val y: Int) {
    init {
        require(z in 0..22 && x in 0 until (1 shl z) && y in 0 until (1 shl z))
    }

    val key
        get() = "$z/$x/$y"

    fun parent(max: Int): MapCell {
        val d = (z - max).coerceAtLeast(0)
        return MapCell(z - d, x shr d, y shr d)
    }

    fun point(u: Double, v: Double): GeoPoint {
        val n = (1 shl z).toDouble()
        return GeoPoint(
            (atan(sinh(PI * (1 - 2 * (y + v) / n))) * 180 / PI).coerceIn(-85.0511287, 85.0511287),
            (x + u) / n * 360 - 180,
        )
    }

    companion object {
        fun at(p: GeoPoint, z: Int): MapCell {
            val n = 1 shl z
            val lat = p.lat * PI / 180
            return MapCell(
                z,
                floor((p.lon + 180) / 360 * n).toInt().coerceIn(0, n - 1),
                floor((1 - ln(tan(lat) + 1 / cos(lat)) / PI) / 2 * n).toInt().coerceIn(0, n - 1),
            )
        }
    }
}
