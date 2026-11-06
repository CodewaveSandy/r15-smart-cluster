package dev.r15cluster.r15_companion

/** English public-extras profile. Anchored phrases only; no icon/resource guesses. */
internal object MapsNotificationParser {
    const val PROFILE = "English public notification text v1"

    data class Fields(
        val title: String,
        val text: String,
        val big: String,
        val sub: String,
        val lines: List<String>,
    ) {
        fun values() = (listOf(title, text, big, sub) + lines).filter { it.isNotBlank() }.distinct()

        fun preview() =
            mapOf(
                "title" to title,
                "text" to text,
                "bigText" to big,
                "subText" to sub,
                "textLines" to lines,
            )
    }

    data class Result(
        val observation: TurnObservation?,
        val supported: Boolean,
        val explanation: String,
    )

    private val number = "([0-9]{1,7}(?:\\.[0-9]{1,3})?)"
    private val distance = Regex("^$number\\s*(m|km|ft|mi|yd)$", RegexOption.IGNORE_CASE)
    private val prefixed =
        Regex(
            "^(?:in\\s+)?$number\\s*(m|km|ft|mi|yd)\\s*(?:[-:·]|,)?\\s+(.+)$",
            RegexOption.IGNORE_CASE,
        )
    private val phrases =
        listOf(
            Regex(
                "^(?:turn|make a) left(?:\\s+(?:onto|on|at|toward|towards)\\s+.+|$)",
                RegexOption.IGNORE_CASE,
            ) to 1,
            Regex(
                "^(?:turn|make a) right(?:\\s+(?:onto|on|at|toward|towards)\\s+.+|$)",
                RegexOption.IGNORE_CASE,
            ) to 2,
            Regex("^(?:head|continue|go) straight(?:\\s+.+|$)", RegexOption.IGNORE_CASE) to 3,
            Regex("^(?:keep|bear) left(?:\\s+.+|$)", RegexOption.IGNORE_CASE) to 5,
            Regex("^(?:keep|bear) right(?:\\s+.+|$)", RegexOption.IGNORE_CASE) to 6,
            Regex("^(?:make a u-turn|make a u turn|u-turn)(?:\\s+.+|$)", RegexOption.IGNORE_CASE) to
                7,
            Regex(
                "^(?:at (?:the )?roundabout|enter (?:the )?roundabout|take (?:the )?[0-9]+(?:st|nd|rd|th) exit)(?:\\s*[,;].+|\\s+.+|$)",
                RegexOption.IGNORE_CASE,
            ) to 8,
            Regex(
                "^(?:continue|head)(?:\\s+(?:on|onto|along|toward|towards|north|south|east|west)\\b.+|$)",
                RegexOption.IGNORE_CASE,
            ) to 0,
        )

    private fun metres(text: String, max: Long = 1000000): Long? {
        val m = distance.matchEntire(text.trim()) ?: return null
        val value = m.groupValues[1].toDoubleOrNull() ?: return null
        val factor =
            when (m.groupValues[2].lowercase()) {
                "km" -> 1000.0
                "mi" -> 1609.344
                "ft" -> 0.3048
                "yd" -> 0.9144
                else -> 1.0
            }
        val converted = value * factor
        return if (converted.isFinite() && converted >= 0 && converted <= max)
            kotlin.math.ceil(converted).toLong()
        else null
    }

    fun parse(fields: Fields, navigationCategory: Boolean): Result {
        val all = fields.values()
        val pieces =
            all.flatMap { field ->
                    val parts = field.split(Regex("\\s+[·|]\\s+")).map { it.trim() }
                    fun action(text: String) = phrases.any { it.first.matches(text) }
                    fun metadata(text: String) =
                        distance.matches(text) ||
                            text.matches(
                                Regex("(?i)^(?:ETA\\s+)?[0-9]{1,2}:[0-9]{2}(?:\\s*[AP]M)?$")
                            ) ||
                            text.matches(
                                Regex(
                                    "(?i)^(?:[0-9]{1,3}\\s*h\\s*)?[0-9]{1,3}\\s*(?:min|minutes)(?:\\s+remaining)?$"
                                )
                            )
                    // Split only a declared instruction + numeric metadata layout, never
                    // silently cut a road name at a Unicode separator.
                    if (
                        parts.size > 1 &&
                            parts.count { action(it) } == 1 &&
                            parts.all { action(it) || metadata(it) }
                    )
                        parts
                    else listOf(field)
                }
                .map { it.trim() }
                .filter { it.isNotEmpty() }
        val special =
            pieces.firstOrNull {
                it.equals("Rerouting", true) ||
                    it.equals("Rerouting...", true) ||
                    it.equals("Recalculating route", true) ||
                    it.equals("Recalculating route...", true)
            }
        if (special != null)
            return Result(
                TurnObservation(source = 2, instruction = "Rerouting"),
                true,
                "Supported rerouting status",
            )
        if (
            pieces.any {
                it.equals("GPS signal lost", true) ||
                    it.equals("Searching for GPS", true) ||
                    it.equals("GPS unavailable", true)
            }
        )
            return Result(
                TurnObservation(source = 5, instruction = "GPS unavailable"),
                true,
                "Maps reports GPS unavailable",
            )
        if (pieces.any { it.equals("Navigation paused", true) })
            return Result(
                TurnObservation(source = 6, instruction = "Navigation paused"),
                true,
                "Maps reports paused",
            )
        if (
            pieces.any {
                it.matches(
                    Regex(
                        "^(?:You have arrived|You.ve arrived|You have reached your destination|Arrived at your destination)[.!]?$",
                        RegexOption.IGNORE_CASE,
                    )
                )
            }
        )
            return Result(
                TurnObservation(source = 3, maneuver = 4, instruction = "You have arrived"),
                true,
                "Explicit arrival",
            )
        var chosen: String? = null
        var next: Long? = null
        var maneuver = 255
        for (piece in pieces) {
            val pref = prefixed.matchEntire(piece)
            val instruction = pref?.groupValues?.get(3) ?: piece
            val recognized = phrases.firstOrNull { it.first.matches(instruction) }
            if (recognized != null) {
                chosen = instruction
                maneuver = recognized.second
                next = pref?.let { metres(it.groupValues[1] + " " + it.groupValues[2]) }
                break
            }
        }
        val recognizedInstructions =
            pieces
                .mapNotNull { piece ->
                    val text = prefixed.matchEntire(piece)?.groupValues?.get(3) ?: piece
                    text.takeIf { candidate -> phrases.any { it.first.matches(candidate) } }
                }
                .distinct()
        val ambiguousSteps = recognizedInstructions.size > 1
        // A separate distance field is accepted only with one recognized first
        // instruction and one unambiguous unit-bearing value, never a route total.
        if (chosen != null && next == null && !ambiguousSteps) {
            val values = pieces.mapNotNull { metres(it) }.distinct()
            if (values.size == 1) next = values.single()
        }
        if (chosen == null && navigationCategory) {
            chosen =
                pieces.firstOrNull {
                    metres(it) == null &&
                        !it.equals("Google Maps", true) &&
                        !it.equals("Navigation", true) &&
                        !it.contains(Regex("(?i)\\bETA\\b|\\bremaining\\b"))
                }
        }
        if (chosen == null)
            return Result(null, false, "No supported navigation instruction in standard fields")
        val instruction = chosen
        if (instruction.any { it.code !in 32..126 })
            return Result(
                TurnObservation(source = 4, instruction = "Unsupported notification language"),
                false,
                "Display supports printable English/ASCII text only",
            )
        // Multi-stage text is neutral; never associate the second action/distance.
        if (ambiguousSteps || instruction.contains(Regex("(?i)\\bthen\\b|;|\\|"))) {
            maneuver = 255
            next = null
        }
        val road =
            Regex("(?i)\\bonto\\s+(.+)$").find(instruction)?.groupValues?.get(1)?.trim().orEmpty()
        if (road.any { it.code !in 32..126 })
            return Result(
                TurnObservation(source = 4, instruction = "Unsupported notification language"),
                false,
                "Road text cannot be displayed",
            )
        val exit =
            if (maneuver == 8)
                Regex("(?i)\\b(?:take\\s+(?:the\\s+)?)?([0-9]{1,2})(?:st|nd|rd|th)?\\s+exit\\b")
                    .find(instruction)
                    ?.groupValues
                    ?.get(1)
                    ?.toIntOrNull()
                    ?.takeIf { it in 1..32 } ?: 0
            else 0
        val summary =
            pieces
                .firstOrNull { it.matches(Regex("(?i)^ETA\\s+[0-9]{1,2}:[0-9]{2}(?:\\s*[AP]M)?$")) }
                .orEmpty()
        val remaining =
            pieces.firstNotNullOfOrNull {
                Regex("(?i)^remaining[: ]+(.+)$").matchEntire(it)?.let { m ->
                    metres(m.groupValues[1], 10000000)
                }
            }
        val seconds =
            pieces.firstNotNullOfOrNull {
                Regex("(?i)^([0-9]{1,3})\\s*(?:min|minutes)\\s+remaining$")
                    .matchEntire(it)
                    ?.groupValues
                    ?.get(1)
                    ?.toLongOrNull()
                    ?.times(60)
                    ?.takeIf { value -> value <= 604800 }
            }
        val truncated = instruction.length > 96 || road.length > 32 || summary.length > 16
        return Result(
            TurnObservation(
                1,
                maneuver,
                instruction.take(96),
                road.take(32),
                summary.take(16),
                next,
                remaining,
                seconds,
                exit,
                truncated,
            ),
            true,
            if (maneuver == 255) "Text only; maneuver unavailable"
            else "Recognized explicit English instruction",
        )
    }
}
