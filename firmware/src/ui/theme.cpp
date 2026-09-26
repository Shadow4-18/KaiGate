#include "ui/theme.h"

ThemeColors themeColors(ThemeId id) {
  ThemeColors c{};
  c.bg = lv_color_hex(0x050505);
  c.warn = lv_color_hex(0xFF2A2A);
  c.cold = lv_color_hex(0x4DA3FF);
  c.muted = lv_color_hex(0x8A8A8A);
  switch (id) {
    case ThemeId::JdmAmber:
      c.fg = lv_color_hex(0xFFB000);
      c.accent = lv_color_hex(0xE10600);
      c.accent2 = lv_color_hex(0xFFE08A);
      break;
    case ThemeId::EuroSport:
      c.fg = lv_color_hex(0xF4F7FA);
      c.accent = lv_color_hex(0xD00000);
      c.accent2 = lv_color_hex(0x1E4BFF);
      break;
    case ThemeId::Mono:
      c.fg = lv_color_hex(0xF2F2F2);
      c.accent = lv_color_hex(0xFFFFFF);
      c.accent2 = lv_color_hex(0x9A9A9A);
      break;
    case ThemeId::Cyberpunk:
      c.fg = lv_color_hex(0x00F5D4);
      c.accent = lv_color_hex(0xF72585);
      c.accent2 = lv_color_hex(0x7B2CBF);
      break;
  }
  return c;
}

lv_color_t themeTachColor(ThemeId id, float t01) {
  if (t01 < 0) t01 = 0;
  if (t01 > 1) t01 = 1;
  const ThemeColors t = themeColors(id);
  if (t01 < 0.62f) {
    const float u = t01 / 0.62f;
    return lv_color_mix(lv_color_hex(0xFFE14A), lv_color_hex(0x22C55E), static_cast<uint8_t>(u * 255));
  }
  if (t01 < 0.85f) {
    const float u = (t01 - 0.62f) / 0.23f;
    return lv_color_mix(lv_color_hex(0xFF3B30), lv_color_hex(0xFFE14A), static_cast<uint8_t>(u * 255));
  }
  return t.warn;
}
