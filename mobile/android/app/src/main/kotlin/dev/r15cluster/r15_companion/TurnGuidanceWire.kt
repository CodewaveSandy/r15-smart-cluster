package dev.r15cluster.r15_companion

import java.util.zip.CRC32

/** Frozen docs/turn-guidance-protocol.md. No Android/Flutter dependencies. */
internal object TurnGuidanceWire {
    const val UNKNOWN = 0xffffffffL
    const val MIN_MTU = 227

    fun u(b: ByteArray, at: Int, n: Int = 4): Long {
        var value = 0L
        repeat(n) { value = value or ((b[at + it].toLong() and 255L) shl (it * 8)) }
        return value
    }

    fun put(b: ByteArray, at: Int, value: Long, n: Int = 4) {
        repeat(n) { b[at + it] = (value ushr (it * 8)).toByte() }
    }

    fun seal(b: ByteArray): ByteArray {
        put(b, b.size - 4, CRC32().apply { update(b, 0, b.size - 4) }.value)
        return b
    }

    private fun valid(b: ByteArray, magic: String, size: Int): Boolean =
        b.size == size &&
            String(b, 0, 4, Charsets.US_ASCII) == magic &&
            u(b, 4, 1) == 1L &&
            u(b, 5, 1) == 0L &&
            u(b, 6, 1) == size.toLong() &&
            u(b, size - 4) == CRC32().apply { update(b, 0, size - 4) }.value

    private fun zero(b: ByteArray, at: Int, n: Int) = (at until at + n).all { b[it] == 0.toByte() }

    data class Cap(
        val boot: Long,
        val connection: Long,
        val display: Long,
        val ready: Boolean,
        val mtu: Int,
    )

    fun capability(b: ByteArray): Cap? {
        if (!valid(b, "R15F", 80) || u(b, 7, 1) > 1 || !zero(b, 64, 12) || b[27] != 0.toByte())
            return null
        val fields =
            mapOf(20 to 224L, 22 to 227L, 28 to 511L, 34 to 2000L, 52 to 500L, 54 to 25000L)
        if (fields.any { (at, expected) -> u(b, at, 2) != expected }) return null
        if (
            listOf(26 to 1, 30 to 96, 31 to 32, 32 to 16, 33 to 2).any { (at, v) ->
                u(b, at, 1) != v.toLong()
            }
        )
            return null
        if (
            mapOf(36 to 5000L, 40 to 30000L, 44 to 120000L, 48 to 30000L).any { (at, v) ->
                u(b, at) != v
            }
        )
            return null
        val cap = Cap(u(b, 8), u(b, 12), u(b, 16), b[7] == 1.toByte(), u(b, 24, 2).toInt())
        if (cap.mtu !in 23..517 || u(b, 56, 8) < 0) return null
        if (
            cap.ready &&
                (cap.boot == 0L || cap.connection == 0L || cap.display == 0L || cap.mtu < MIN_MTU)
        )
            return null
        return cap
    }

    data class Status(
        val boot: Long,
        val connection: Long,
        val display: Long,
        val stream: Long,
        val sequence: Long,
        val revision: Long,
        val flags: Int,
        val source: Int,
        val quality: Int,
        val error: Int,
        val reason: Int,
        val age: Long,
        val received: Long,
        val published: Long,
        val request: Long,
        val t0: Long,
        val db: Long,
        val changed: Long,
        val rejected: Long,
    )

    fun status(b: ByteArray): Status? {
        if (!valid(b, "R15W", 112) || !zero(b, 60, 4)) return null
        val s =
            Status(
                u(b, 8),
                u(b, 12),
                u(b, 16),
                u(b, 20),
                u(b, 24),
                u(b, 28),
                u(b, 7, 1).toInt(),
                u(b, 32, 1).toInt(),
                u(b, 33, 1).toInt(),
                u(b, 34, 1).toInt(),
                u(b, 35, 1).toInt(),
                u(b, 36),
                u(b, 40, 8),
                u(b, 48, 8),
                u(b, 56),
                u(b, 64, 8),
                u(b, 72, 8),
                u(b, 80, 8),
                u(b, 96),
            )
        if (s.source !in 0..6 || s.quality !in 0..8 || s.error !in 0..8 || s.reason !in 0..6)
            return null
        if ((s.flags and 32 != 0) != (s.age != UNKNOWN)) return null
        if (
            s.flags and 1 != 0 &&
                (s.boot == 0L || s.connection == 0L || s.display == 0L || s.stream == 0L)
        )
            return null
        if (s.flags and 18 != 0 && s.flags and 1 == 0) return null
        if (s.flags and 128 != 0 && s.flags and 4 == 0) return null
        if (
            s.flags and 8 != 0 &&
                (s.flags and 51 != 51 ||
                    s.quality != 2 ||
                    s.age >= 30000 ||
                    s.source !in listOf(1, 3))
        )
            return null
        if (listOf(s.received, s.published, s.t0, s.db, u(b, 88, 8)).any { it < 0 }) return null
        if (s.changed < 0 && s.changed != -1L) return null
        if (s.request != 0L && (s.t0 == 0L || s.db == 0L)) return null
        return s
    }

    fun command(
        cap: Cap,
        stream: Long,
        sequence: Long,
        type: Int,
        body: ByteArray,
        revision: Long = 0,
        age: Long = UNKNOWN,
    ): ByteArray {
        require(
            body.size <= 180 &&
                cap.boot != 0L &&
                cap.connection != 0L &&
                cap.display != 0L &&
                stream != 0L &&
                sequence != 0L
        )
        val b = ByteArray(44 + body.size)
        "R15U".toByteArray(Charsets.US_ASCII).copyInto(b)
        b[4] = 1
        b[5] = type.toByte()
        b[6] = 40
        put(b, 8, cap.boot)
        put(b, 12, cap.connection)
        put(b, 16, cap.display)
        put(b, 20, stream)
        put(b, 24, sequence)
        put(b, 28, revision)
        put(b, 32, age)
        put(b, 36, body.size.toLong(), 2)
        body.copyInto(b, 40)
        return seal(b)
    }

    fun snapshot(value: TurnObservation, now: Long): Pair<ByteArray, Long> {
        val b = ByteArray(180)
        b[0] = value.source.toByte()
        b[1] = value.maneuver.toByte()
        var flags = 0
        fun text(value: String, at: Int, lenAt: Int, max: Int, bit: Int) {
            require(value.length <= max && value.all { it.code in 32..126 })
            if (value.isNotEmpty()) {
                flags = flags or bit
                b[lenAt] = value.length.toByte()
                value.toByteArray(Charsets.US_ASCII).copyInto(b, at)
            }
        }
        text(value.instruction, 36, 17, 96, 1)
        text(value.road, 132, 18, 32, 2)
        text(value.summary, 164, 19, 16, 4)
        if (value.maneuver != 255) flags = flags or 8
        put(b, 4, value.next ?: UNKNOWN)
        put(b, 8, value.remaining ?: UNKNOWN)
        put(b, 12, value.seconds ?: UNKNOWN)
        if (value.next != null) flags = flags or 16
        if (value.remaining != null) flags = flags or 32
        if (value.seconds != null) flags = flags or 64
        if (value.exit != 0) {
            flags = flags or 128
            b[16] = value.exit.toByte()
        }
        if (value.truncated) flags = flags or 256
        val changed = value.changedAt
        val age =
            if (changed != null && now >= changed && now - changed < UNKNOWN) now - changed
            else UNKNOWN
        if (age != UNKNOWN) flags = flags or 512
        put(b, 2, flags.toLong(), 2)
        put(b, 20, if (age == UNKNOWN) -1L else changed!!, 8)
        put(b, 28, now.coerceAtLeast(1), 8)
        return b to age
    }
}

internal data class TurnObservation(
    val source: Int = 0,
    val maneuver: Int = 255,
    val instruction: String = "",
    val road: String = "",
    val summary: String = "",
    val next: Long? = null,
    val remaining: Long? = null,
    val seconds: Long? = null,
    val exit: Int = 0,
    val truncated: Boolean = false,
    val changedAt: Long? = null,
    val revision: Long = 0,
) {
    fun core() = listOf(source, maneuver, instruction, road, next, exit, truncated)
}
