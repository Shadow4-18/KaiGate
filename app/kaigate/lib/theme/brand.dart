import "package:flutter/material.dart";
import "package:flutter/services.dart";
import "package:google_fonts/google_fonts.dart";

class Brand {
  static const black = Color(0xFF000000);
  static const card = Color(0xFF121212);
  static const red = Color(0xFFE10600);
  static const amber = Color(0xFFFFB000);
  static const mist = Color(0xFFB9B9B9);
  static const white = Color(0xFFF5F5F5);

  static const systemUi = SystemUiOverlayStyle(
    statusBarColor: Colors.transparent,
    statusBarIconBrightness: Brightness.light,
    statusBarBrightness: Brightness.dark,
    systemNavigationBarColor: black,
    systemNavigationBarIconBrightness: Brightness.light,
  );

  static ThemeData theme() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: black,
      canvasColor: black,
      cardColor: card,
      dividerColor: const Color(0xFF1A1A1A),
      colorScheme: const ColorScheme.dark(
        primary: red,
        secondary: amber,
        surface: black,
        onSurface: white,
      ),
      textTheme: GoogleFonts.bebasNeueTextTheme(base.textTheme).apply(
        bodyColor: white,
        displayColor: white,
      ).copyWith(
        bodyMedium: GoogleFonts.dmSans(color: mist, fontSize: 14),
        bodySmall: GoogleFonts.dmSans(color: mist, fontSize: 12),
        titleMedium: GoogleFonts.dmSans(color: white, fontWeight: FontWeight.w600, fontSize: 16),
      ),
      appBarTheme: const AppBarTheme(
        backgroundColor: black,
        foregroundColor: white,
        elevation: 0,
        centerTitle: true,
        systemOverlayStyle: systemUi,
      ),
      sliderTheme: const SliderThemeData(
        activeTrackColor: red,
        thumbColor: amber,
        inactiveTrackColor: Colors.white24,
      ),
    );
  }
}
