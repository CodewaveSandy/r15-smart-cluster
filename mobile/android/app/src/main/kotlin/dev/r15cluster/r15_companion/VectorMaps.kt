package dev.r15cluster.r15_companion

import android.content.Context
import android.util.AtomicFile
import java.io.*
import java.net.URL
import java.util.zip.GZIPInputStream
import javax.net.ssl.HttpsURLConnection
import kotlin.math.*
import org.json.JSONObject

/** Only network owner for map data. Raw MVT feeds both local phone GeoJSON and R15O. */
internal class VectorMaps(private val context: Context) {
    private val root = File(context.noBackupFilesDir, "navigation/maps").apply { mkdirs() }
    private var template = ""
    var maxZoom = 0
        private set

    private var detailZoom =
        context
            .getSharedPreferences("navigation", Context.MODE_PRIVATE)
            .getInt("detailZoom", 14)
            .coerceIn(8, 14)
    var source = byteArrayOf()
        private set

    var hits = 0
        private set

    var misses = 0
        private set

    var received = 0L
        private set

    private var pinned = emptySet<String>()
    private var working = emptySet<String>()
    private var visibleKeys = emptySet<String>()
    private val decoded =
        object : LinkedHashMap<String, List<Road>>(12, .75f, true) {
            override fun removeEldestEntry(e: MutableMap.MutableEntry<String, List<Road>>?) =
                size > 12
        }

    data class Road(val kind: Int, val name: String, val points: List<Pair<Double, Double>>)

    companion object {
        const val TILE_MAX = 2 * 1024 * 1024
        const val JOB_MAX = 50L * 1024 * 1024

        fun http(url: String, max: Int): ByteArray {
            val uri = URL(url)
            require(
                uri.protocol == "https" &&
                    uri.host in setOf("tiles.openfreemap.org", "api.geoapify.com")
            )
            val c = uri.openConnection() as HttpsURLConnection
            try {
                c.connectTimeout = 12000
                c.readTimeout = 20000
                c.instanceFollowRedirects = false
                c.setRequestProperty(
                    "User-Agent",
                    "R15ClusterCompanion/2.0 (personal navigation; github.com/codewavesandy/r15-smart-cluster)",
                )
                c.setRequestProperty("Accept-Encoding", "gzip")
                val status = c.responseCode
                if (status == 429) error("Provider quota reached; retry later")
                if (status == 401 || status == 403)
                    error("Provider access refused; check your API key and restrictions")
                if (status != 200) error("Provider unavailable (HTTP $status)")
                require(c.contentLengthLong <= max || c.contentLengthLong < 0) {
                    "Provider response exceeds limit"
                }
                val raw = c.inputStream
                val stream = if (c.contentEncoding == "gzip") GZIPInputStream(raw) else raw
                return stream.use { input ->
                    val out = ByteArrayOutputStream()
                    val buf = ByteArray(8192)
                    while (true) {
                        if (Thread.currentThread().isInterrupted) throw InterruptedIOException()
                        val n = input.read(buf)
                        if (n < 0) break
                        require(out.size() + n <= max) { "Provider response exceeds limit" }
                        out.write(buf, 0, n)
                    }
                    out.toByteArray()
                }
            } finally {
                c.disconnect()
            }
        }

        fun atomic(file: File, bytes: ByteArray) {
            file.parentFile?.mkdirs()
            val a = AtomicFile(file)
            val out = a.startWrite()
            try {
                out.write(bytes)
                a.finishWrite(out)
            } catch (e: Exception) {
                a.failWrite(out)
                throw e
            }
        }
    }

    @Synchronized
    fun initialize() {
        if (source.isNotEmpty()) return
        val file = File(root, "source.json")
        val data =
            if (file.exists()) {
                require(file.length() <= 256 * 1024)
                file.readBytes()
            } else
                http("https://tiles.openfreemap.org/planet", 256 * 1024).also { atomic(file, it) }
        val j = JSONObject(String(data, Charsets.UTF_8))
        val t = j.getJSONArray("tiles").getString(0)
        require(
            t.startsWith("https://tiles.openfreemap.org/planet/") &&
                t.contains("{z}") &&
                t.contains("{x}") &&
                t.contains("{y}")
        )
        maxZoom = j.getInt("maxzoom")
        require(maxZoom in 0..22)
        template = t
        updateDigest()
    }

    private fun updateDigest() {
        source =
            NavigationWire.sha(
                ("R15MVT/2;roads-v1;style1;attr2;lod2;detail=$detailZoom;max=$maxZoom;" + template)
                    .toByteArray()
            )
    }

    @Synchronized
    fun detail(zoom: Int) {
        initialize()
        detailZoom = zoom.coerceIn(8, 14)
        updateDigest()
        context
            .getSharedPreferences("navigation", Context.MODE_PRIVATE)
            .edit()
            .putInt("detailZoom", detailZoom)
            .apply()
    }

    private fun upstream(c: MapCell) = c.parent(minOf(maxZoom, detailZoom))

    private fun file(c: MapCell) = File(root, "${c.z}_${c.x}_${c.y}.mvt")

    @Synchronized
    fun current(cells: List<MapCell>) {
        working = cells.map { upstream(it).key }.toSet()
    }

    @Synchronized
    fun view(cells: List<MapCell>) {
        visibleKeys = cells.map { upstream(it).key }.toSet()
    }

    @Synchronized
    fun pin(cells: Set<MapCell>) {
        pinned = cells.map { upstream(it).key }.toSet()
    }

    @Synchronized fun size(c: MapCell) = file(upstream(c)).length()

    @Synchronized fun bytesOnDisk() = root.listFiles()?.sumOf { it.length() } ?: 0L

    @Synchronized
    fun clear() {
        require(pinned.isEmpty()) { "End navigation before clearing maps" }
        root.listFiles()?.filter { it.extension in listOf("mvt", "sha") }?.forEach { it.delete() }
        decoded.clear()
        File(root, "source.json").delete()
        source = byteArrayOf()
        template = ""
        maxZoom = 0
    }

    @Synchronized
    fun cached(c: MapCell): Boolean {
        val f = file(upstream(c))
        val h = File(f.path + ".sha")
        return try {
            f.isFile &&
                f.length() <= TILE_MAX &&
                h.isFile &&
                h.length() == 32L &&
                NavigationWire.sha(f.readBytes()).contentEquals(h.readBytes())
        } catch (_: Exception) {
            false
        }
    }

    @Synchronized
    fun roads(cell: MapCell, allowNetwork: Boolean = true): List<Road> {
        initialize()
        val c = upstream(cell)
        if (decoded.containsKey(c.key)) hits++
        val base =
            decoded[c.key]
                ?: run {
                    val f = file(c)
                    var data: ByteArray? = null
                    if (f.exists()) {
                        try {
                            val raw = f.readBytes()
                            require(raw.size <= TILE_MAX && cached(c))
                            Mvt.decode(raw)
                            data = raw
                            hits++
                            f.setLastModified(System.currentTimeMillis())
                        } catch (_: Exception) {
                            f.delete()
                        }
                    }
                    if (data == null) {
                        require(allowNetwork) { "Map not cached; connect to the internet" }
                        val fetched =
                            http(
                                template
                                    .replace("{z}", "${c.z}")
                                    .replace("{x}", "${c.x}")
                                    .replace("{y}", "${c.y}"),
                                TILE_MAX,
                            )
                        val parsed = Mvt.decode(fetched)
                        evict(fetched.size.toLong())
                        atomic(f, fetched)
                        atomic(File(f.path + ".sha"), NavigationWire.sha(fetched))
                        misses++
                        received += fetched.size
                        decoded[c.key] = parsed
                        return@run parsed
                    }
                    Mvt.decode(requireNotNull(data)).also { decoded[c.key] = it }
                }
        val scale = (1 shl (cell.z - c.z)).toDouble()
        val dx = cell.x - c.x * scale
        val dy = cell.y - c.y * scale
        return base.mapNotNull { road ->
            val parts = road.points.map { Pair(it.first * scale - dx, it.second * scale - dy) }
            if (parts.size < 2) null else road.copy(points = parts)
        }
    }

    private fun evict(extra: Long) {
        var size = bytesOnDisk()
        val limit = 256L * 1024 * 1024
        root
            .listFiles()
            ?.filter {
                it.extension == "mvt" &&
                    it.nameWithoutExtension.replace('_', '/') !in (pinned + working + visibleKeys)
            }
            ?.sortedBy { it.lastModified() }
            ?.forEach {
                if (size + extra > limit) {
                    val n = it.length()
                    if (it.delete()) {
                        size -= n
                        File(it.path + ".sha").delete()
                    }
                }
            }
        require(size + extra <= limit && root.usableSpace > extra + 8 * 1024 * 1024) {
            "Map cache or disk full; end navigation and clear cached maps"
        }
    }

    fun geoJson(cells: List<MapCell>, budget: Long = 8L * 1024 * 1024): Map<String, Any> {
        val features = ArrayList<Map<String, Any>>()
        var vertices = 0
        val before = received
        for (c in cells) {
            require(cached(c) || received - before + TILE_MAX <= budget) {
                "Nearby map download budget reached; zoom in"
            }
            for (r in roads(c)) {
                for (line in clip(r.points)) {
                    vertices += line.size
                    require(features.size < 20000 && vertices <= 200000) {
                        "Viewport too dense; zoom in"
                    }
                    features.add(
                        mapOf(
                            "type" to "Feature",
                            "properties" to mapOf("kind" to r.kind, "name" to r.name),
                            "geometry" to
                                mapOf(
                                    "type" to "LineString",
                                    "coordinates" to
                                        line.map { c.point(it.first, it.second).json() },
                                ),
                        )
                    )
                }
            }
        }
        return mapOf("type" to "FeatureCollection", "features" to features)
    }

    fun cell(c: MapCell): NavigationWire.Object {
        val records = ArrayList<ByteArray>()
        for (r in roads(c)) {
            // Fixed semantic LOD profile, never a record-count truncation.
            if (c.z < 12 && r.kind !in listOf(1, 2)) continue
            if (c.z < 15 && r.kind in listOf(3, 4)) continue
            for (line in clip(r.points)) {
                val quant =
                    line
                        .map {
                            Pair(
                                (it.first * 4096).roundToInt().coerceIn(0, 4096),
                                (it.second * 4096).roundToInt().coerceIn(0, 4096),
                            )
                        }
                        .fold(ArrayList<Pair<Int, Int>>()) { a, p ->
                            if (a.lastOrNull() != p) a.add(p)
                            a
                        }
                var at = 0
                while (at < quant.size - 1) {
                    val pts = quant.subList(at, minOf(at + 8, quant.size))
                    at += pts.size - 1
                    if (pts.distinct().size < 2) continue
                    val name =
                        r.name
                            .filter { it.code in 32..126 }
                            .trim()
                            .take(23)
                            .toByteArray(Charsets.US_ASCII)
                    val b =
                        NavigationWire.buffer(4 + name.size + pts.size * 4)
                            .put(r.kind.toByte())
                            .put(pts.size.toByte())
                            .put(name.size.toByte())
                            .put(0)
                            .put(name)
                    pts.forEach { b.putShort(it.first.toShort()).putShort(it.second.toShort()) }
                    records.add(b.array())
                }
            }
        }
        require(records.size <= 512 && records.sumOf { it.size } + 64 <= 32768) {
            "Display cell too dense; zoom the display in for complete coverage"
        }
        val body = ByteArrayOutputStream()
        records.forEach { body.write(it) }
        return NavigationWire.obj(source, 1, c.key, records.size, body.toByteArray(), cell = c)
    }

    /** Liang–Barsky clips actual road segments, preserving every intersection and split. */
    private fun clip(points: List<Pair<Double, Double>>): List<List<Pair<Double, Double>>> {
        val out = ArrayList<List<Pair<Double, Double>>>()
        var line = ArrayList<Pair<Double, Double>>()
        fun flush() {
            if (line.size >= 2) out.add(line)
            line = ArrayList()
        }
        for (i in 1 until points.size) {
            val a = points[i - 1]
            val b = points[i]
            val dx = b.first - a.first
            val dy = b.second - a.second
            var lo = 0.0
            var hi = 1.0
            var ok = true
            for ((p, q) in
                listOf(
                    -dx to a.first,
                    dx to (1 - a.first),
                    -dy to a.second,
                    dy to (1 - a.second),
                )) {
                if (abs(p) < 1e-15) {
                    if (q < 0) ok = false
                } else {
                    val t = q / p
                    if (p < 0) lo = max(lo, t) else hi = min(hi, t)
                }
            }
            if (!ok || lo > hi) {
                flush()
                continue
            }
            val s = Pair(a.first + lo * dx, a.second + lo * dy)
            val e = Pair(a.first + hi * dx, a.second + hi * dy)
            if (line.lastOrNull() != s) flush()
            if (line.isEmpty()) line.add(s)
            if (line.last() != e) line.add(e)
            if (hi < 1) flush()
        }
        flush()
        return out
    }
}

/** Bounded MVT protobuf/geometry reader; rejects corrupt data before cache publication. */
private object Mvt {
    private class Pb(val bytes: ByteArray) {
        var at = 0

        fun n(): Long {
            var n = 0L
            for (i in 0..9) {
                require(at < bytes.size)
                val b = bytes[at++].toInt() and 255
                require(i < 9 || b <= 1)
                n = n or ((b and 127).toLong() shl (7 * i))
                if (b and 128 == 0) return n
            }
            error("Invalid protobuf")
        }

        fun data(): ByteArray {
            val n = n()
            require(n in 0..(bytes.size - at).toLong())
            return bytes.copyOfRange(at, at + n.toInt()).also { at += n.toInt() }
        }

        fun skip(w: Int) {
            when (w) {
                0 -> n()
                1 -> {
                    require(at + 8 <= bytes.size)
                    at += 8
                }
                2 -> data()
                5 -> {
                    require(at + 4 <= bytes.size)
                    at += 4
                }
                else -> error("Invalid protobuf wire type")
            }
        }

        fun fields(read: (Int, Int) -> Unit) {
            var count = 0
            while (at < bytes.size) {
                require(++count <= 200000)
                val k = n().toInt()
                require(k > 0)
                read(k ushr 3, k and 7)
            }
        }
    }

    fun decode(raw: ByteArray): List<VectorMaps.Road> {
        val bytes =
            if (raw.size >= 2 && raw[0] == 0x1f.toByte() && raw[1] == 0x8b.toByte())
                GZIPInputStream(ByteArrayInputStream(raw)).use { s ->
                    val out = ByteArrayOutputStream()
                    val b = ByteArray(8192)
                    while (true) {
                        val n = s.read(b)
                        if (n < 0) break
                        require(out.size() + n <= 8 * 1024 * 1024)
                        out.write(b, 0, n)
                    }
                    out.toByteArray()
                }
            else raw
        require(bytes.size <= 8 * 1024 * 1024)
        val out = ArrayList<VectorMaps.Road>()
        val p = Pb(bytes)
        var layers = 0
        val budget = intArrayOf(0)
        p.fields { f, w ->
            if (f == 3 && w == 2) {
                require(++layers <= 64)
                layer(p.data(), out, budget)
            } else p.skip(w)
        }
        return out
    }

    private fun layer(raw: ByteArray, out: MutableList<VectorMaps.Road>, budget: IntArray) {
        val p = Pb(raw)
        var name = ""
        var extent = 4096
        val keys = ArrayList<String>()
        val values = ArrayList<String>()
        val features = ArrayList<ByteArray>()
        p.fields { f, w ->
            when {
                f == 1 && w == 2 -> name = String(p.data(), Charsets.UTF_8)
                f == 2 && w == 2 -> {
                    require(features.size < 50000)
                    features.add(p.data())
                }
                f == 3 && w == 2 -> {
                    require(keys.size < 4096)
                    keys.add(String(p.data(), Charsets.UTF_8))
                }
                f == 4 && w == 2 -> {
                    val v = Pb(p.data())
                    var s = ""
                    v.fields { vf, vw ->
                        if (vf == 1 && vw == 2) s = String(v.data(), Charsets.UTF_8).take(256)
                        else v.skip(vw)
                    }
                    values.add(s)
                }
                f == 5 && w == 0 -> extent = p.n().toInt()
                else -> p.skip(w)
            }
        }
        require(extent in 1..65536)
        if (name !in setOf("transportation", "transportation_name", "waterway")) return
        for (rawFeature in features) {
            val q = Pb(rawFeature)
            var type = 0
            var tags = byteArrayOf()
            var geom = byteArrayOf()
            q.fields { f, w ->
                when {
                    f == 2 && w == 2 -> tags = q.data()
                    f == 3 && w == 0 -> type = q.n().toInt()
                    f == 4 && w == 2 -> geom = q.data()
                    else -> q.skip(w)
                }
            }
            if (type != 2) continue
            val props = HashMap<String, String>()
            val t = Pb(tags)
            while (t.at < tags.size) {
                val k = t.n().toInt()
                val v = t.n().toInt()
                require(k in keys.indices && v in values.indices)
                props[keys[k]] = values[v]
            }
            val kind =
                if (name == "waterway") 2
                else
                    when (props["class"]) {
                        "motorway",
                        "trunk",
                        "primary",
                        "secondary",
                        "tertiary" -> 1
                        "path",
                        "track",
                        "pedestrian",
                        "footway",
                        "cycleway",
                        "steps" -> 3
                        "service" -> 4
                        else -> 0
                    }
            val label = props["name:en"] ?: props["name"] ?: props["ref"] ?: ""
            val g = Pb(geom)
            var x = 0L
            var y = 0L
            var commands = 0
            var line = ArrayList<Pair<Double, Double>>()
            fun flush() {
                if (line.size >= 2) {
                    require(out.size < 50000)
                    out.add(VectorMaps.Road(kind, label, line))
                }
                line = ArrayList()
            }
            while (g.at < geom.size) {
                val cmd = g.n()
                val id = (cmd and 7).toInt()
                val count = (cmd ushr 3).toInt()
                require(count in 1..100000 && commands + count <= 200000)
                commands += count
                require(id == 1 || id == 2 || id == 7)
                if (id == 7) {
                    require(count == 1)
                    flush()
                    continue
                }
                repeat(count) {
                    require(++budget[0] <= 100000) { "Vector tile exceeds geometry budget" }
                    if (id == 1) flush()
                    val a = g.n()
                    val b = g.n()
                    x += (a ushr 1) xor -(a and 1)
                    y += (b ushr 1) xor -(b and 1)
                    require(abs(x) <= extent * 16L && abs(y) <= extent * 16L)
                    line.add(Pair(x.toDouble() / extent, y.toDouble() / extent))
                }
            }
            flush()
        }
    }
}
