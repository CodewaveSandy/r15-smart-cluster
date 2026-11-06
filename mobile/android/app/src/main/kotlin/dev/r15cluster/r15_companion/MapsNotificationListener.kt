package dev.r15cluster.r15_companion

import android.app.Notification
import android.app.NotificationManager
import android.content.ComponentName
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.Process
import android.os.SystemClock
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import java.lang.ref.WeakReference

/** OS-bound, Maps-only, no radio/Activity ownership and no persistent content. */
class MapsNotificationListener : NotificationListenerService() {
    private var attached = false

    override fun onListenerConnected() {
        super.onListenerConnected()
        attached = true
        MapsNotifications.attach(this)
        refreshBaseline()
    }

    override fun onListenerDisconnected() {
        attached = false
        MapsNotifications.detach(this)
        super.onListenerDisconnected()
    }

    override fun onDestroy() {
        attached = false
        MapsNotifications.detach(this)
        super.onDestroy()
    }

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        if (attached && sbn != null) MapsNotifications.post(this, sbn, false)
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        if (attached && sbn != null) MapsNotifications.remove(this, sbn)
    }

    internal fun refreshBaseline() {
        if (!attached || !MapsNotifications.capturing) return
        try {
            MapsNotifications.baseline(
                this,
                (activeNotifications ?: emptyArray())
                    .filter {
                        it.packageName == MapsNotifications.PACKAGE &&
                            it.user == Process.myUserHandle()
                    }
                    .take(9),
            )
        } catch (_: Exception) {
            MapsNotifications.unavailable(this)
        }
    }
}

/** Main-thread, one selected observation; other apps are filtered before extras. */
internal object MapsNotifications {
    const val PACKAGE = "com.google.android.apps.maps"
    private val handler = Handler(Looper.getMainLooper())
    private var listener = WeakReference<MapsNotificationListener>(null)
    private val candidates = linkedMapOf<String, MapsNotificationParser.Fields>()
    private val categories = mutableMapOf<String, Boolean>()
    private val versions = linkedMapOf<String, Long>()
    private var overflow = false
    private var selectedCore: List<Any?>? = null
    private var lastActionCore: List<Any?>? = null
    private var lastActionChangedAt: Long? = null
    private var rebindAt = -10000L
    private var rebindAttempts = 0
    private var revision = 0L
    private var pending = false
    private var problem = "Start Google Maps navigation"
    private var preview: Map<String, Any> = emptyMap()
    var capturing = false
        private set

    var bound = false
        private set

    var observation: TurnObservation? = null
        private set

    var observer: (() -> Unit)? = null
    private val removed = Runnable {
        pending = false
        observation = null
        problem = "Google Maps navigation ended"
        preview = emptyMap()
        notifyChanged()
    }

    fun granted(context: Context): Boolean =
        try {
            Build.VERSION.SDK_INT >= 27 &&
                context
                    .getSystemService(NotificationManager::class.java)
                    .isNotificationListenerAccessGranted(
                        ComponentName(context, MapsNotificationListener::class.java)
                    )
        } catch (_: Exception) {
            false
        }

    fun setCapture(value: Boolean) {
        if (capturing == value) return
        capturing = value
        handler.removeCallbacks(removed)
        candidates.clear()
        categories.clear()
        versions.clear()
        overflow = false
        selectedCore = null
        lastActionCore = null
        lastActionChangedAt = null
        observation = null
        preview = emptyMap()
        pending = false
        problem = if (value) "Waiting for Google Maps navigation" else "Guidance stopped"
        if (value) listener.get()?.refreshBaseline()
        notifyChanged()
    }

    fun refresh() {
        if (capturing && bound) listener.get()?.refreshBaseline()
    }

    fun requestRebind(context: Context) {
        val now = SystemClock.elapsedRealtime()
        if (
            Build.VERSION.SDK_INT < 24 ||
                !granted(context) ||
                bound ||
                rebindAttempts >= 3 ||
                now - rebindAt < 10000
        )
            return
        rebindAt = now
        rebindAttempts++
        try {
            NotificationListenerService.requestRebind(
                ComponentName(context, MapsNotificationListener::class.java)
            )
        } catch (_: Exception) {}
    }

    fun attach(service: MapsNotificationListener) {
        listener = WeakReference(service)
        bound = true
        rebindAttempts = 0
        clearSource("Reading current Maps notification")
        notifyChanged()
    }

    fun detach(service: MapsNotificationListener) {
        if (listener.get() !== service) return
        listener.clear()
        bound = false
        clearSource("Notification listener disconnected")
        notifyChanged()
    }

    fun unavailable(service: MapsNotificationListener) {
        if (listener.get() !== service) return
        clearSource("Notification content unavailable")
        notifyChanged()
    }

    private fun clearSource(message: String) {
        handler.removeCallbacks(removed)
        pending = false
        candidates.clear()
        categories.clear()
        versions.clear()
        overflow = false
        observation = null
        selectedCore = null
        lastActionCore = null
        lastActionChangedAt = null
        preview = emptyMap()
        problem = message
    }

    fun baseline(service: MapsNotificationListener, values: List<StatusBarNotification>) {
        if (listener.get() !== service || !capturing) return
        overflow = false
        val keys = values.map { it.key }.toSet()
        candidates.keys.retainAll(keys)
        categories.keys.retainAll(keys)
        if (values.size > 8) {
            clearSource("Multiple Maps notifications are ambiguous")
            notifyChanged()
            return
        }
        values.forEach { add(it) }
        select(true)
    }

    private fun add(sbn: StatusBarNotification) {
        if (sbn.packageName != PACKAGE || sbn.user != Process.myUserHandle()) return
        // OS postTime is only an ordering hint, never the elapsed source age.
        if (versions[sbn.key]?.let { sbn.postTime < it } == true) return
        if (!versions.containsKey(sbn.key) && versions.size >= 16)
            versions.remove(versions.keys.first())
        versions[sbn.key] = sbn.postTime
        val n = sbn.notification
        if (
            n.flags and Notification.FLAG_GROUP_SUMMARY != 0 ||
                !sbn.isOngoing && n.category != "navigation"
        ) {
            candidates.remove(sbn.key)
            categories.remove(sbn.key)
            return
        }
        if (candidates.size >= 8 && !candidates.containsKey(sbn.key)) {
            overflow = true
            problem = "Too many Maps notifications"
            return
        }
        fun clean(value: CharSequence?) =
            value
                ?.take(512)
                ?.toString()
                ?.replace(Regex("[\\p{Cc}\\p{Z}\\s]+"), " ")
                ?.trim()
                .orEmpty()
        try {
            val e = n.extras
            val fields =
                MapsNotificationParser.Fields(
                    clean(e.getCharSequence(Notification.EXTRA_TITLE)),
                    clean(e.getCharSequence(Notification.EXTRA_TEXT)),
                    clean(e.getCharSequence(Notification.EXTRA_BIG_TEXT)),
                    clean(e.getCharSequence(Notification.EXTRA_SUB_TEXT)),
                    e.getCharSequenceArray(Notification.EXTRA_TEXT_LINES)?.take(4)?.map {
                        clean(it)
                    } ?: emptyList(),
                )
            candidates[sbn.key] = fields
            categories[sbn.key] = n.category == "navigation"
        } catch (_: Exception) {
            candidates.remove(sbn.key)
            categories.remove(sbn.key)
            problem = "Maps notification content is unavailable"
        }
    }

    fun post(service: MapsNotificationListener, sbn: StatusBarNotification, baseline: Boolean) {
        if (
            listener.get() !== service ||
                !capturing ||
                sbn.packageName != PACKAGE ||
                sbn.user != Process.myUserHandle()
        )
            return
        add(sbn)
        select(baseline)
    }

    fun remove(service: MapsNotificationListener, sbn: StatusBarNotification) {
        if (
            listener.get() !== service ||
                !capturing ||
                sbn.packageName != PACKAGE ||
                sbn.user != Process.myUserHandle()
        )
            return
        if (candidates.remove(sbn.key) == null) return
        categories.remove(sbn.key)
        if (candidates.isNotEmpty()) {
            select(false)
            return
        }
        pending = true
        problem = "Maps notification is being replaced"
        handler.removeCallbacks(removed)
        handler.postDelayed(removed, 750)
        notifyChanged()
    }

    private fun select(baseline: Boolean) {
        if (overflow) {
            pending = false
            preview = emptyMap()
            updateCore(
                TurnObservation(source = 4, instruction = "Ambiguous Maps notifications"),
                true,
            )
            notifyChanged()
            return
        }
        val parsed =
            candidates.map { (key, fields) ->
                Triple(key, fields, MapsNotificationParser.parse(fields, categories[key] == true))
            }
        val useful = parsed.filter { it.third.observation != null }
        if (useful.map { it.third.observation!!.core() }.distinct().size > 1) {
            pending = false
            updateCore(
                TurnObservation(source = 4, instruction = "Ambiguous Maps notifications"),
                baseline,
            )
            preview = mapOf("candidates" to useful.take(2).map { it.second.preview() })
            problem = "Multiple eligible Maps notifications; no turn selected"
            notifyChanged()
            return
        }
        val item = useful.firstOrNull()
        if (item == null) {
            if (parsed.isNotEmpty()) {
                preview = parsed.first().second.preview()
                problem = parsed.first().third.explanation
                updateCore(
                    TurnObservation(source = 4, instruction = "Unsupported Maps notification"),
                    baseline,
                )
            } else {
                observation = null
                problem = "Start Google Maps navigation"
                preview = emptyMap()
            }
        } else {
            preview = item.second.preview()
            problem = item.third.explanation
            updateCore(item.third.observation!!, baseline)
        }
        pending = false
        handler.removeCallbacks(removed)
        notifyChanged()
    }

    private fun updateCore(next: TurnObservation, baseline: Boolean) {
        val core = next.core()
        if (selectedCore != core) {
            revision = (revision + 1) and TurnGuidanceWire.UNKNOWN
            if (revision == 0L) revision = 1
            selectedCore = core
        }
        val changedAt =
            if (next.source in listOf(1, 3)) {
                if (lastActionCore != core) {
                    lastActionCore = core
                    lastActionChangedAt = if (baseline) null else SystemClock.elapsedRealtime()
                }
                lastActionChangedAt
            } else null
        observation = next.copy(revision = revision, changedAt = changedAt)
    }

    private fun notifyChanged() {
        observer?.invoke()
    }

    fun snapshot(context: Context) =
        mapOf<String, Any?>(
            "access" to granted(context),
            "listener" to bound,
            "capturing" to capturing,
            "replacementPending" to pending,
            "problem" to problem,
            "raw" to preview,
            "profile" to MapsNotificationParser.PROFILE,
            "source" to observation?.source,
            "instruction" to observation?.instruction,
            "road" to observation?.road,
            "nextMetres" to observation?.next,
            "summary" to observation?.summary,
            "remainingMetres" to observation?.remaining,
            "remainingSeconds" to observation?.seconds,
            "roundaboutExit" to observation?.exit,
            "maneuver" to observation?.maneuver,
            "revision" to observation?.revision,
            "ageMs" to
                observation?.changedAt?.let {
                    (SystemClock.elapsedRealtime() - it).coerceAtLeast(0) / 1000 * 1000
                },
            "truncated" to observation?.truncated,
        )

    fun replacementPending() = pending
}
