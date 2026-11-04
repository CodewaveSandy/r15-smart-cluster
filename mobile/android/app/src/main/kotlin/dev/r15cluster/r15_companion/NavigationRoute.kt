package dev.r15cluster.r15_companion

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import java.io.File
import java.net.URLEncoder
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import kotlin.math.*
import org.json.JSONObject

/** Private credential storage plus bounded provider operations; called only on IO worker. */
internal class NavigationProvider(private val context: Context) {
    private val keyFile = File(context.noBackupFilesDir, "navigation/provider.key")
    private val quota = context.getSharedPreferences("navigation-quota", Context.MODE_PRIVATE)
    private var lastRequest = 0L

    private fun key(): SecretKey {
        val ks = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
        return (ks.getKey("r15-navigation", null) as? SecretKey)
            ?: KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore")
                .apply {
                    init(
                        KeyGenParameterSpec.Builder(
                                "r15-navigation",
                                KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT,
                            )
                            .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                            .build()
                    )
                }
                .generateKey()
    }

    fun configured() = keyFile.exists()

    fun save(value: String) {
        if (value.isBlank()) {
            keyFile.delete()
            return
        }
        require(value.length in 12..256 && value.all { it.isLetterOrDigit() || it in "-_" }) {
            "Invalid API key format"
        }
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, key())
        VectorMaps.atomic(keyFile, cipher.iv + cipher.doFinal(value.toByteArray()))
    }

    private fun credential(): String {
        require(configured()) { "Add your Geoapify key in map settings" }
        val b = keyFile.readBytes()
        require(b.size in 29..512)
        val c = Cipher.getInstance("AES/GCM/NoPadding")
        c.init(Cipher.DECRYPT_MODE, key(), GCMParameterSpec(128, b.copyOfRange(0, 12)))
        return String(c.doFinal(b.copyOfRange(12, b.size)), Charsets.UTF_8)
    }

    private fun enc(s: String) = URLEncoder.encode(s, "UTF-8")

    private fun request(path: String, params: String, cost: Int = 1): JSONObject {
        val day = System.currentTimeMillis() / 86400000
        val used = if (quota.getLong("day", 0) == day) quota.getInt("used", 0) else 0
        require(used + cost <= 2500) { "Local daily provider budget reached; try tomorrow" }
        val wait = 500 - (android.os.SystemClock.elapsedRealtime() - lastRequest)
        if (wait > 0) Thread.sleep(wait)
        lastRequest = android.os.SystemClock.elapsedRealtime()
        quota.edit().putLong("day", day).putInt("used", used + cost).apply()
        return JSONObject(
            String(
                VectorMaps.http(
                    "https://api.geoapify.com/v1/$path?$params&apiKey=${enc(credential())}",
                    8 * 1024 * 1024,
                ),
                Charsets.UTF_8,
            )
        )
    }

    fun search(text: String, at: GeoPoint?): List<Map<String, Any>> {
        require(text.length in 3..200)
        val bias = at?.let { "&bias=proximity:${it.lon},${it.lat}" } ?: ""
        val items =
            request("geocode/autocomplete", "text=${enc(text)}&format=json&limit=5$bias")
                .getJSONArray("results")
        return (0 until minOf(items.length(), 5)).map {
            val j = items.getJSONObject(it)
            val p = GeoPoint(j.getDouble("lat"), j.getDouble("lon"))
            mapOf("label" to j.getString("formatted").take(300), "lat" to p.lat, "lon" to p.lon)
        }
    }

    fun route(
        origin: GeoPoint,
        destination: GeoPoint,
        label: String,
        revision: Int,
    ): NavigationRoute {
        val j =
            request(
                "routing",
                "waypoints=${enc("${origin.lat},${origin.lon}|${destination.lat},${destination.lon}")}&mode=motorcycle&details=instruction_details&lang=en",
                8,
            )
        val feature = j.getJSONArray("features").getJSONObject(0)
        val properties = feature.getJSONObject("properties")
        val geometry = feature.getJSONObject("geometry")
        require(geometry.getString("type") == "MultiLineString")
        val legs = geometry.getJSONArray("coordinates")
        require(legs.length() == 1)
        val line = legs.getJSONArray(0)
        require(line.length() in 2..50000)
        val points =
            (0 until line.length()).map {
                val c = line.getJSONArray(it)
                GeoPoint(c.getDouble(1), c.getDouble(0))
            }
        val distance = properties.getDouble("distance").roundToInt()
        val seconds = properties.getDouble("time").roundToInt()
        require(distance in 1..100000000 && seconds in 0..31536000)
        val steps = properties.getJSONArray("legs").getJSONObject(0).getJSONArray("steps")
        require(steps.length() <= 10000)
        val parsed =
            (0 until steps.length()).map {
                val s = steps.getJSONObject(it)
                val index = s.getInt("from_index")
                require(index in points.indices)
                val instruction = s.optJSONObject("instruction")
                NavigationRoute.Step(
                    index,
                    instruction?.optString("text", "")?.take(500) ?: "Continue",
                    instruction?.optString("type", "None") ?: "None",
                )
            }
        return NavigationRoute(
            (java.security.SecureRandom().nextLong() and Long.MAX_VALUE).coerceAtLeast(1),
            revision,
            points,
            distance,
            seconds,
            destination,
            label.take(300),
            parsed,
        )
    }
}

internal class NavigationRoute(
    val id: Long,
    val revision: Int,
    val points: List<GeoPoint>,
    val distance: Int,
    val seconds: Int,
    val destination: GeoPoint,
    val label: String,
    val steps: List<Step>,
) {
    data class Step(val index: Int, val text: String, val type: String)

    val along = DoubleArray(points.size)

    init {
        require(
            id > 0 &&
                revision > 0 &&
                points.size in 2..50000 &&
                distance in 1..100000000 &&
                seconds in 0..31536000
        )
        require(points.zipWithNext().all { abs(it.first.lon - it.second.lon) < 180 }) {
            "Antimeridian routes are not supported by this conversion profile"
        }
        for (i in 1 until points.size) along[i] = along[i - 1] + points[i - 1].distance(points[i])
        require(along.last() > 0)
    }

    data class Projection(val along: Double, val error: Double, val index: Int)

    fun project(p: GeoPoint, previous: Double? = null): Projection {
        var result = Projection(0.0, Double.MAX_VALUE, 0)
        val scale = cos(p.lat * PI / 180)
        for (i in 1 until points.size) {
            val a = points[i - 1]
            val b = points[i]
            val ax = (a.lon - p.lon) * 111195 * scale
            val ay = (a.lat - p.lat) * 111195
            val dx = (b.lon - a.lon) * 111195 * scale
            val dy = (b.lat - a.lat) * 111195
            val t =
                if (dx * dx + dy * dy > 0)
                    (-(ax * dx + ay * dy) / (dx * dx + dy * dy)).coerceIn(0.0, 1.0)
                else 0.0
            val e = hypot(ax + dx * t, ay + dy * t)
            val candidate = along[i - 1] + (along[i] - along[i - 1]) * t
            val penalty =
                if (previous != null && candidate < previous - 50) 1000.0
                else if (previous != null && candidate > previous + 1000) 100.0 else 0.0
            val oldPenalty =
                if (previous != null && result.along < previous - 50) 1000.0
                else if (previous != null && result.along > previous + 1000) 100.0 else 0.0
            if (e + penalty < result.error + oldPenalty) result = Projection(candidate, e, i - 1)
        }
        return result
    }

    fun metres(at: Double) = (at / along.last() * distance).roundToInt().coerceIn(0, distance)

    fun nextStep(p: Projection) =
        steps.firstOrNull { along[it.index] > p.along + 8 }
            ?: Step(points.lastIndex, "Arrive at destination", "DestinationReached")

    fun maneuver(step: Step): Int =
        when (step.type) {
            "DestinationReached",
            "DestinationReachedRight",
            "DestinationReachedLeft" -> 4
            "SlightRight",
            "Right",
            "SharpRight",
            "ExitRight" -> 2
            "SharpLeft",
            "Left",
            "SlightLeft",
            "ExitLeft" -> 1
            "TurnAroundRight",
            "TurnAroundLeft" -> 7
            "Straight" -> 3
            "StayLeft",
            "MergeLeft" -> 5
            "StayRight",
            "MergeRight" -> 6
            else -> 0
        }

    fun corridor(z: Int, width: Double = 500.0): Set<MapCell> {
        val cells = LinkedHashSet<MapCell>()
        for (i in 1 until points.size) {
            val a = points[i - 1]
            val b = points[i]
            val samples = ceil(a.distance(b) / 250).toInt().coerceAtLeast(1)
            require(samples <= 10000) {
                "Route too large for offline allowance; choose coarser detail"
            }
            for (j in 0..samples) {
                val t = j.toDouble() / samples
                val p = GeoPoint(a.lat + (b.lat - a.lat) * t, a.lon + (b.lon - a.lon) * t)
                val dy = width / 111195
                val dx = dy / cos(p.lat * PI / 180).coerceAtLeast(.087)
                val nw =
                    MapCell.at(
                        GeoPoint(
                            (p.lat + dy).coerceAtMost(85.0511287),
                            (p.lon - dx).coerceAtLeast(-180.0),
                        ),
                        z,
                    )
                val se =
                    MapCell.at(
                        GeoPoint(
                            (p.lat - dy).coerceAtLeast(-85.0511287),
                            (p.lon + dx).coerceAtMost(180.0),
                        ),
                        z,
                    )
                for (x in nw.x..se.x) for (y in nw.y..se.y) {
                    cells.add(MapCell(z, x, y))
                    require(cells.size <= 3000) {
                        "Route exceeds 3000-tile offline allowance; explicitly choose coarser detail"
                    }
                }
            }
        }
        return cells
    }

    private fun simplify(input: List<GeoPoint>, tolerance: Double): List<GeoPoint> {
        if (input.size <= 2) return input
        val keep = BooleanArray(input.size)
        keep[0] = true
        keep[input.lastIndex] = true
        val stack = java.util.ArrayDeque<Pair<Int, Int>>()
        stack.add(0 to input.lastIndex)
        var work = 0
        while (stack.isNotEmpty()) {
            val (a, b) = stack.removeLast()
            val same = input[a].distance(input[b]) < 0.01
            val mini =
                if (same) null
                else
                    NavigationRoute(
                        1,
                        1,
                        listOf(input[a], input[b]),
                        1,
                        0,
                        input[b],
                        "",
                        emptyList(),
                    )
            var maxError = 0.0
            var index = -1
            for (i in a + 1 until b) {
                require(++work <= 3000000) { "Route geometry too complex for bounded conversion" }
                val error = mini?.project(input[i])?.error ?: input[a].distance(input[i])
                if (error > maxError) {
                    maxError = error
                    index = i
                }
            }
            if (maxError > tolerance && index >= 0) {
                keep[index] = true
                stack.add(a to index)
                stack.add(index to b)
            }
        }
        return input.filterIndexed { i, _ -> keep[i] }
    }

    fun overview(source: ByteArray): NavigationWire.Object {
        val anchors = (listOf(0, points.lastIndex) + steps.map { it.index }).distinct().sorted()
        require(anchors.size <= 512) { "Route has too many maneuvers for the display overview" }
        fun reduced(tolerance: Double): List<GeoPoint> {
            val result = ArrayList<GeoPoint>()
            for (i in 1 until anchors.size) {
                val part = simplify(points.subList(anchors[i - 1], anchors[i] + 1), tolerance)
                if (result.isEmpty()) result.addAll(part) else result.addAll(part.drop(1))
            }
            return result
        }
        var tolerance = 2.0
        var p = reduced(tolerance)
        while (p.size > 512 && tolerance < 32) {
            tolerance *= 2
            p = reduced(tolerance)
        }
        require(p.size <= 512) {
            "Route overview exceeds 512 points at 32 m error; choose a shorter route"
        }
        return objectFor(source, p, 2, 0, distance)
    }

    fun window(source: ByteArray, atMetres: Int): NavigationWire.Object {
        val target = atMetres.toDouble() / distance * along.last()
        val center =
            (points.indices.lastOrNull { along[it] <= target } ?: 0).coerceAtMost(
                points.lastIndex - 1
            )
        val start =
            max(
                    (points.indices.lastOrNull { along[it] <= max(0.0, target - 200) } ?: 0),
                    center - 128,
                )
                .coerceAtMost(points.lastIndex - 1)
        var end =
            (points.indices.firstOrNull { along[it] >= target + 4000 } ?: points.lastIndex)
                .coerceAtLeast(center + 1)
        fun reduced(last: Int): List<GeoPoint> {
            val anchors =
                (listOf(start, last) + steps.map { it.index }.filter { it in start..last })
                    .distinct()
                    .sorted()
            val result = ArrayList<GeoPoint>()
            for (i in 1 until anchors.size) {
                val p = simplify(points.subList(anchors[i - 1], anchors[i] + 1), 1.0)
                if (result.isEmpty()) result.addAll(p) else result.addAll(p.drop(1))
            }
            return result
        }
        var p = reduced(end)
        while (p.size > 512 && end > center + 1) {
            end = center + max(1, (end - center) / 2)
            p = reduced(end)
        }
        require(p.size <= 512) { "Detailed route window exceeds geometry limit" }
        return objectFor(
            source,
            p,
            3,
            metres(along[start]),
            metres(along[end]).coerceAtLeast(metres(along[start]) + 1).coerceAtMost(distance),
        )
    }

    private fun objectFor(
        source: ByteArray,
        p: List<GeoPoint>,
        kind: Int,
        start: Int,
        end: Int,
    ): NavigationWire.Object {
        require(p.size in 2..512 && start < end)
        val b = NavigationWire.buffer(p.size * 8)
        p.forEach { b.putInt((it.lat * 1e7).roundToInt()).putInt((it.lon * 1e7).roundToInt()) }
        return NavigationWire.obj(
            source,
            kind,
            "route/$id/$revision/$kind/$start/$end",
            p.size,
            b.array(),
            route = id,
            revision = revision,
            start = start,
            end = end,
            distance = distance,
            seconds = seconds,
        )
    }

    fun json(): JSONObject =
        JSONObject()
            .put("id", id)
            .put("revision", revision)
            .put("points", org.json.JSONArray(points.map { it.json() }))
            .put("distance", distance)
            .put("seconds", seconds)
            .put("label", label)
            .put("destination", org.json.JSONArray(destination.json()))
            .put("steps", org.json.JSONArray(steps.map { listOf(it.index, it.text, it.type) }))

    companion object {
        fun restore(j: JSONObject): NavigationRoute {
            val a = j.getJSONArray("points")
            require(a.length() in 2..50000)
            val p =
                (0 until a.length()).map {
                    val q = a.getJSONArray(it)
                    GeoPoint(q.getDouble(1), q.getDouble(0))
                }
            val s = j.getJSONArray("steps")
            require(s.length() <= 10000)
            val steps =
                (0 until s.length()).map {
                    val q = s.getJSONArray(it)
                    val index = q.getInt(0)
                    require(index in p.indices)
                    Step(index, q.getString(1).take(500), q.getString(2))
                }
            val d = j.getJSONArray("destination")
            val distance = j.getInt("distance")
            val seconds = j.getInt("seconds")
            require(distance in 1..100000000 && seconds in 0..31536000)
            return NavigationRoute(
                j.getLong("id"),
                j.getInt("revision"),
                p,
                distance,
                seconds,
                GeoPoint(d.getDouble(1), d.getDouble(0)),
                j.getString("label").take(300),
                steps,
            )
        }
    }
}
