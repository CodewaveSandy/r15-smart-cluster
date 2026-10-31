import 'package:flutter/material.dart';

/// Shared geometry and motion for the approved companion design.
abstract final class CompanionTokens {
  static const pagePadding = 24.0;
  static const contentWidth = 600.0;
  static const cardRadius = 18.0;
  static const controlRadius = 14.0;
  static const openingDuration = Duration(milliseconds: 900);
  static const pulseDuration = Duration(milliseconds: 1600);
  static const homeHeroAspectRatio = 1.72;
  static const telemetryGap = 10.0;
}

abstract final class CompanionTheme {
  static Color connectedColor(Brightness brightness) =>
      brightness == Brightness.dark
      ? const Color(0xff8adf98)
      : const Color(0xff226d37);

  static ThemeData forBrightness(Brightness brightness) {
    final dark = brightness == Brightness.dark;
    final colors =
        ColorScheme.fromSeed(
          seedColor: const Color(0xff258bff),
          brightness: brightness,
        ).copyWith(
          primary: dark ? const Color(0xff59adff) : const Color(0xff0063bd),
          onPrimary: dark ? const Color(0xff001c36) : Colors.white,
          surface: dark ? const Color(0xff090e14) : const Color(0xffeff3f7),
          onSurface: dark ? const Color(0xfff5f7fb) : const Color(0xff111e2c),
          onSurfaceVariant: dark
              ? const Color(0xffa5b5c7)
              : const Color(0xff4a6075),
          surfaceContainerLow: dark ? const Color(0xff111c28) : Colors.white,
          outlineVariant: dark
              ? const Color(0xff304358)
              : const Color(0xffb9c9d9),
        );
    final base = ThemeData(
      useMaterial3: true,
      fontFamily: 'Inter',
      colorScheme: colors,
      scaffoldBackgroundColor: colors.surface,
    );
    return base.copyWith(
      textTheme: base.textTheme.copyWith(
        headlineMedium: base.textTheme.headlineMedium?.copyWith(
          fontWeight: FontWeight.w600,
          letterSpacing: -0.8,
        ),
        titleLarge: base.textTheme.titleLarge?.copyWith(
          fontWeight: FontWeight.w600,
        ),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          minimumSize: const Size(48, 52),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(CompanionTokens.controlRadius),
          ),
        ),
      ),
      cardTheme: CardThemeData(
        color: colors.surfaceContainerLow,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(CompanionTokens.cardRadius),
          side: BorderSide(color: colors.outlineVariant),
        ),
      ),
      bottomSheetTheme: BottomSheetThemeData(
        backgroundColor: colors.surfaceContainerLow,
        showDragHandle: true,
        shape: const RoundedRectangleBorder(
          borderRadius: BorderRadius.vertical(top: Radius.circular(28)),
        ),
      ),
    );
  }
}
