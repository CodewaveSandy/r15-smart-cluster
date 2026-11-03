package dev.r15cluster.r15_companion

/** Main-thread state machine. BleBond alone dispatches ATT; ACKs belong to display. */
internal class MapSync {
    data class Request(val kind: String, val bytes: ByteArray = byteArrayOf())
    private data class Pending(val type: Int, val seq: Long, val bytes: ByteArray, val expected: Int = 0,
        val dispatched: Long, var deadline: Long, var retries: Int = 0, var readFirst: Boolean = true, var retryAt: Long = 0)
    var wanted = false; private set
    var region: MapWire.Region? = null; private set
    var fix = PhoneFix()
    var reconnectNeeded = false; private set
    private var caps: MapWire.Caps? = null
    private var status: MapWire.Status? = null
    private var statusAt = 0L
    private var nextStatusRead = 0L
    private var capsDue = 0L
    private var initialized = false
    private var subscribed = false
    private var subscribeNeeded = false
    private var pending: Pending? = null
    private var sequence = 0L
    private var locationSequence = 0L
    private var generation = 0L
    private var offset = 0
    private var anchored = 0L
    private var timeConfirm: ByteArray? = null
    private var began = false
    private var committed = false
    private var active = false
    private var checkingAt = 0L
    private var nextData = 0L
    private var nextLocation = 0L
    private var lastLocationSample = -1L
    private var lastLocationQuality = ""
    private var stopping = false
    private var failed = false
    private var message = "Connect your cluster to share maps"
    var state = "notConnected"; private set
    fun sharing(value: Boolean, map: MapWire.Region? = region) {
        region = map; wanted = value; failed = false
        if (!value) { stopping = true; pending = null; timeConfirm = null; fix = PhoneFix(problem = "stopped"); message = "Location sharing stopped" }
        else { stopping = false; message = "Preparing map sharing"; if (state == "failed") reconnectNeeded = true }
    }
    fun resetConnection() {
        caps = null; status = null; statusAt = 0; nextStatusRead = 0; capsDue = 0; initialized = false; subscribed = false; subscribeNeeded = false
        pending = null; sequence = 0; locationSequence = 0; generation = 0; offset = 0; anchored = 0; timeConfirm = null
        began = false; committed = false; active = false; checkingAt = 0; failed = false; reconnectNeeded = false
        lastLocationSample = -1; lastLocationQuality = ""; state = "notConnected"; message = "Waiting for cluster connection"
    }
    fun unavailable() { state = "unsupported"; message = "Map sharing needs compatible cluster firmware and MTU96"; initialized = true }
    fun fail(text: String) { failed = true; state = "failed"; message = text; pending = null; timeConfirm = null }
    fun capabilities(bytes: ByteArray?, now: Long) {
        val next = bytes?.let { MapWire.capabilities(it) }
        if (next == null) { capsDue = now + 2000; state = "waiting"; message = "Waiting for authenticated map capability"; return }
        if (caps != null && next.context != caps?.context) { reconnectNeeded = true; return }
        caps = next; initialized = true; capsDue = now + 2000
        if (!next.ready) { state = "waiting"; message = "Cluster map resources or return link unavailable"; return }
        if (region != null && (region!!.bytes.size > next.maxBytes || region!!.roads > next.maxRoads)) { fail("Area too detailed for this display"); return }
        subscribeNeeded = !subscribed
        if (!wanted) { state = "ready"; message = "Kolkata map ready to share" }
    }
    fun subscribed(success: Boolean) {
        subscribed = success; subscribeNeeded = false
        if (!success) fail("Map status subscription failed. Reconnect and retry.")
    }
    fun receive(bytes: ByteArray, now: Long) {
        val s = MapWire.status(bytes) ?: return
        val c = caps?.context ?: return
        if (s.context != c) {
            // A temporarily unavailable bridge returns no display ID. It cannot
            // authorize a new session or reuse a previous ACK.
            if (s.context.display != 0L) reconnectNeeded = true
            return
        }
        if (status != null && s.displayNow < status!!.displayNow) return
        status = s; statusAt = now
        val p = pending
        if (p != null && s.sequence == p.seq) {
            if (s.error != 0) { fail("Cluster rejected map operation (${s.error}). Retry sharing."); return }
            if (p.type in 3..6 && s.generation != generation) { fail("Map generation acknowledgement mismatch"); return }
            pending = null
            when (p.type) {
                1 -> {
                    val t0 = MapWire.get(p.bytes, 32, 8)
                    if (s.t0 != t0 || now < t0 || now - t0 > 500) { anchored = 0; nextData = now + 2000; message = "Timing delayed; retrying" }
                    else { timeConfirm = ByteArray(20).also { MapWire.put(it, 0, p.seq); MapWire.put(it, 4, t0, 8); MapWire.put(it, 12, now, 8) } }
                }
                2 -> { anchored = now; timeConfirm = null }
                3 -> {
                    val map = region ?: return
                    if (s.total != map.bytes.size || s.offset !in 0..map.bytes.size) { fail("Invalid map transfer acknowledgement"); return }
                    began = true; offset = s.offset; active = s.state == 4 && s.active == generation
                    if (!active && (s.state != 2 || s.offset != 0)) fail("Unexpected map transfer state")
                }
                4 -> { if (s.offset != p.expected) { fail("Unexpected accepted map offset"); return }; offset = s.offset; nextData = now + 67 }
                5 -> { committed = true; checkingAt = now }
                8 -> { stopping = false; anchored = 0; state = "stopped"; message = "Location sharing stopped" }
            }
        }
        if (s.state == 5 && s.generation == generation && began) { fail("Map update failed (${s.error}); previous roads retained"); return }
        if (committed && s.state == 4 && s.active == generation) active = true
        if (wanted && !failed) {
            state = if (active) "active" else if (s.state == 3) "checking" else if (began) "transferring" else "preparing"
            message = if (active) "Cluster confirmed map active" else if (s.state == 3) "Checking complete map…" else if (began) "Loading map" else "Preparing map sharing"
        }
    }
    fun writeResult(success: Boolean, now: Long) {
        if (!success && pending != null) { pending!!.deadline = now; pending!!.readFirst = true }
    }
    fun statusReadFailed() { if (pending != null) pending!!.readFirst = false }
    private fun command(type: Int, now: Long, body: ByteArray = byteArrayOf(), map: Boolean = false, expected: Int = 0): Request {
        sequence = (sequence + 1) and 0xffffffffL; if (sequence == 0L) sequence = 1
        val bytes = MapWire.command(caps!!.context, type, sequence, if (map) generation else 0, body)
        pending = Pending(type, sequence, bytes, expected, now, now + 2000)
        return Request("write", bytes)
    }
    fun next(now: Long, mtu: Int, endpoints: Boolean): Request? {
        if (!endpoints || mtu < 96) { unavailable(); return null }
        if (!initialized || (caps?.ready != true && now >= capsDue)) {
            capsDue = now + 2000
            // Until the capability callback, initialize as attempted and retry
            // with a bounded delay rather than a tight ATT read loop.
            initialized = true
            return Request("capability")
        }
        if (caps?.ready != true) return null
        if (subscribeNeeded) return Request("subscribe")
        if (!subscribed) return null
        if (statusAt == 0L) { if (now < nextStatusRead) return null; nextStatusRead = now + 500; return Request("status") }
        if (now - statusAt >= 2000) {
            if (now - statusAt >= 8000) { reconnectNeeded = true; return null }
            if (now < nextStatusRead) return null; nextStatusRead = now + 500; return Request("status")
        }
        if (stopping) {
            if (pending == null) return command(8, now)
        }
        if (!wanted && !stopping) return null
        val p = pending
        if (p != null && now >= p.deadline) {
            if (p.readFirst) { p.readFirst = false; return Request("status") }
            if (p.retries >= 3) { fail("Map acknowledgement timed out. Retry sharing."); return null }
            if (p.retryAt == 0L) p.retryAt = now + longArrayOf(500, 1000, 2000)[p.retries]
            if (now >= p.retryAt) { p.retries++; p.retryAt = 0; p.deadline = now + 2000; p.readFirst = true; return Request("write", p.bytes) }
        }
        val c = caps ?: return null
        if (wanted && now >= nextLocation && anchored != 0L && now - anchored < 30000) {
            val q = fix.quality(now)
            if (fix.sampled != lastLocationSample || q != lastLocationQuality) {
                lastLocationSample = fix.sampled; lastLocationQuality = q; nextLocation = now + 500
                locationSequence = (locationSequence + 1) and 0xffffffffL; if (locationSequence == 0L) locationSequence = 1
                return Request("location", MapWire.command(c.context, 7, locationSequence, body = fix.packet(now)))
            }
        }
        if (pending != null || status?.credit != true) return null
        if (timeConfirm != null) return command(2, now, timeConfirm!!)
        if ((anchored == 0L || now - anchored >= 25000) && now >= nextData) {
            return command(1, now, ByteArray(8).also { MapWire.put(it, 0, now, 8) })
        }
        if (failed || !wanted || region == null || anchored == 0L) return null
        val map = region!!
        if (!began) {
            if (map.bytes.size > c.maxBytes || map.roads > c.maxRoads) { fail("Area too detailed for this display"); return null }
            generation = if (generation == 0L) (now and 0xffffffffL).coerceAtLeast(1) else generation
            val body = ByteArray(36); MapWire.put(body, 0, map.bytes.size.toLong()); map.sha.copyInto(body, 4)
            return command(3, now, body, true)
        }
        if (offset < map.bytes.size && now >= nextData) {
            val count = minOf(240, mtu - 3, c.maxValue) - 40
            val until = minOf(map.bytes.size, offset + count)
            val body = ByteArray(4 + until - offset); MapWire.put(body, 0, offset.toLong()); map.bytes.copyInto(body, 4, offset, until)
            return command(4, now, body, true, until)
        }
        if (!active && !committed && offset == map.bytes.size) return command(5, now, map = true)
        if (committed && !active && now - checkingAt >= 15000) fail("Map validation timed out; previous roads retained")
        if (committed && !active && now - statusAt >= 500) return Request("status")
        return null
    }
    fun snapshot(now: Long): Map<String, Any?> = mapOf("state" to state, "message" to message, "sharing" to wanted,
        "acceptedBytes" to offset, "totalBytes" to (region?.bytes?.size ?: 0), "clusterActive" to (active && statusAt != 0L && now - statusAt < 2000),
        "clusterLocation" to status?.location?.takeIf { statusAt != 0L && now - statusAt < 2000 }, "location" to fix.snapshot(now, region))
}
