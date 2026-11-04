package dev.r15cluster.r15_companion

import kotlin.math.ceil

/** Main-thread v2 scheduler. ATT serialization remains exclusively in BleBond. */
internal class NavigationSync {
    data class Request(val kind: String, val bytes: ByteArray = byteArrayOf())

    private data class Pending(
        val type: Int,
        val seq: Long,
        val id: Long,
        val bytes: ByteArray,
        val end: Int,
        var sent: Long,
        var retries: Int = 0,
        val activation: String = "",
    )

    private data class Transfer(
        val id: Long,
        val obj: NavigationWire.Object,
        var phase: Int = 0,
        var offset: Int = 0,
    )

    var wanted = false
        private set

    var fix = PhoneFix()
    var progress: ByteArray? = null
    var onInterest: ((List<MapCell>) -> Unit)? = null
    var onWindow: ((Int) -> Unit)? = null
    var onSession: (() -> Unit)? = null
    var reconnectNeeded = false
        private set

    private var source = byteArrayOf()
    private var cap: NavigationWire.Cap? = null
    private var context: NavigationWire.Context? = null
    private var capAt = 0L
    private var capAsked = false
    private var subscription = false
    private var subscriptionAsked = false
    private var seq = 0L
    private var locationSeq = 0L
    private var progressSeq = 0L
    private var sessions = 0L
    private var opened = false
    private var anchored = 0L
    private var confirm: ByteArray? = null
    private var lastLocation = 0L
    private var lastProgress = 0L
    private var nextData = 0L
    private var lastStatus = 0L
    private var displayNow = 0L
    private var readAt = 0L
    private var stopType = 0
    private var error: String? = null
    private var stage = 0
    private var remoteError = 0
    private var coverage = 0
    private var quality = 0
    private var camera = 0
    private var ceiling = false
    private var slots = 1
    private var ack = 0L
    private var ids = 0L
    private val pending = java.util.ArrayDeque<Pending>()
    private val queue = java.util.ArrayDeque<NavigationWire.Object>()
    private var transfer: Transfer? = null
    private val complete = LinkedHashMap<String, Pair<Long, NavigationWire.Object>>()
    private var route: NavigationRoute? = null
    private var currentCell = ""
    private var activeWindow = ""
    private var activated = ""
    private var interestGeneration = 0L
    private var needGeneration = 0L
    private var sentBytes = 0L
    private var started = 0L

    fun sharing(value: Boolean, digest: ByteArray = source) {
        if (value) {
            if (wanted && source.contentEquals(digest)) return
            if (context != null) {
                resetSession()
                sessions++
            }
            source = digest.copyOf()
            wanted = true
            stopType = 0
            error = null
        } else {
            wanted = false
            stopType = 11
            pending.clear()
            queue.clear()
            transfer = null
            progress = null
        }
    }

    fun end() {
        wanted = false
        stopType = 12
        pending.clear()
        queue.clear()
        transfer = null
        progress = null
        route = null
        activated = ""
    }

    private fun resetSession() {
        context = null
        opened = false
        anchored = 0
        confirm = null
        pending.clear()
        seq = 0
        locationSeq = 0
        progressSeq = 0
        queue.clear()
        transfer = null
        complete.clear()
        activated = ""
        activeWindow = ""
        interestGeneration = 0
        needGeneration = 0
        ack = 0
        lastStatus = 0
        displayNow = 0
        readAt = 0
        lastLocation = 0
        lastProgress = 0
        sentBytes = 0
        started = 0
        nextData = 0
    }

    fun resetConnection() {
        cap = null
        capAsked = false
        capAt = 0
        subscription = false
        subscriptionAsked = false
        lastStatus = 0
        displayNow = 0
        readAt = 0
        error = null
        reconnectNeeded = false
        stage = 0
        quality = 0
        coverage = 0
        camera = 0
        ceiling = false
        resetSession()
    }

    fun setRoute(value: NavigationRoute?) {
        if (route?.id == value?.id && route?.revision == value?.revision) return
        route = value
        progress = null
        activated = ""
        activeWindow = ""
        queue.removeAll { it.kind != 1 }
        // Cancel the old candidate before placing a new revision behind its barrier.
        if (transfer?.obj?.kind != 1 && transfer != null) {
            stopType = 7
            pending.clear()
        }
    }

    fun enqueue(obj: NavigationWire.Object, current: Boolean = false) {
        if (!wanted) return
        if (current && obj.kind == 1) currentCell = obj.key
        if (obj.kind == 3) activeWindow = obj.key
        if (
            complete[obj.key]?.second?.hash?.contentEquals(obj.hash) == true ||
                transfer?.obj?.key == obj.key ||
                queue.any { it.key == obj.key }
        )
            return
        if (queue.size >= 16) {
            val stale = queue.firstOrNull { it.kind == 1 && it.key != currentCell }
            if (stale != null) queue.remove(stale) else return
        }
        if (current) queue.addFirst(obj) else queue.addLast(obj)
    }

    fun capabilities(bytes: ByteArray?, now: Long) {
        val next = bytes?.let { NavigationWire.cap(it) }
        if (next == null || next.window < 1 || next.value < 93 || next.rate <= 0) {
            capAt = now + 2000
            capAsked = false
            error = "Waiting for authenticated v2 display capability"
            return
        }
        if (cap != null && cap!!.context != next.context) {
            reconnectNeeded = true
            return
        }
        cap = next
        capAsked = true
        error = null
        slots = next.window
    }

    fun subscribed(ok: Boolean) {
        subscription = ok
        if (!ok) error = "V2 status subscription failed; reconnect"
    }

    fun writeResult(ok: Boolean, now: Long) {
        if (!ok) {
            pending.lastOrNull()?.sent = now - 2000
            error = "Retrying Bluetooth write"
        }
    }

    fun readFailed() {
        readAt = 0
    }

    fun receive(bytes: ByteArray, now: Long) {
        val c = context ?: return
        val r = NavigationWire.reply(bytes, c) ?: return
        if (r.type == 65) {
            val b = r.body
            val gen = NavigationWire.u32(b, 0)
            val z = b[8].toInt() and 255
            val count = b[9].toInt() and 255
            val have = b[10].toInt() and 255
            if (
                r.id != 0L ||
                    r.offset != 0 ||
                    gen != r.seq ||
                    NavigationWire.u32(b, 4) != NavigationWire.u32(source, 0) ||
                    z !in 0..22 ||
                    count !in 0..4 ||
                    b[11].toInt() !in 0..1 ||
                    have ushr count != 0 ||
                    (interestGeneration != 0L && !forwardOrEqual(gen, interestGeneration))
            )
                return
            val cells = ArrayList<MapCell>()
            for (i in 0..3) {
                val x = NavigationWire.u32(b, 12 + i * 8)
                val y = NavigationWire.u32(b, 16 + i * 8)
                if (i < count) {
                    if (x >= (1L shl z) || y >= (1L shl z)) return
                    if (have and (1 shl i) == 0) cells.add(MapCell(z, x.toInt(), y.toInt()))
                } else if (x != 0L || y != 0L) return
            }
            interestGeneration = gen
            cells.forEach { complete.remove(it.key) }
            onInterest?.invoke(cells)
            return
        }
        if (r.type == 66) {
            val b = r.body
            val rr = route ?: return
            val v = java.nio.ByteBuffer.wrap(b).order(java.nio.ByteOrder.LITTLE_ENDIAN)
            val at = NavigationWire.u32(b, 12)
            if (
                v.getLong(0) != rr.id ||
                    NavigationWire.u32(b, 8) != rr.revision.toLong() ||
                    at > rr.distance ||
                    r.id != 0L ||
                    r.offset != 0 ||
                    (needGeneration != 0L && !forwardOrEqual(r.seq, needGeneration))
            )
                return
            if (
                NavigationWire.u32(b, 16) > NavigationWire.u32(b, 20) ||
                    NavigationWire.u32(b, 20) > rr.distance
            )
                return
            needGeneration = r.seq
            onWindow?.invoke(at.toInt())
            return
        }
        val b = r.body
        val st = b[0].toInt() and 255
        val err = b[1].toInt() and 255
        val win = b[2].toInt() and 255
        val free = b[3].toInt() and 255
        if (
            st !in 0..6 ||
                err !in 0..12 ||
                win !in 1..4 ||
                free > win ||
                b[12].toInt() !in 0..6 ||
                b[13].toInt() !in 0..2 ||
                b[14].toInt() !in 0..1 ||
                b[15].toInt() !in 0..1 ||
                !forwardOrEqual(r.seq, ack) ||
                !forwardOrEqual(seq, r.seq)
        )
            return
        val clock = java.nio.ByteBuffer.wrap(b).order(java.nio.ByteOrder.LITTLE_ENDIAN).getLong(16)
        if (
            clock < displayNow ||
                r.offset < 0 ||
                NavigationWire.u32(b, 4) > 32768 ||
                r.offset > NavigationWire.u32(b, 4)
        )
            return
        displayNow = clock
        lastStatus = now
        stage = st
        remoteError = err
        quality = b[12].toInt()
        coverage = b[13].toInt()
        camera = b[14].toInt()
        ceiling = b[15].toInt() != 0
        slots = free
        ack = r.seq
        val waiting = transfer
        if (
            waiting != null &&
                waiting.phase == 4 &&
                err == 0 &&
                r.id == waiting.id &&
                st in listOf(4, 6) &&
                r.offset == waiting.obj.bytes.size
        )
            finish(waiting)
        val p = pending.firstOrNull() ?: return
        if (!forwardOrEqual(r.seq, p.seq)) return
        if (err != 0) {
            if (err == 10 && p.type in listOf(2, 3)) {
                pending.clear()
                anchored = 0
                confirm = null
                readAt = now + 1000
                return
            }
            if (err == 9 && p.type == 9) {
                pending.clear()
                activated = ""
                currentCell = ""
                onSession?.invoke()
                return
            }
            if (err in listOf(1, 6)) {
                pending.forEach { it.sent = now - 2000 }
                return
            }
            if (err == 3) {
                pending.forEach { it.sent = now - 2000 }
                return
            }
            error = "Display rejected v2 operation ($err). Stop sharing and retry"
            wanted = false
            stopType = 11
            pending.clear()
            return
        }
        while (pending.isNotEmpty() && forwardOrEqual(r.seq, pending.first().seq)) {
            val item = pending.removeFirst()
            val t = transfer
            when (item.type) {
                1 -> {
                    opened = true
                    started = now
                    onSession?.invoke()
                }
                2 -> {
                    val v = java.nio.ByteBuffer.wrap(b).order(java.nio.ByteOrder.LITTLE_ENDIAN)
                    val t0 =
                        java.nio.ByteBuffer.wrap(item.bytes)
                            .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                            .getLong(40)
                    if (v.getLong(24) == t0 && now >= t0 && now - t0 <= 500)
                        confirm =
                            NavigationWire.buffer(20)
                                .putInt(item.seq.toInt())
                                .putLong(t0)
                                .putLong(now)
                                .array()
                    else {
                        anchored = 0
                        readAt = now + 1500
                    }
                }
                3 -> {
                    anchored = now
                    confirm = null
                }
                12 -> {
                    if (t != null && r.id == t.id) {
                        if (st == 6 && r.offset == t.obj.bytes.size) finish(t)
                        else {
                            t.phase = 1
                            t.offset = 0
                        }
                    }
                }
                4 -> {
                    if (t != null && r.id == t.id && r.offset in 0..t.obj.bytes.size) {
                        t.phase = if (st == 6) 4 else 2
                        t.offset = r.offset
                        if (st == 6) finish(t)
                    } else {
                        error = "Invalid BEGIN acknowledgement"
                        wanted = false
                    }
                }
                5 -> {
                    if (
                        t != null &&
                            r.id == t.id &&
                            r.offset >= item.end &&
                            r.offset <= t.obj.bytes.size
                    )
                        t.offset = r.offset
                    else {
                        error = "Invalid cumulative DATA acknowledgement"
                        wanted = false
                    }
                }
                6 -> {
                    if (t != null && r.id == t.id && st in listOf(4, 6)) finish(t)
                    else if (t != null) t.phase = 4
                }
                7 -> {
                    transfer = null
                    stopType = 0
                }
                9 -> {
                    if (NavigationWire.u32(b, 8) == NavigationWire.u32(item.bytes, 48)) {
                        activated = item.activation
                        error = null
                    } else {
                        error = "Route activation revision mismatch"
                        reconnectNeeded = true
                    }
                }
                11,
                13 -> {
                    stopType = 0
                    anchored = 0
                    opened = false
                    if (item.type == 13) context = null
                    transfer = null
                    complete.clear()
                    activated = ""
                }
            }
        }
        // COMMIT can first acknowledge Verifying; publication arrives later with same sequence.
        val t = transfer
        if (
            t != null &&
                t.phase == 4 &&
                r.id == t.id &&
                st in listOf(4, 6) &&
                r.offset == t.obj.bytes.size
        )
            finish(t)
    }

    private fun finish(t: Transfer) {
        complete[t.obj.key] = t.id to t.obj
        transfer = null
        while (complete.size > 12) {
            val key =
                complete.entries
                    .firstOrNull {
                        it.key != currentCell &&
                            it.key != activeWindow &&
                            !(it.value.second.kind == 2 &&
                                it.value.second.route == route?.id &&
                                it.value.second.revision == route?.revision)
                    }
                    ?.key ?: complete.keys.first()
            complete.remove(key)
        }
    }

    private fun forwardOrEqual(a: Long, b: Long) =
        a == b || ((a - b) and 0xffffffffL) in 1..0x7fffffffL

    private fun nextSeq(): Long {
        seq = (seq + 1) and 0xffffffffL
        if (seq == 0L) seq = 1
        return seq
    }

    private fun command(
        type: Int,
        now: Long,
        body: ByteArray = byteArrayOf(),
        id: Long = 0,
        offset: Int = 0,
        end: Int = 0,
    ): Request {
        val s = nextSeq()
        val bytes = NavigationWire.packet(context!!, type, s, id, offset, body)
        pending.add(
            Pending(type, s, id, bytes, end, now, activation = if (type == 9) activeWindow else "")
        )
        return Request("write", bytes)
    }

    fun next(now: Long, mtu: Int, endpoints: Boolean): Request? {
        if (!wanted && stopType == 0) return null
        if (!endpoints || mtu < 96) {
            error =
                "Navigation v2 requires updated boards and MTU 96+; phone navigation remains available"
            return null
        }
        if (cap == null) {
            if (!capAsked && now >= capAt) {
                capAsked = true
                return Request("capability")
            }
            return null
        }
        if (!subscriptionAsked) {
            subscriptionAsked = true
            return Request("subscribe")
        }
        if (!subscription) return null
        if (context == null) {
            if (!wanted) {
                stopType = 0
                return null
            }
            sessions = (sessions + 1) and 0xffffffffL
            if (sessions == 0L) sessions = 1
            context = cap!!.context.copy(session = sessions)
            seq = 0
            ack = 0
            opened = false
            return command(
                1,
                now,
                NavigationWire.buffer(40)
                    .put(source)
                    .putShort(1)
                    .putShort(2)
                    .put(cap!!.window.toByte())
                    .array(),
            )
        }
        if (stopType != 0 && pending.isEmpty()) {
            val type =
                when (stopType) {
                    7 -> 7
                    12 -> 13
                    else -> 11
                }
            return command(type, now, id = if (type == 7) transfer?.id ?: 0 else 0)
        }
        val first = pending.firstOrNull()
        if (first != null && now - first.sent >= 2000) {
            if (now - readAt >= 1000) {
                readAt = now
                return Request("status")
            }
            if (first.retries >= 3) {
                reconnectNeeded = true
                return null
            }
            // Retry the missing prefix and all following unacknowledged DATA in order.
            val retry = pending.firstOrNull { now - it.sent >= 2000 } ?: return null
            retry.retries++
            retry.sent = now
            return Request("write", retry.bytes)
        }
        if (!opened) return null
        if (lastStatus > 0 && now - lastStatus >= 8000) {
            reconnectNeeded = true
            return null
        }
        if (wanted && anchored != 0L && now - lastLocation >= 500) {
            lastLocation = now
            locationSeq = (locationSeq + 1) and 0xffffffffL
            if (locationSeq == 0L) locationSeq = 1
            return Request(
                "observation",
                NavigationWire.packet(
                    context!!,
                    8,
                    locationSeq,
                    body = NavigationWire.location(fix, now),
                ),
            )
        }
        if (
            wanted &&
                progress != null &&
                anchored != 0L &&
                now - lastProgress >= 500 &&
                activated.isNotEmpty()
        ) {
            lastProgress = now
            progressSeq = (progressSeq + 1) and 0xffffffffL
            if (progressSeq == 0L) progressSeq = 1
            return Request(
                "observation",
                NavigationWire.packet(context!!, 10, progressSeq, body = progress!!),
            )
        }
        if (pending.any { it.type != 5 }) return null
        if (pending.isEmpty()) {
            if (!wanted) return null
            if (confirm != null) return command(3, now, confirm!!)
            if ((anchored == 0L || now - anchored >= 25000) && now >= readAt)
                return command(2, now, NavigationWire.buffer(8).putLong(now).array())
            val rr = route
            val overview =
                complete.values.firstOrNull {
                    it.second.kind == 2 &&
                        it.second.route == rr?.id &&
                        it.second.revision == rr?.revision
                }
            val window = complete[activeWindow]
            val cell = complete[currentCell]
            if (
                rr != null &&
                    overview != null &&
                    window != null &&
                    cell != null &&
                    activated != activeWindow
            )
                return command(
                    9,
                    now,
                    NavigationWire.buffer(24)
                        .putLong(rr.id)
                        .putInt(rr.revision)
                        .putInt(overview.first.toInt())
                        .putInt(window.first.toInt())
                        .putInt(cell.first.toInt())
                        .array(),
                )
            if (transfer == null && queue.isNotEmpty()) {
                ids = (ids + 1) and 0xffffffffL
                if (ids == 0L) ids = 1
                transfer = Transfer(ids, queue.removeFirst())
            }
            val t = transfer
            if (t != null)
                when (t.phase) {
                    0 -> {
                        t.phase = 5
                        return command(
                            12,
                            now,
                            NavigationWire.buffer(40)
                                .put(t.obj.hash)
                                .putInt(t.obj.bytes.size)
                                .array(),
                            t.id,
                        )
                    }
                    1 -> {
                        t.phase = 5
                        return command(
                            4,
                            now,
                            NavigationWire.buffer(40)
                                .putInt(t.obj.bytes.size)
                                .put(t.obj.hash)
                                .put(t.obj.kind.toByte())
                                .array(),
                            t.id,
                        )
                    }
                    2 ->
                        if (t.offset == t.obj.bytes.size) {
                            t.phase = 3
                            return command(6, now, id = t.id)
                        }
                    4 ->
                        if (now - readAt >= 1000) {
                            readAt = now
                            return Request("status")
                        }
                }
        }
        val t = transfer
        if (
            t != null &&
                t.phase == 2 &&
                pending.size < minOf(cap!!.window, 4) &&
                slots > pending.size &&
                now >= nextData
        ) {
            val at = if (pending.isEmpty()) t.offset else pending.last().end
            if (at < t.obj.bytes.size) {
                val n = minOf(t.obj.bytes.size - at, minOf(240, mtu - 3, cap!!.value) - 44)
                val req = command(5, now, t.obj.bytes.copyOfRange(at, at + n), t.id, at, at + n)
                sentBytes += req.bytes.size
                // Exact inner escaping plus a worst-case 37 encoded bytes per
                // 48-byte tunnel fragment (header, UART sequence/CRC, prefix/end).
                val escaped =
                    req.bytes.sumOf {
                        if (it == 0.toByte() || it == 0x52.toByte() || it == 0x7d.toByte()) 2 else 1
                    }
                val encoded = escaped + ((req.bytes.size + 47) / 48) * 37
                nextData = now + ceil(encoded * 1000.0 / cap!!.rate).toLong()
                return req
            }
        }
        if (now - lastStatus >= 1500 && now - readAt >= 1000) {
            readAt = now
            return Request("status")
        }
        return null
    }

    fun snapshot(now: Long): Map<String, Any?> =
        mapOf(
            "sharing" to wanted,
            "state" to
                when {
                    error != null -> "blocked"
                    !wanted -> "stopped"
                    cap == null -> "connecting"
                    activated.isNotEmpty() -> "displayReady"
                    transfer != null -> "transferring"
                    else -> "preparing"
                },
            "error" to error,
            "stage" to stage,
            "remoteError" to remoteError,
            "coverage" to coverage,
            "quality" to
                if (lastStatus > 0 && now - lastStatus < 2000) quality
                else if (quality in listOf(1, 2, 3)) 3 else 0,
            "statusAgeMs" to if (lastStatus > 0) now - lastStatus else null,
            "camera" to camera,
            "geometryCeiling" to ceiling,
            "objects" to complete.size,
            "queued" to queue.size,
            "offset" to (transfer?.offset ?: 0),
            "total" to (transfer?.obj?.bytes?.size ?: 0),
            "sentBytes" to sentBytes,
            "bytesPerSecond" to
                if (started > 0 && now > started) sentBytes * 1000 / (now - started) else 0,
            "anchorAgeMs" to if (anchored > 0) now - anchored else null,
        )
}
