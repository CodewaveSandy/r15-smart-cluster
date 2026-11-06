package dev.r15cluster.r15_companion

import java.security.SecureRandom

/** Protocol/session scheduler called only by BleBond's serialized ATT owner. */
internal class TurnGuidanceSync {
    data class Request(val kind: String, val bytes: ByteArray = byteArrayOf())

    private data class Pending(
        val bytes: ByteArray,
        val sequence: Long,
        val type: Int,
        val issued: Long,
        val observation: TurnObservation? = null,
        var deadline: Long = issued + 1000,
        var retries: Int = 0,
        var readFallback: Boolean = false,
    )

    private var cap: TurnGuidanceWire.Cap? = null
    private var previousContext: Triple<Long, Long, Long>? = null
    private var sequence = 0L
    private var stream = 0L
    private var oldStream = 0L
    private var pending: Pending? = null
    private var desired: TurnObservation? = null
    private var wanted = false
    private var clearReason = 0
    private var withdrawnRevision = -1L
    private var capAt = -1000L
    private var statusAt = -1000L
    private var readAt = -1000L
    private var capNeeded = true
    private var capFailures = 0
    private var baseline = false
    private var subscribed = false
    private var subscribeAttempted = false
    private var last: TurnGuidanceWire.Status? = null
    private var acknowledged: TurnObservation? = null
    private var wireRevision = 0L
    private var lastSnapshot = -500L
    private var lastPresence = -2000L
    private var clockAt = -30000L
    private var confirmBody: ByteArray? = null
    private var clockAttempts = 0
    private var failed = false
    private var phase = "stopped"
    private var problem: String? = null
    private var accepted = 0L
    private var rejected = 0L

    fun update(value: TurnObservation?, enabled: Boolean, reason: Int = 2) {
        wanted = enabled
        desired = if (enabled) value else null
        if ((!enabled || value == null) && stream != 0L) clearReason = reason
        if (
            value != null &&
                value.source !in listOf(1, 3) &&
                withdrawnRevision != value.revision &&
                (acknowledged?.source in listOf(1, 3) ||
                    pending?.observation?.source in listOf(1, 3))
        ) {
            clearReason = 4
            withdrawnRevision = value.revision
        }
    }

    fun reset() {
        cap = null
        pending = null
        oldStream = stream.takeIf { it != 0L } ?: oldStream
        stream = 0
        capNeeded = true
        baseline = false
        subscribed = false
        subscribeAttempted = false
        acknowledged = null
        wireRevision = 0
        last = null
        confirmBody = null
        clockAt = -30000
        capAt = -1000
        capFailures = 0
        statusAt = -1000
        readAt = -1000
        clearReason = 0
        failed = false
        clockAttempts = 0
        phase = if (wanted) "connecting" else "stopped"
        problem = null
        // Sequence/context survive rediscovery on the same secured connection.
    }

    fun active() = wanted || clearReason != 0 || pending != null

    fun retry() {
        reset()
    }

    fun capability(bytes: ByteArray?, now: Long) {
        capAt = now
        capNeeded = false
        val value = bytes?.let { TurnGuidanceWire.capability(it) }
        if (value == null) {
            if (bytes == null && capFailures++ < 3) {
                capNeeded = true
                phase = "waitingDisplay"
                problem = "Guidance capability read will retry"
                return
            }
            cap = null
            failed = true
            phase = "unsupported"
            problem = "Guidance capability is unavailable or incompatible"
            return
        }
        capFailures = 0
        val identity = Triple(value.boot, value.connection, value.display)
        if (previousContext != identity) {
            sequence = 0
            stream = 0
            oldStream = 0
            last = null
            statusAt = -1000
            readAt = -1000
            clearReason = 0
            clockAttempts = 0
            lastSnapshot = -500
            lastPresence = -2000
            pending = null
            baseline = false
            acknowledged = null
            wireRevision = 0
            confirmBody = null
            clockAt = -30000
            previousContext = identity
        }
        cap = value
        if (!value.ready) {
            phase = "waitingDisplay"
            problem = "Waiting for the vehicle/display connection"
        } else {
            problem = null
            if (stream == 0L) phase = "ready"
        }
    }

    fun subscribed(ok: Boolean) {
        subscribed = ok
        subscribeAttempted = true
        baseline = false
    }

    fun writeResult(ok: Boolean, now: Long) {
        if (!ok) {
            rejected++
            pending?.deadline = now + 150
            problem = "Cluster admission needs retry"
        }
    }

    fun readFailed(now: Long) {
        readAt = now
        if (pending == null) {
            phase = "waitingDisplay"
            capNeeded = true
        }
    }

    fun status(bytes: ByteArray, receivedAt: Long) {
        val value =
            TurnGuidanceWire.status(bytes)
                ?: run {
                    rejected++
                    return
                }
        val context = cap ?: return
        if (
            value.boot != context.boot ||
                value.connection != context.connection ||
                value.display != context.display
        ) {
            capNeeded = true
            return
        }
        if (last != null && value.received < last!!.received) return
        last = value
        statusAt = receivedAt
        baseline = true
        if (pending == null && (sequence == 0L || forward(value.sequence, sequence)))
            sequence = value.sequence
        val p =
            pending
                ?: run {
                    if (stream != 0L && value.stream == stream && value.flags and 1 == 0) {
                        oldStream = stream
                        stream = 0
                        acknowledged = null
                        wireRevision = 0
                        clockAt = -30000
                        confirmBody = null
                    }
                    return
                }
        if (value.stream != stream) return
        if (value.error != 0 && value.rejected == p.sequence) {
            pending = null
            rejected++
            if (value.error == 5) {
                clockAt = -30000
                confirmBody = null
                phase = "timing"
                return
            }
            if (value.error == 4) {
                oldStream = stream
                stream = 0
                wireRevision = 0
                acknowledged = null
                return
            }
            fail("Display rejected guidance (reason ${value.error}); retry guidance")
            return
        }
        if (value.sequence != p.sequence) return
        if (p.type == 4 && (value.flags and 4 == 0 || value.revision != p.observation?.revision))
            return
        when (p.type) {
            1 -> {
                clockAt = -30000
                confirmBody = null
                phase = "timing"
            }
            2 -> {
                val t0 = TurnGuidanceWire.u(p.bytes, 40, 8)
                if (value.request != p.sequence || value.t0 != t0 || value.db <= 0) return
                val rtt = receivedAt - t0
                if (rtt !in 0..500) {
                    pending = null
                    clockAttempts++
                    if (clockAttempts >= 3)
                        fail(
                            "Timing exchange exceeded 500 ms; check cluster link and retry guidance"
                        )
                    return
                }
                confirmBody =
                    ByteArray(32).also {
                        TurnGuidanceWire.put(it, 0, p.sequence)
                        TurnGuidanceWire.put(it, 8, t0, 8)
                        TurnGuidanceWire.put(it, 16, receivedAt, 8)
                        TurnGuidanceWire.put(it, 24, value.db, 8)
                    }
                clockAt = t0
            }
            3 -> {
                if (value.flags and 2 == 0) {
                    pending = null
                    clockAt = -30000
                    confirmBody = null
                    return
                }
                confirmBody = null
                clockAttempts = 0
                phase = "ready"
            }
            4 -> {
                acknowledged = p.observation
                wireRevision = value.revision
                accepted++
                phase = quality(value)
                problem = null
            }
            6 -> {
                oldStream = stream
                stream = 0
                clearReason = 0
                acknowledged = null
                wireRevision = 0
                confirmBody = null
                clockAt = -30000
                phase = if (wanted) "waitingMaps" else "stopped"
            }
        }
        pending = null
    }

    private fun quality(s: TurnGuidanceWire.Status) =
        when (s.quality) {
            2 -> "shared"
            3 -> "lastUpdate"
            4 -> "updatePending"
            5 -> "ageUnknown"
            6 -> "sourceUnsupported"
            7 -> "linkLost"
            else -> "sharedStatus"
        }

    private fun forward(a: Long, b: Long): Boolean {
        val d = (a - b) and TurnGuidanceWire.UNKNOWN
        return d in 1..0x7fffffffL
    }

    private fun fail(text: String) {
        if (pending?.type == 6) {
            oldStream = stream
            stream = 0
        }
        clearReason = 0
        failed = true
        phase = "error"
        problem = text
        pending = null
    }

    private fun nextSequence(): Long {
        sequence = (sequence + 1) and TurnGuidanceWire.UNKNOWN
        if (sequence == 0L) sequence = 1
        return sequence
    }

    private fun newStream(): Long {
        var result: Long
        do {
            result = SecureRandom().nextInt().toLong() and TurnGuidanceWire.UNKNOWN
        } while (result == 0L || result == oldStream)
        return result
    }

    private fun issue(
        type: Int,
        body: ByteArray,
        now: Long,
        observation: TurnObservation? = null,
        age: Long = TurnGuidanceWire.UNKNOWN,
    ): Request {
        val seq = nextSequence()
        val bytes =
            TurnGuidanceWire.command(
                cap!!,
                stream,
                seq,
                type,
                body,
                if (type == 5) wireRevision else observation?.revision ?: 0,
                age,
            )
        pending = Pending(bytes, seq, type, now, observation)
        return Request("write", bytes)
    }

    fun next(now: Long, mtu: Int, endpoints: Boolean): Request? {
        if (!wanted && clearReason == 0 && pending?.type != 6) {
            phase = "stopped"
            return null
        }
        if (!endpoints || mtu < TurnGuidanceWire.MIN_MTU) {
            phase = "unsupported"
            problem =
                if (!endpoints) "Install matching guidance firmware on both boards"
                else "Guidance needs negotiated MTU 227 or higher (actual $mtu)"
            return null
        }
        if (failed && clearReason == 0) return null
        if (capNeeded || cap == null || cap?.ready != true) {
            if (now - capAt >= 1000) {
                capAt = now
                return Request("capability")
            }
            return null
        }
        if (!subscribeAttempted) {
            subscribeAttempted = true
            return Request("subscribe")
        }
        if (!baseline) {
            if (now - readAt >= 250) {
                readAt = now
                return Request("status")
            }
            return null
        }
        if (
            clearReason != 0 &&
                stream != 0L &&
                (pending?.type != 6 ||
                    TurnGuidanceWire.u(pending!!.bytes, 40, 1).toInt() != clearReason)
        ) {
            failed = false
            phase = "clearing"
            return issue(6, byteArrayOf(clearReason.toByte(), 0, 0, 0), now)
        }
        val p = pending
        if (p != null) {
            if (
                p.type == 2 && now - readAt >= 100 && now - p.issued >= 100 && now - p.issued <= 600
            ) {
                readAt = now
                return Request("status")
            }
            if (now >= p.deadline) {
                if (!p.readFallback) {
                    p.readFallback = true
                    p.deadline = now + 500
                    readAt = now
                    return Request("status")
                }
                if (p.retries < 2 && now - p.issued < 4500) {
                    p.retries++
                    p.readFallback = false
                    p.deadline = now + 1000
                    return Request("write", p.bytes)
                }
                fail("No display acknowledgement; retry guidance (BLE sync remains active)")
            }
            return null
        }
        if (!wanted || desired == null) {
            if (stream == 0L) clearReason = 0
            phase = if (wanted) "waitingMaps" else "stopped"
            return null
        }
        if (stream == 0L) {
            stream = newStream()
            phase = "opening"
            return issue(1, byteArrayOf(1, 0, 0, 0), now)
        }
        val confirm = confirmBody
        if (confirm != null) {
            if (now - TurnGuidanceWire.u(confirm, 16, 8) > 350) {
                confirmBody = null
                clockAt = -30000
            } else return issue(3, confirm, now)
        }
        if (now - clockAt >= 24000) {
            phase = "timing"
            return issue(
                2,
                ByteArray(8).also { TurnGuidanceWire.put(it, 0, now.coerceAtLeast(1), 8) },
                now,
            )
        }
        val desired = desired!!
        val changed =
            acknowledged?.let {
                it.revision != desired.revision ||
                    it.summary != desired.summary ||
                    it.remaining != desired.remaining ||
                    it.seconds != desired.seconds
            } ?: true
        if (changed && now - lastSnapshot >= 500) {
            val encoded = TurnGuidanceWire.snapshot(desired, now)
            lastSnapshot = now
            return issue(4, encoded.first, now, desired, encoded.second)
        }
        if (now - lastPresence >= 2000) {
            lastPresence = now
            return issue(5, byteArrayOf(7, 0, 0, 0), now)
        }
        if (now - statusAt >= 1000 && now - readAt >= 500) {
            readAt = now
            return Request("status")
        }
        if (last != null && acknowledged != null) phase = quality(last!!)
        return null
    }

    fun snapshot(now: Long): Map<String, Any?> {
        val s = last?.takeIf { now - statusAt <= 2500 }
        return mapOf(
            "state" to phase,
            "problem" to problem,
            "capable" to (cap?.ready == true),
            "pending" to (pending != null),
            "acknowledged" to
                (s != null &&
                    acknowledged != null &&
                    acknowledged == desired &&
                    s.stream == stream &&
                    s.revision == acknowledged?.revision &&
                    s.revision == desired?.revision &&
                    s.flags and 4 != 0),
            "timingReady" to (s != null && s.flags and 2 != 0),
            "live" to (s != null && s.flags and 16 != 0),
            "displayQuality" to s?.quality,
            "displayVisible" to (s?.flags?.and(128) != 0 && s != null),
            "sourceAgeMs" to s?.age?.takeIf { it != TurnGuidanceWire.UNKNOWN },
            "accepted" to accepted,
            "rejected" to rejected,
            "notifications" to subscribed,
        )
    }
}
