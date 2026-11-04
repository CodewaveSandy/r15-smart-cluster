package dev.r15cluster.r15_companion

import android.Manifest
import android.content.*
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.*
import com.google.android.gms.common.ConnectionResult
import com.google.android.gms.common.GoogleApiAvailability
import com.google.android.gms.location.*

/** Service-scoped latest observation only. Never persisted, logged or page-owned. */
internal class PhoneLocation(private val context: Context, private val observed: (PhoneFix) -> Unit) {
    private val provider = LocationServices.getFusedLocationProviderClient(context)
    private var running = false
    private var generation = 0
    private var startedAt = 0L
    private var futureFloor = 0L
    private var latest = PhoneFix()
    private val handler = Handler(Looper.getMainLooper())
    private val callback = object : LocationCallback() {
        override fun onLocationResult(result: LocationResult) {
            if (!running) return
            val location = result.lastLocation ?: return
            val sampled = location.elapsedRealtimeNanos / 1000000
            if(sampled<=futureFloor)return
            if(sampled>SystemClock.elapsedRealtime()) { futureFloor=sampled;latest=PhoneFix(problem="invalid");observed(latest);return }
            if (sampled <= startedAt || sampled <= latest.sampled) return
            latest = PhoneFix(sampled = sampled, lat = location.latitude, lon = location.longitude,
                accuracy = if (location.hasAccuracy()) location.accuracy.toDouble() else null,
                bearing = if (location.hasBearing()) location.bearing.toDouble() else null,
                bearingAccuracy = if (Build.VERSION.SDK_INT >= 26 && location.hasBearingAccuracy()) location.bearingAccuracyDegrees.toDouble() else null,
                speed = if (location.hasSpeed()) location.speed.toDouble() else null, precise = precise(),
                mock = if (Build.VERSION.SDK_INT >= 31) location.isMock else location.isFromMockProvider, problem = "fix")
            observed(latest)
        }
    }
    private val check = object : Runnable {
        override fun run() {
            if (!running) return
            val manager = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
            val enabled = if (Build.VERSION.SDK_INT >= 28) manager.isLocationEnabled else manager.isProviderEnabled(LocationManager.GPS_PROVIDER) || manager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)
            val value = if (!permitted()) PhoneFix(problem = "permissionRequired") else if (!enabled) PhoneFix(problem = "gpsDisabled") else latest.copy(precise = precise())
            observed(value)
            handler.postDelayed(this, 500)
        }
    }
    fun permitted() = context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED || context.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED
    private fun precise() = context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
    fun start(): Boolean {
        if (running) return true
        stop()
        if (!permitted()) { observed(PhoneFix(problem = "permissionRequired")); return false }
        if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) != ConnectionResult.SUCCESS) { observed(PhoneFix(problem = "providerUnsupported")); return false }
        running = true; startedAt = SystemClock.elapsedRealtime(); val requestGeneration = generation
        val request = LocationRequest.Builder(Priority.PRIORITY_HIGH_ACCURACY, 500).setMinUpdateIntervalMillis(500).setMaxUpdateDelayMillis(0).build()
        try {
            provider.requestLocationUpdates(request, callback, Looper.getMainLooper()).addOnFailureListener {
                if (running && generation == requestGeneration) { stop(); observed(PhoneFix(problem = "providerUnavailable")) }
            }
        } catch (_: SecurityException) { stop(); observed(PhoneFix(problem = "permissionRequired")); return false }
        handler.post(check); return true
    }
    fun stop() { futureFloor=0; ++generation; running = false; handler.removeCallbacks(check); try { provider.removeLocationUpdates(callback) } catch (_: Exception) { }; latest = PhoneFix() }
}
