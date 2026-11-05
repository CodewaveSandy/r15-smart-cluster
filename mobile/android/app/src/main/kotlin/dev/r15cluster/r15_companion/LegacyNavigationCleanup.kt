package dev.r15cluster.r15_companion

import android.content.Context
import java.io.File
import java.security.KeyStore

/** Upgrade-only removal of private data owned exclusively by the retired maps feature. */
internal object LegacyNavigationCleanup {
    @Synchronized
    fun start(context: Context) {
        val app = context.applicationContext
        val marker = app.getSharedPreferences("retired-navigation-cleanup", Context.MODE_PRIVATE)
        if (marker.getBoolean("complete", false)) return
        Thread(
                {
                    try {
                        val root = File(app.noBackupFilesDir, "navigation")
                        check(!root.exists() || root.deleteRecursively())
                        val keys = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
                        if (keys.containsAlias("r15-navigation")) keys.deleteEntry("r15-navigation")
                        check(
                            app.getSharedPreferences("navigation", Context.MODE_PRIVATE)
                                .edit()
                                .clear()
                                .commit()
                        )
                        check(
                            app.getSharedPreferences("navigation-quota", Context.MODE_PRIVATE)
                                .edit()
                                .clear()
                                .commit()
                        )
                        // BLE selection/bonds and Mac pairing keys are deliberately untouched.
                        marker.edit().putBoolean("complete", true).commit()
                    } catch (_: Exception) {
                        // Retry on the next service creation. Never log provider keys or routes.
                        android.util.Log.w("R15Cleanup", "Retired map data cleanup will retry")
                    }
                },
                "retired-map-cleanup",
            )
            .start()
    }
}
