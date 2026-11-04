package dev.r15cluster.r15_companion

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import java.io.File
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicInteger
import kotlin.math.*

/** Native lifetime owns location/route/corridor work; no Activity, Flutter engine or GATT. */
internal class NavigationSession(
    private val context: Context,
    private val wire: NavigationSync,
    private val changed: () -> Unit,
) {
    private val main = Handler(Looper.getMainLooper())
    private val io = Executors.newSingleThreadExecutor()
    private val progressWorker = Executors.newSingleThreadExecutor()
    private val maps = VectorMaps(context)
    private val provider = NavigationProvider(context)
    private val routeFile = File(context.noBackupFilesDir, "navigation/route.json")
    private val consent = context.getSharedPreferences("navigation", Context.MODE_PRIVATE)
    private val searchGeneration = AtomicInteger()
    private val routeGeneration = AtomicInteger()
    private val corridorGeneration = AtomicInteger()
    @Volatile private var dead = false
    var visible = false
        private set

    var navigating = false
        private set

    var sharing = false
        private set

    var fix = PhoneFix()
        private set

    private var route: NavigationRoute? = null
    private var routeRevision = 0
    private var projection: NavigationRoute.Projection? = null
    private var projectionSample = 0L
    private var suggestions = emptyList<Map<String, Any>>()
    private var searching = false
    private var routing = false
    private var message: String? = null
    private var mapError: String? = null
    private var mapRevision = 0
    private var geo: Map<String, Any> =
        mapOf("type" to "FeatureCollection", "features" to emptyList<Any>())
    private var viewport: List<MapCell> = emptyList()
    @Volatile private var viewportGeneration = 0
    private var viewportLoading = false
    private var viewportPending: List<MapCell>? = null
    private var interestPending: List<MapCell>? = null
    private var interestLoading = false
    private var currentKey = ""
    private var warmKey = ""
    private var warmBusy = false
    private var warmAt = 0L
    private var fixWork = false
    private var lastFixWork = 0L
    private var lastProgressSample = 0L
    private var windowPending: Int? = null
    private var windowLoading = false
    private var contextPending: MapCell? = null
    private var contextLoading = false
    private var offSince = 0L
    private var offSamples = 0
    private var rerouteAt = 0L
    private var cacheReady = false
    private var cacheDone = 0
    private var cacheTotal = 0
    private var cacheBytes = 0L
    private var cacheBusy = false
    private var cacheError: String? = null
    private var downloadZoom =
        context
            .getSharedPreferences("navigation", Context.MODE_PRIVATE)
            .getInt("detailZoom", 14)
            .coerceIn(8, 14)
    @Volatile private var generation = 0
    private var configured = provider.configured()
    val allowed
        get() = consent.getBoolean("consent", false)

    init {
        wire.onInterest = { if (sharing) interests(it) }
        wire.onWindow = { at -> prepareWindow(at) }
        wire.onSession = {
            if (sharing) {
                currentKey = ""
                prepareContext()
            }
        }
        io.execute {
            val saved =
                try {
                    if (
                        routeFile.exists() &&
                            routeFile.length() <= 8 * 1024 * 1024 &&
                            File(routeFile.path + ".sha").isFile &&
                            File(routeFile.path + ".sha").length() == 32L &&
                            NavigationWire.sha(routeFile.readBytes())
                                .contentEquals(File(routeFile.path + ".sha").readBytes())
                    )
                        NavigationRoute.restore(org.json.JSONObject(routeFile.readText()))
                    else null
                } catch (_: Exception) {
                    null
                }
            main.post {
                if (!dead && route == null && saved != null) {
                    route = saved
                    routeRevision = saved.revision
                    mapRevision++
                    changed()
                }
            }
        }
    }

    fun accept() {
        consent.edit().putBoolean("consent", true).apply()
        changed()
    }

    fun setVisible(value: Boolean) {
        visible = value
        changed()
    }

    fun setKey(value: String) {
        io.execute {
            try {
                provider.save(value)
                main.post {
                    if (!dead) {
                        configured = provider.configured()
                        message = null
                        changed()
                    }
                }
            } catch (_: Exception) {
                publishError("Could not save protected provider key")
            }
        }
    }

    private fun publishError(text: String) {
        main.post {
            if (!dead) {
                message = text
                changed()
            }
        }
    }

    fun onFix(value: PhoneFix) {
        fix = value
        wire.fix = value
        val now = SystemClock.elapsedRealtime()
        if (!allowed) return
        if (NavigationWire.eligible(value, now)) {
            if (!warmBusy && now - warmAt >= 3000) warm(GeoPoint(value.lat, value.lon))
            if (viewport.isEmpty()) camera(value.lat, value.lon, 15.0)
            if (sharing) prepareContext()
            if (
                !fixWork &&
                    value.sampled > lastProgressSample &&
                    now - lastFixWork >= 500 &&
                    route != null &&
                    navigating &&
                    NavigationWire.precise(value, now)
            ) {
                fixWork = true
                lastFixWork = now
                lastProgressSample = value.sampled
                val r = route!!
                val f = value
                val g = generation
                progressWorker.execute {
                    val p = r.project(GeoPoint(f.lat, f.lon), projection?.along)
                    main.post {
                        fixWork = false
                        if (
                            !dead &&
                                g == generation &&
                                route === r &&
                                navigating &&
                                f.sampled == fix.sampled
                        ) {
                            updateProgress(r, p, f)
                            changed()
                        }
                    }
                }
            }
        } else {
            wire.progress = null
            offSince = 0
            offSamples = 0
        }
        changed()
    }

    private fun warm(p: GeoPoint) {
        warmBusy = true
        warmAt = SystemClock.elapsedRealtime()
        io.execute {
            try {
                maps.initialize()
                val c = MapCell.at(p, minOf(14, maps.maxZoom))
                val key = c.key + maps.source.take(8).toString()
                if (key == warmKey) {
                    main.post { warmBusy = false }
                    return@execute
                }
                warmKey = key
                val n = 1 shl c.z
                val cells = ArrayList<MapCell>()
                for (dy in -1..1) for (dx in -1..1) if (c.y + dy in 0 until n)
                    cells.add(MapCell(c.z, (c.x + dx + n) % n, c.y + dy))
                maps.current(cells)
                warmNext(cells.sortedBy { abs(it.x - c.x) + abs(it.y - c.y) }, 0, maps.received)
            } catch (_: Exception) {
                warmKey = ""
                main.post { warmBusy = false }
            }
        }
    }

    private fun warmNext(cells: List<MapCell>, at: Int, before: Long) {
        if (dead) return
        if (at == cells.size) {
            main.post { warmBusy = false }
            return
        }
        io.execute {
            try {
                require(
                    maps.cached(cells[at]) ||
                        maps.received - before + VectorMaps.TILE_MAX <= 8L * 1024 * 1024
                )
                maps.roads(cells[at])
                warmNext(cells, at + 1, before)
            } catch (_: Exception) {
                warmKey = ""
                main.post { warmBusy = false }
            }
        }
    }

    private fun updateProgress(r: NavigationRoute, p: NavigationRoute.Projection, f: PhoneFix) {
        val now = SystemClock.elapsedRealtime()
        if (!NavigationWire.precise(f, now)) return
        projection = p
        projectionSample = f.sampled
        val off = p.error > max(45.0, (f.accuracy ?: 50.0) * 2)
        if (off) {
            if (offSince == 0L) offSince = now
            offSamples++
            wire.progress = null
            if (now - offSince >= 5000 && offSamples >= 5 && now - rerouteAt >= 10000 && !routing) {
                rerouteAt = now
                select(r.destination, r.label, true)
            }
            return
        }
        offSince = 0
        offSamples = 0
        val along = r.metres(p.along)
        val remaining = r.distance - along
        val step = r.nextStep(p)
        val distanceToStep = r.metres(r.along[step.index]) - along
        wire.progress =
            NavigationWire.buffer(40)
                .putLong(r.id)
                .putInt(r.revision)
                .putLong(f.sampled)
                .putInt(remaining)
                .putInt((r.seconds.toDouble() * remaining / r.distance).roundToInt())
                .putInt(distanceToStep.coerceAtLeast(0))
                .put(r.maneuver(step).toByte())
                .put(1)
                .array()
        if (
            remaining <= 25 &&
                GeoPoint(f.lat, f.lon).distance(r.destination) <= max(20.0, f.accuracy ?: 0.0)
        ) {
            message = "Arrived — end navigation when ready"
        }
    }

    fun camera(lat: Double, lon: Double, zoom: Double) {
        if (!allowed) return
        val p = GeoPoint(lat, lon)
        val z = (floor(zoom).toInt() - 1).coerceIn(0, 14)
        val c = MapCell.at(p, z)
        val n = 1 shl z
        val cells = ArrayList<MapCell>()
        for (dy in -1..1) for (dx in -1..1) if (c.y + dy in 0 until n)
            cells.add(MapCell(z, (c.x + dx + n) % n, c.y + dy))
        val ordered = cells.sortedBy { abs(it.x - c.x) + abs(it.y - c.y) }
        if (ordered == viewport && mapError == null) return
        viewport = ordered
        viewportPending = ordered
        viewportGeneration++
        pumpViewport()
    }

    private fun pumpViewport() {
        if (viewportLoading || viewportPending == null || dead) return
        val cells = viewportPending!!
        viewportPending = null
        viewportLoading = true
        val gen = viewportGeneration
        io.execute {
            try {
                maps.initialize()
                val actual = cells.map { it.parent(maps.maxZoom) }.distinct()
                maps.view(actual)
                loadViewport(actual, 0, ArrayList(), gen, maps.received)
            } catch (e: Exception) {
                viewportFinished(gen, safe(e, "Map unavailable; cached roads retained"))
            }
        }
    }

    private fun viewportFinished(gen: Int, error: String?) {
        main.post {
            if (!dead) {
                viewportLoading = false
                if (gen == viewportGeneration) {
                    mapError = error
                    changed()
                }
                pumpViewport()
            }
        }
    }

    private fun loadViewport(
        cells: List<MapCell>,
        at: Int,
        features: ArrayList<Any>,
        gen: Int,
        before: Long,
    ) {
        if (dead) return
        if (gen != viewportGeneration) {
            viewportFinished(gen, null)
            return
        }
        if (at == cells.size) {
            viewportFinished(gen, null)
            return
        }
        io.execute {
            if (dead) return@execute
            if (gen != viewportGeneration) {
                viewportFinished(gen, null)
                return@execute
            }
            try {
                val data =
                    maps.geoJson(listOf(cells[at]), 8L * 1024 * 1024 - (maps.received - before))
                features.addAll((data["features"] as List<*>).filterNotNull())
                require(features.size <= 20000) { "Viewport too dense; zoom in" }
                val snapshot =
                    mapOf<String, Any>(
                        "type" to "FeatureCollection",
                        "features" to features.toList(),
                    )
                main.post {
                    if (!dead && gen == viewportGeneration) {
                        geo = snapshot
                        mapRevision++
                        mapError = null
                        changed()
                    }
                }
                loadViewport(cells, at + 1, features, gen, before)
            } catch (e: Exception) {
                viewportFinished(gen, safe(e, "Map unavailable; usable cached roads retained"))
            }
        }
    }

    fun search(text: String) {
        val g = searchGeneration.incrementAndGet()
        suggestions = emptyList()
        searching = text.trim().length >= 3
        message = null
        changed()
        if (!searching) return
        val query = text.trim().take(200)
        val p =
            if (NavigationWire.eligible(fix, SystemClock.elapsedRealtime()))
                GeoPoint(fix.lat, fix.lon)
            else null
        main.postDelayed(
            {
                if (dead || g != searchGeneration.get()) return@postDelayed
                io.execute {
                    if (g != searchGeneration.get()) return@execute
                    var results = emptyList<Map<String, Any>>()
                    var error: String? = null
                    try {
                        results = provider.search(query, p)
                    } catch (e: Exception) {
                        error = safe(e, "Search unavailable; check internet and API key")
                    }
                    main.post {
                        if (!dead && g == searchGeneration.get()) {
                            suggestions = results
                            searching = false
                            message = error
                            changed()
                        }
                    }
                }
            },
            400,
        )
    }

    fun select(destination: GeoPoint, label: String, rerouting: Boolean = false) {
        val now = SystemClock.elapsedRealtime()
        require(NavigationWire.precise(fix, now)) {
            "Wait for a fresh precise location (accuracy 50 m or better) before routing"
        }
        val origin = GeoPoint(fix.lat, fix.lon)
        val g = routeGeneration.incrementAndGet()
        val revision = ++routeRevision
        routing = true
        message = if (rerouting) "Off route; finding a new motorcycle route" else null
        suggestions = emptyList()
        searchGeneration.incrementAndGet()
        changed()
        io.execute {
            var next: NavigationRoute? = null
            var error: String? = null
            try {
                val fetched = provider.route(origin, destination, label, revision)
                next = fetched
                if (g == routeGeneration.get() && !dead) {
                    val bytes = fetched.json().toString().toByteArray()
                    VectorMaps.atomic(routeFile, bytes)
                    VectorMaps.atomic(File(routeFile.path + ".sha"), NavigationWire.sha(bytes))
                }
            } catch (e: Exception) {
                error = safe(e, "Routing unavailable; previous route retained")
            }
            main.post {
                if (!dead && g == routeGeneration.get()) {
                    routing = false
                    message = error
                    if (next != null) {
                        generation++
                        windowPending = null
                        contextPending = null
                        route = next
                        projection = null
                        lastProgressSample = 0
                        offSince = 0
                        offSamples = 0
                        cacheReady = false
                        cacheError = null
                        corridorGeneration.incrementAndGet()
                        wire.setRoute(next)
                        mapRevision++
                        if (sharing) {
                            currentKey = ""
                            prepareContext()
                        }
                        if (navigating) download(downloadZoom)
                    }
                    changed()
                }
            }
        }
    }

    fun start() {
        require(allowed) { "Enable maps and location first" }
        require(route != null) { "Choose a destination first" }
        navigating = true
        message = null
        download(downloadZoom)
        changed()
    }

    fun share() {
        require(allowed) { "Enable maps first" }
        require(route != null) { "Select a route first" }
        navigating = true
        sharing = true
        message = null
        currentKey = ""
        generation++
        io.execute {
            try {
                maps.initialize()
                val digest = maps.source.copyOf()
                main.post {
                    if (!dead && sharing) {
                        wire.sharing(true, digest)
                        wire.setRoute(route)
                        prepareContext()
                        download(downloadZoom)
                        changed()
                    }
                }
            } catch (e: Exception) {
                publishError(safe(e, "Map source unavailable"))
            }
        }
    }

    fun pause(error: String) {
        navigating = false
        corridorGeneration.incrementAndGet()
        cacheBusy = false
        stopSharing()
        io.execute { maps.pin(emptySet()) }
        message = error
        changed()
    }

    fun stopSharing() {
        sharing = false
        generation++
        interestPending = null
        windowPending = null
        contextPending = null
        wire.sharing(false)
        changed()
    }

    fun end() {
        navigating = false
        sharing = false
        generation++
        windowPending = null
        contextPending = null
        routeGeneration.incrementAndGet()
        corridorGeneration.incrementAndGet()
        cacheBusy = false
        cacheReady = false
        projection = null
        wire.end()
        io.execute {
            maps.pin(emptySet())
            routeFile.delete()
            File(routeFile.path + ".sha").delete()
            File(context.noBackupFilesDir, "navigation/offline.json").delete()
        }
        route = null
        mapRevision++
        message = null
        changed()
    }

    fun stopAll() {
        end()
        visible = false
        fix = PhoneFix(problem = "stopped")
        changed()
    }

    fun download(zoom: Int = downloadZoom) {
        val r = route ?: return
        require(navigating) { "Start navigation before downloading the full corridor" }
        downloadZoom = zoom.coerceIn(8, 14)
        val z = downloadZoom
        val g = corridorGeneration.incrementAndGet()
        cacheBusy = true
        cacheReady = false
        cacheDone = 0
        cacheTotal = 0
        cacheBytes = 0
        cacheError = null
        changed()
        // Each tile yields the worker to location, search and current display interests.
        io.execute {
            try {
                maps.initialize()
                val oldSource = maps.source.copyOf()
                maps.detail(z)
                val cells = LinkedHashSet<MapCell>()
                for (level in 1..minOf(z, maps.maxZoom)) {
                    cells.addAll(r.corridor(level))
                    require(cells.size <= 3000) {
                        "Route exceeds 3000 tiles across zoom levels; choose coarser detail"
                    }
                }
                maps.pin(cells)
                if (!oldSource.contentEquals(maps.source)) {
                    val digest = maps.source.copyOf()
                    main.post {
                        if (!dead && g == corridorGeneration.get()) {
                            generation++
                            warmKey = ""
                            if (sharing) {
                                wire.sharing(true, digest)
                                wire.setRoute(route)
                                currentKey = ""
                                prepareContext()
                            }
                            viewportPending = viewport
                            viewportGeneration++
                            pumpViewport()
                        }
                    }
                }
                val f = fix
                val origin =
                    if (NavigationWire.eligible(f, SystemClock.elapsedRealtime()))
                        MapCell.at(GeoPoint(f.lat, f.lon), minOf(z, maps.maxZoom))
                    else cells.first()
                val sorted = cells.sortedBy { abs(it.x - origin.x) + abs(it.y - origin.y) }
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheTotal = sorted.size
                        changed()
                    }
                }
                downloadNext(sorted, 0, 0L, g)
            } catch (e: Exception) {
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheBusy = false
                        cacheError = safe(e, "Corridor unavailable")
                        changed()
                    }
                }
            }
        }
    }

    private fun downloadNext(cells: List<MapCell>, at: Int, bytes: Long, g: Int) {
        if (dead || g != corridorGeneration.get()) return
        if (at == cells.size) {
            try {
                val manifest =
                    org.json
                        .JSONObject()
                        .put("version", 2)
                        .put(
                            "source",
                            maps.source.joinToString("") { "%02x".format(it.toInt() and 255) },
                        )
                        .put("detailZoom", downloadZoom)
                        .put("bufferMetres", 500)
                        .put("bytes", bytes)
                        .put("tiles", org.json.JSONArray(cells.map { it.key }))
                VectorMaps.atomic(
                    File(context.noBackupFilesDir, "navigation/offline.json"),
                    manifest.toString().toByteArray(),
                )
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheBusy = false
                        cacheReady = true
                        changed()
                    }
                }
            } catch (_: Exception) {
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheBusy = false
                        cacheError = "Offline manifest could not be finalized"
                        changed()
                    }
                }
            }
            return
        }
        io.execute {
            if (dead || g != corridorGeneration.get()) return@execute
            try {
                // Reserve a bounded worst-case response before making a missing-tile request.
                require(
                    bytes +
                        (if (maps.cached(cells[at])) maps.size(cells[at])
                        else VectorMaps.TILE_MAX.toLong()) <= VectorMaps.JOB_MAX
                ) {
                    "Route reaches 50 MiB offline allowance; choose coarser detail"
                }
                maps.roads(cells[at])
                val total = bytes + maps.size(cells[at])
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheDone = at + 1
                        cacheBytes = total
                        changed()
                    }
                }
                downloadNext(cells, at + 1, total, g)
            } catch (e: Exception) {
                main.post {
                    if (!dead && g == corridorGeneration.get()) {
                        cacheBusy = false
                        cacheError = safe(e, "Corridor unavailable; cached maps retained")
                        changed()
                    }
                }
            }
        }
    }

    private fun prepareContext() {
        if (!sharing || !NavigationWire.eligible(fix, SystemClock.elapsedRealtime())) return
        val cell = MapCell.at(GeoPoint(fix.lat, fix.lon), 16)
        if (currentKey == cell.key) return
        currentKey = cell.key
        contextPending = cell
        pumpContext()
    }

    private fun pumpContext() {
        if (contextLoading || contextPending == null || !sharing || dead) return
        val cell = contextPending!!
        contextPending = null
        contextLoading = true
        val g = generation
        val r = route
        val at = projection?.let { r?.metres(it.along) } ?: 0
        io.execute {
            var c: NavigationWire.Object? = null
            var overview: NavigationWire.Object? = null
            var window: NavigationWire.Object? = null
            var error: String? = null
            try {
                c = maps.cell(cell)
                overview = r?.overview(maps.source)
                window = r?.window(maps.source, at)
            } catch (e: Exception) {
                error = safe(e, "Current display map unavailable")
            }
            main.post {
                contextLoading = false
                if (!dead && sharing && g == generation) {
                    val current =
                        NavigationWire.eligible(fix, SystemClock.elapsedRealtime()) &&
                            MapCell.at(GeoPoint(fix.lat, fix.lon), 16) == cell
                    c?.let { wire.enqueue(it, current) }
                    overview?.let { wire.enqueue(it) }
                    window?.let { wire.enqueue(it) }
                    if (error != null) {
                        currentKey = ""
                        message = error
                    }
                    changed()
                }
                if (!dead) pumpContext()
            }
        }
    }

    private fun prepareWindow(at: Int) {
        windowPending = at
        pumpWindow()
    }

    private fun pumpWindow() {
        if (windowLoading || windowPending == null || !sharing || dead) return
        val at = windowPending!!
        windowPending = null
        val r = route ?: return
        val g = generation
        windowLoading = true
        io.execute {
            var obj: NavigationWire.Object? = null
            var error: String? = null
            try {
                obj = r.window(maps.source, at)
            } catch (e: Exception) {
                error = safe(e, "Route window unavailable")
            }
            main.post {
                windowLoading = false
                if (!dead) {
                    if (sharing && g == generation) {
                        obj?.let { wire.enqueue(it, true) }
                        if (error != null) message = error
                        changed()
                    }
                    pumpWindow()
                }
            }
        }
    }

    private fun interests(cells: List<MapCell>) {
        interestPending = cells.take(4)
        pumpInterests()
    }

    private fun pumpInterests() {
        if (interestLoading || interestPending == null || !sharing) return
        val cells = interestPending!!
        interestPending = null
        interestLoading = true
        val g = generation
        io.execute {
            val objects = ArrayList<NavigationWire.Object>()
            var err: String? = null
            try {
                for (c in cells) {
                    if (dead || g != generation) break
                    objects.add(maps.cell(c))
                }
            } catch (e: Exception) {
                err = safe(e, "Display browse area unavailable")
            }
            main.post {
                interestLoading = false
                if (!dead && sharing && g == generation) {
                    objects.forEach { wire.enqueue(it) }
                    if (err != null) message = err
                    changed()
                    pumpInterests()
                }
            }
        }
    }

    fun retry() {
        message = null
        mapError = null
        viewportPending = viewport
        pumpViewport()
        if (sharing) {
            currentKey = ""
            prepareContext()
        }
        changed()
    }

    fun clearCache() {
        require(!navigating && !sharing) { "End navigation first" }
        io.execute {
            try {
                maps.pin(emptySet())
                maps.clear()
                main.post {
                    if (!dead) {
                        geo = mapOf("type" to "FeatureCollection", "features" to emptyList<Any>())
                        mapRevision++
                        cacheReady = false
                        viewportGeneration++
                        viewportPending = null
                        viewport = emptyList()
                        warmKey = ""
                        message = "Cached maps cleared; the visible current area can download again"
                        changed()
                    }
                }
            } catch (_: Exception) {
                publishError("Cache could not be cleared")
            }
        }
    }

    fun mapData(): Map<String, Any?> =
        mapOf(
            "revision" to mapRevision,
            "roads" to geo,
            "route" to
                mapOf(
                    "type" to "FeatureCollection",
                    "features" to
                        (route?.let {
                            listOf(
                                mapOf(
                                    "type" to "Feature",
                                    "properties" to emptyMap<String, Any>(),
                                    "geometry" to
                                        mapOf(
                                            "type" to "LineString",
                                            "coordinates" to it.points.map { p -> p.json() },
                                        ),
                                )
                            )
                        } ?: emptyList<Any>()),
                ),
        )

    fun snapshot(): Map<String, Any?> {
        val now = SystemClock.elapsedRealtime()
        val eligible = NavigationWire.eligible(fix, now)
        val precise = NavigationWire.precise(fix, now)
        val r = route
        val p = projection
        val progressFresh =
            precise &&
                navigating &&
                offSince == 0L &&
                projectionSample > 0 &&
                now - projectionSample < 2000
        return mapOf(
            "enabled" to allowed,
            "keyConfigured" to configured,
            "visible" to visible,
            "navigating" to navigating,
            "sharing" to sharing,
            "mapRevision" to mapRevision,
            "mapLoading" to viewportLoading,
            "mapError" to mapError,
            "message" to message,
            "location" to
                mapOf(
                    "quality" to
                        if (eligible) if (precise) "precise" else "approximate"
                        else if (fix.problem == "fix")
                            if (fix.sampled > 0 && fix.sampled <= now && now - fix.sampled >= 2000)
                                "stale"
                            else "invalid"
                        else fix.problem,
                    "lat" to if (eligible) fix.lat else null,
                    "lon" to if (eligible) fix.lon else null,
                    "accuracy" to fix.accuracy?.takeIf { it.isFinite() && it >= 0 },
                    "ageMs" to if (fix.sampled > 0) now - fix.sampled else null,
                    "bearing" to
                        if (
                            precise &&
                                fix.speed?.let { it.isFinite() && it >= 2 } == true &&
                                fix.bearingAccuracy?.let { it.isFinite() && it in 0.0..45.0 } ==
                                    true &&
                                fix.bearing?.isFinite() == true
                        )
                            fix.bearing
                        else null,
                ),
            "searching" to searching,
            "suggestions" to suggestions,
            "routing" to routing,
            "route" to
                r?.let {
                    mapOf(
                        "label" to it.label,
                        "distance" to it.distance,
                        "seconds" to it.seconds,
                        "revision" to it.revision,
                        "remaining" to
                            if (progressFresh) p?.let { q -> it.distance - it.metres(q.along) }
                            else null,
                        "remainingSeconds" to
                            if (progressFresh)
                                p?.let { q ->
                                    (it.seconds.toDouble() * (it.distance - it.metres(q.along)) /
                                            it.distance)
                                        .roundToInt()
                                }
                            else null,
                        "maneuverDistance" to
                            if (progressFresh)
                                p?.let { q ->
                                    (it.metres(it.along[it.nextStep(q).index]) - it.metres(q.along))
                                        .coerceAtLeast(0)
                                }
                            else null,
                        "instruction" to
                            if (progressFresh) p?.let { q -> it.nextStep(q).text } else null,
                        "offRoute" to (offSince != 0L),
                    )
                },
            "download" to
                mapOf(
                    "ready" to cacheReady,
                    "busy" to cacheBusy,
                    "done" to cacheDone,
                    "total" to cacheTotal,
                    "bytes" to cacheBytes,
                    "zoom" to downloadZoom,
                    "error" to cacheError,
                ),
            "cache" to
                mapOf("hits" to maps.hits, "misses" to maps.misses, "received" to maps.received),
        )
    }

    fun dispose() {
        dead = true
        generation++
        corridorGeneration.incrementAndGet()
        searchGeneration.incrementAndGet()
        routeGeneration.incrementAndGet()
        wire.onInterest = null
        wire.onWindow = null
        wire.onSession = null
        main.removeCallbacksAndMessages(null)
        io.shutdownNow()
        progressWorker.shutdownNow()
    }

    private fun safe(e: Exception, fallback: String): String {
        val text = e.message ?: return fallback
        return if (
            e is IllegalArgumentException ||
                (e is IllegalStateException &&
                    !text.contains("http", true) &&
                    !text.contains("apiKey", true))
        )
            text.take(220)
        else fallback
    }
}
