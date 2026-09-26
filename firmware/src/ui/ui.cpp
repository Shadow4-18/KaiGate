#include "ui/ui.h"

#include "ble/gatt_server.h"
#include "ble/obd_client.h"
#include "config.h"
#include "core/safety.h"
#include "core/state.h"
#include "hal/display_hal.h"
#include "hal/haptic_hal.h"
#include "hal/touch_hal.h"
#include "media/media_player.h"
#include "pins.h"
#include "ui/theme.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

static lv_disp_draw_buf_t s_drawBuf;
static lv_disp_drv_t s_dispDrv;
static lv_indev_drv_t s_indevDrv;
static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;

static lv_obj_t *s_tv = nullptr;
static lv_obj_t *s_lockBadge = nullptr;
static lv_obj_t *s_strobe = nullptr;
static lv_obj_t *s_gearLbl = nullptr;
static lv_obj_t *s_rpmLbl = nullptr;
static lv_obj_t *s_arc = nullptr;
static lv_obj_t *s_spdLbl = nullptr;
static lv_obj_t *s_boostBar = nullptr;
static lv_obj_t *s_boostPeak = nullptr;
static lv_obj_t *s_cltLbl = nullptr;
static lv_obj_t *s_iatLbl = nullptr;
static lv_obj_t *s_voltLbl = nullptr;
static lv_obj_t *s_gDot = nullptr;
static lv_obj_t *s_gLatPeak = nullptr;
static lv_obj_t *s_gLongPeak = nullptr;
static lv_obj_t *s_t060 = nullptr;
static lv_obj_t *s_tShift = nullptr;
static lv_obj_t *s_tBest = nullptr;
static lv_obj_t *s_dtcList = nullptr;
static lv_obj_t *s_mediaRoot = nullptr;
static lv_obj_t *s_mediaGear = nullptr;
static lv_obj_t *s_wifiLbl = nullptr;
static lv_obj_t *s_npTitle = nullptr;
static lv_obj_t *s_npArtist = nullptr;
static lv_obj_t *s_npPlay = nullptr;
static lv_obj_t *s_npVol = nullptr;
static ThemeId s_lastTheme = ThemeId::JdmAmber;
static bool s_idleShown = false;

static lv_obj_t *makeTile(lv_obj_t *tv) {
  lv_obj_t *t = lv_tileview_add_tile(tv, lv_obj_get_child_cnt(tv), 0, LV_DIR_HOR);
  lv_obj_set_style_bg_color(t, lv_color_hex(0x050505), 0);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(t, 0, 0);
  lv_obj_set_style_pad_all(t, 0, 0);
  return t;
}

static lv_obj_t *label(lv_obj_t *p, const lv_font_t *font, lv_color_t col, const char *txt) {
  lv_obj_t *l = lv_label_create(p);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, col, 0);
  lv_label_set_text(l, txt);
  return l;
}

static void buildTach(lv_obj_t *tile, const ThemeColors &th) {
  s_arc = lv_arc_create(tile);
  lv_obj_set_size(s_arc, 430, 430);
  lv_obj_center(s_arc);
  lv_arc_set_rotation(s_arc, 135);
  lv_arc_set_bg_angles(s_arc, 0, 270);
  lv_arc_set_range(s_arc, 0, 1000);
  lv_arc_set_value(s_arc, 0);
  lv_obj_remove_style(s_arc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(s_arc, 18, LV_PART_MAIN);
  lv_obj_set_style_arc_width(s_arc, 18, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
  lv_obj_set_style_arc_color(s_arc, th.fg, LV_PART_INDICATOR);

  s_gearLbl = label(tile, &lv_font_montserrat_48, th.fg, "N");
  lv_obj_center(s_gearLbl);
  lv_obj_set_style_text_font(s_gearLbl, &lv_font_montserrat_48, 0);

  s_rpmLbl = label(tile, &lv_font_montserrat_20, th.muted, "0");
  lv_obj_align(s_rpmLbl, LV_ALIGN_CENTER, 0, 64);

  s_spdLbl = label(tile, &lv_font_montserrat_16, th.muted, "0 mph");
  lv_obj_align(s_spdLbl, LV_ALIGN_CENTER, 0, 92);

  lv_obj_t *kai = label(tile, &lv_font_montserrat_14, th.accent, "改  KAIGATE");
  lv_obj_align(kai, LV_ALIGN_BOTTOM_MID, 0, -28);
}

static void buildVitals(lv_obj_t *tile, const ThemeColors &th) {
  label(tile, &lv_font_montserrat_16, th.muted, "ENGINE VITALS");
  lv_obj_align(lv_obj_get_child(tile, 0), LV_ALIGN_TOP_MID, 0, 36);

  s_boostBar = lv_bar_create(tile);
  lv_obj_set_size(s_boostBar, 280, 22);
  lv_obj_align(s_boostBar, LV_ALIGN_CENTER, 0, -70);
  lv_bar_set_range(s_boostBar, -50, 250);
  lv_obj_set_style_bg_color(s_boostBar, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_boostBar, th.accent2, LV_PART_INDICATOR);

  s_boostPeak = label(tile, &lv_font_montserrat_14, th.fg, "BOOST  0.0 kPa");
  lv_obj_align(s_boostPeak, LV_ALIGN_CENTER, 0, -42);

  s_cltLbl = label(tile, &lv_font_montserrat_18, th.fg, "CLT  --");
  lv_obj_align(s_cltLbl, LV_ALIGN_CENTER, 0, 0);
  s_iatLbl = label(tile, &lv_font_montserrat_18, th.fg, "IAT  --");
  lv_obj_align(s_iatLbl, LV_ALIGN_CENTER, 0, 36);
  s_voltLbl = label(tile, &lv_font_montserrat_18, th.fg, "BATT  --");
  lv_obj_align(s_voltLbl, LV_ALIGN_CENTER, 0, 72);
}

static void buildGForce(lv_obj_t *tile, const ThemeColors &th) {
  label(tile, &lv_font_montserrat_16, th.muted, "G-METER");
  lv_obj_align(lv_obj_get_child(tile, 0), LV_ALIGN_TOP_MID, 0, 36);

  lv_obj_t *ring = lv_obj_create(tile);
  lv_obj_set_size(ring, 260, 260);
  lv_obj_center(ring);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_0, 0);
  lv_obj_set_style_border_width(ring, 2, 0);
  lv_obj_set_style_border_color(ring, th.muted, 0);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *crossH = lv_obj_create(ring);
  lv_obj_set_size(crossH, 220, 2);
  lv_obj_center(crossH);
  lv_obj_set_style_bg_color(crossH, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(crossH, 0, 0);

  lv_obj_t *crossV = lv_obj_create(ring);
  lv_obj_set_size(crossV, 2, 220);
  lv_obj_center(crossV);
  lv_obj_set_style_bg_color(crossV, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(crossV, 0, 0);

  auto mkPeak = [&]() {
    lv_obj_t *o = lv_obj_create(ring);
    lv_obj_set_size(o, 10, 10);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, th.accent, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
  };
  s_gLatPeak = mkPeak();
  s_gLongPeak = mkPeak();

  s_gDot = lv_obj_create(ring);
  lv_obj_set_size(s_gDot, 18, 18);
  lv_obj_center(s_gDot);
  lv_obj_set_style_radius(s_gDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_gDot, th.fg, 0);
  lv_obj_set_style_border_width(s_gDot, 0, 0);
  lv_obj_clear_flag(s_gDot, LV_OBJ_FLAG_SCROLLABLE);
}

static void buildTimers(lv_obj_t *tile, const ThemeColors &th) {
  label(tile, &lv_font_montserrat_16, th.muted, "PERFORMANCE");
  lv_obj_align(lv_obj_get_child(tile, 0), LV_ALIGN_TOP_MID, 0, 40);
  s_t060 = label(tile, &lv_font_montserrat_28, th.fg, "0-60  --.-s");
  lv_obj_align(s_t060, LV_ALIGN_CENTER, 0, -30);
  s_tShift = label(tile, &lv_font_montserrat_22, th.fg, "SHIFT  ---- ms");
  lv_obj_align(s_tShift, LV_ALIGN_CENTER, 0, 20);
  s_tBest = label(tile, &lv_font_montserrat_14, th.muted, "PB  --  /  --");
  lv_obj_align(s_tBest, LV_ALIGN_CENTER, 0, 70);
}

static void buildMedia(lv_obj_t *tile, const ThemeColors &th) {
  s_mediaRoot = tile;
  mediaDrawWallpaper(tile);
  s_mediaGear = label(tile, &lv_font_montserrat_28, th.fg, "N");
  lv_obj_align(s_mediaGear, LV_ALIGN_BOTTOM_MID, 0, -48);
  s_wifiLbl = label(tile, &lv_font_montserrat_12, th.muted, "");
  lv_obj_align(s_wifiLbl, LV_ALIGN_TOP_MID, 0, 40);
}

static void onMediaAction(lv_event_t *e) {
  const char *act = static_cast<const char *>(lv_event_get_user_data(e));
  if (!act) return;
  AppState &st = AppState::get();
  {
    StateGuard g(st);
    if (!strcmp(act, "playPause")) st.nowPlaying.playing = !st.nowPlaying.playing;
    else if (!strcmp(act, "volUp")) {
      int v = st.nowPlaying.volume + 8;
      st.nowPlaying.volume = static_cast<uint8_t>(v > 100 ? 100 : v);
    } else if (!strcmp(act, "volDown")) {
      int v = st.nowPlaying.volume - 8;
      st.nowPlaying.volume = static_cast<uint8_t>(v < 0 ? 0 : v);
    }
    st.lastInputMs = millis();
    if (st.config.hapticEnabled) hapticClick();
  }
  gattNotifyMediaAction(act);
}

static lv_obj_t *roundMediaBtn(lv_obj_t *parent, const char *symbol, int size, const ThemeColors &th,
                               const char *action) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, size, size);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A1A), 0);
  lv_obj_set_style_bg_color(btn, th.accent, LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_border_color(btn, th.fg, 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, symbol);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(l, th.fg, 0);
  lv_obj_center(l);
  lv_obj_add_event_cb(btn, onMediaAction, LV_EVENT_CLICKED, const_cast<char *>(action));
  return btn;
}

static void buildMediaControl(lv_obj_t *tile, const ThemeColors &th) {
  label(tile, &lv_font_montserrat_14, th.muted, "MEDIA");
  lv_obj_align(lv_obj_get_child(tile, 0), LV_ALIGN_TOP_MID, 0, 28);

  s_npTitle = label(tile, &lv_font_montserrat_18, th.fg, "Not playing");
  lv_obj_set_width(s_npTitle, 320);
  lv_label_set_long_mode(s_npTitle, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_text_align(s_npTitle, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_npTitle, LV_ALIGN_CENTER, 0, -78);

  s_npArtist = label(tile, &lv_font_montserrat_14, th.muted, "Phone media");
  lv_obj_set_width(s_npArtist, 300);
  lv_label_set_long_mode(s_npArtist, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(s_npArtist, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_npArtist, LV_ALIGN_CENTER, 0, -48);

  static const char kPrev[] = "prev";
  static const char kPlay[] = "playPause";
  static const char kNext[] = "next";
  static const char kVdn[] = "volDown";
  static const char kVup[] = "volUp";

  lv_obj_t *prev = roundMediaBtn(tile, LV_SYMBOL_PREV, 64, th, kPrev);
  lv_obj_align(prev, LV_ALIGN_CENTER, -110, 28);
  s_npPlay = roundMediaBtn(tile, LV_SYMBOL_PLAY, 88, th, kPlay);
  lv_obj_align(s_npPlay, LV_ALIGN_CENTER, 0, 28);
  lv_obj_t *next = roundMediaBtn(tile, LV_SYMBOL_NEXT, 64, th, kNext);
  lv_obj_align(next, LV_ALIGN_CENTER, 110, 28);

  lv_obj_t *vdn = roundMediaBtn(tile, LV_SYMBOL_VOLUME_MID, 48, th, kVdn);
  lv_obj_align(vdn, LV_ALIGN_BOTTOM_MID, -70, -36);
  lv_obj_t *vup = roundMediaBtn(tile, LV_SYMBOL_VOLUME_MAX, 48, th, kVup);
  lv_obj_align(vup, LV_ALIGN_BOTTOM_MID, 70, -36);

  s_npVol = label(tile, &lv_font_montserrat_14, th.fg, "50%");
  lv_obj_align(s_npVol, LV_ALIGN_BOTTOM_MID, 0, -48);
}

static void onClearDtc(lv_event_t *e) {
  (void)e;
  obdClientClearDtc();
}

static void buildDtc(lv_obj_t *tile, const ThemeColors &th) {
  label(tile, &lv_font_montserrat_16, th.muted, "DIAGNOSTICS");
  lv_obj_align(lv_obj_get_child(tile, 0), LV_ALIGN_TOP_MID, 0, 32);
  s_dtcList = lv_label_create(tile);
  lv_obj_set_style_text_font(s_dtcList, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_dtcList, th.fg, 0);
  lv_obj_set_width(s_dtcList, 320);
  lv_label_set_long_mode(s_dtcList, LV_LABEL_LONG_WRAP);
  lv_label_set_text(s_dtcList, "No stored codes");
  lv_obj_align(s_dtcList, LV_ALIGN_CENTER, 0, -10);

  lv_obj_t *btn = lv_btn_create(tile);
  lv_obj_set_size(btn, 180, 44);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_set_style_bg_color(btn, th.accent, 0);
  lv_obj_set_style_radius(btn, 22, 0);
  lv_obj_t *bl = lv_label_create(btn);
  lv_label_set_text(bl, "CLEAR CODES");
  lv_obj_center(bl);
  lv_obj_add_event_cb(btn, onClearDtc, LV_EVENT_CLICKED, nullptr);
}

static void buildChrome(lv_obj_t *scr, const ThemeColors &th) {
  s_lockBadge = label(scr, &lv_font_montserrat_14, th.accent, LV_SYMBOL_EYE_CLOSE " LOCK");
  lv_obj_align(s_lockBadge, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_add_flag(s_lockBadge, LV_OBJ_FLAG_HIDDEN);

  s_strobe = lv_obj_create(scr);
  lv_obj_set_size(s_strobe, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_center(s_strobe);
  lv_obj_set_style_radius(s_strobe, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_strobe, lv_color_hex(0xFF0000), 0);
  lv_obj_set_style_bg_opa(s_strobe, LV_OPA_0, 0);
  lv_obj_set_style_border_width(s_strobe, 0, 0);
  lv_obj_clear_flag(s_strobe, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

void uiBegin() {
  lv_init();
  const size_t n = static_cast<size_t>(LCD_WIDTH) * LVGL_BUFFER_LINES;
  s_buf1 = static_cast<lv_color_t *>(
      heap_caps_malloc(n * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  s_buf2 = static_cast<lv_color_t *>(
      heap_caps_malloc(n * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!s_buf1) s_buf1 = static_cast<lv_color_t *>(malloc(n * sizeof(lv_color_t)));
  if (!s_buf2) s_buf2 = static_cast<lv_color_t *>(malloc(n * sizeof(lv_color_t)));
  lv_disp_draw_buf_init(&s_drawBuf, s_buf1, s_buf2, n);

  lv_disp_drv_init(&s_dispDrv);
  s_dispDrv.hor_res = LCD_WIDTH;
  s_dispDrv.ver_res = LCD_HEIGHT;
  s_dispDrv.flush_cb = displayFlush;
  s_dispDrv.draw_buf = &s_drawBuf;
  s_dispDrv.full_refresh = 0;
  lv_disp_drv_register(&s_dispDrv);

  lv_indev_drv_init(&s_indevDrv);
  s_indevDrv.type = LV_INDEV_TYPE_POINTER;
  s_indevDrv.read_cb = touchLvglRead;
  lv_indev_drv_register(&s_indevDrv);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_radius(scr, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(scr, true, 0);

  const ThemeColors th = themeColors(AppState::get().config.theme);
  s_tv = lv_tileview_create(scr);
  lv_obj_set_size(s_tv, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_center(s_tv);
  lv_obj_set_style_bg_opa(s_tv, LV_OPA_0, 0);
  lv_obj_set_scrollbar_mode(s_tv, LV_SCROLLBAR_MODE_OFF);

  buildTach(makeTile(s_tv), th);
  buildVitals(makeTile(s_tv), th);
  buildGForce(makeTile(s_tv), th);
  buildTimers(makeTile(s_tv), th);
  buildMedia(makeTile(s_tv), th);
  buildMediaControl(makeTile(s_tv), th);
  buildDtc(makeTile(s_tv), th);
  buildChrome(scr, th);
}

void uiLockIndev(bool allowSwipe) {
  if (!s_tv) return;
  if (allowSwipe) lv_obj_add_flag(s_tv, LV_OBJ_FLAG_SCROLLABLE);
  else lv_obj_clear_flag(s_tv, LV_OBJ_FLAG_SCROLLABLE);
}

static void updateTach(const AppState &st, const ThemeColors &th) {
  const uint16_t red = safetyEffectiveRedline(st);
  float t = red > 0 ? st.telemetry.rpm / static_cast<float>(red) : 0;
  if (t > 1) t = 1;
  lv_arc_set_value(s_arc, static_cast<int16_t>(t * 1000));
  lv_obj_set_style_arc_color(s_arc, themeTachColor(st.config.theme, t), LV_PART_INDICATOR);
  lv_label_set_text(s_gearLbl, gearLabel(st.telemetry.confirmedGear));
  lv_obj_set_style_text_color(s_gearLbl, st.telemetry.moneyShiftWarn ? th.warn : th.fg, 0);
  char b[24];
  snprintf(b, sizeof(b), "%u", st.telemetry.rpm);
  lv_label_set_text(s_rpmLbl, b);
  snprintf(b, sizeof(b), "%.0f mph", st.telemetry.speedMph);
  lv_label_set_text(s_spdLbl, b);
}

static void updateVitals(const AppState &st) {
  lv_bar_set_value(s_boostBar, static_cast<int32_t>(st.telemetry.boostKpa), LV_ANIM_OFF);
  char b[40];
  snprintf(b, sizeof(b), "BOOST  %.1f  pk %.1f kPa", st.telemetry.boostKpa, st.telemetry.boostPeakKpa);
  lv_label_set_text(s_boostPeak, b);
  const float cltF = st.telemetry.coolantC * 9.0f / 5.0f + 32.0f;
  snprintf(b, sizeof(b), "CLT  %.0f F", cltF);
  lv_label_set_text(s_cltLbl, b);
  snprintf(b, sizeof(b), "IAT  %.0f F", st.telemetry.iatC * 9.0f / 5.0f + 32.0f);
  lv_label_set_text(s_iatLbl, b);
  snprintf(b, sizeof(b), "BATT  %.1f V   knob %u%%", st.telemetry.batteryV, st.telemetry.knobBatteryPct);
  lv_label_set_text(s_voltLbl, b);
}

static void placeG(lv_obj_t *dot, float lat, float lon) {
  constexpr float scale = 70.0f;  // px per G
  constexpr float maxR = 110.0f;
  float x = lat * scale;
  float y = -lon * scale;
  const float r = sqrtf(x * x + y * y);
  if (r > maxR && r > 0.01f) {
    x *= maxR / r;
    y *= maxR / r;
  }
  lv_obj_align(dot, LV_ALIGN_CENTER, static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y));
}

static void updateG(const AppState &st) {
  placeG(s_gDot, st.telemetry.latG, st.telemetry.longG);
  placeG(s_gLatPeak, st.telemetry.latGPeak, 0);
  placeG(s_gLongPeak, 0, st.telemetry.longGPeak);
}

static void updateTimers(const AppState &st) {
  char b[48];
  snprintf(b, sizeof(b), "0-60  %.2fs", st.telemetry.zeroToSixtyS);
  lv_label_set_text(s_t060, b);
  snprintf(b, sizeof(b), "SHIFT  %.0f ms", st.telemetry.lastShiftMs);
  lv_label_set_text(s_tShift, b);
  snprintf(b, sizeof(b), "PB  %.2fs  /  %.0f ms", st.telemetry.bestZeroToSixtyS, st.telemetry.bestShiftMs);
  lv_label_set_text(s_tBest, b);
}

static void updateMediaControl(const AppState &st) {
  if (!s_npTitle) return;
  lv_label_set_text(s_npTitle, st.nowPlaying.title);
  lv_label_set_text(s_npArtist, st.nowPlaying.artist);
  lv_obj_t *icon = s_npPlay ? lv_obj_get_child(s_npPlay, 0) : nullptr;
  if (icon) lv_label_set_text(icon, st.nowPlaying.playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  char b[12];
  snprintf(b, sizeof(b), "%u%%", st.nowPlaying.volume);
  lv_label_set_text(s_npVol, b);
}

static void updateDtc(const AppState &st) {
  if (st.dtcCount == 0) {
    lv_label_set_text(s_dtcList, st.telemetry.obdConnected ? "No stored codes" : "OBD disconnected");
    return;
  }
  char buf[256] = {};
  size_t n = 0;
  for (uint8_t i = 0; i < st.dtcCount && n + 16 < sizeof(buf); ++i) {
    n += snprintf(buf + n, sizeof(buf) - n, "%s\n", st.dtcs[i].code);
  }
  lv_label_set_text(s_dtcList, buf);
}

void uiTick(uint32_t nowMs) {
  AppState &st = AppState::get();
  const ThemeColors th = themeColors(st.config.theme);

  if (st.telemetry.idleMascot) {
    if (s_tv) lv_obj_add_flag(s_tv, LV_OBJ_FLAG_HIDDEN);
    mediaShowIdle(true);
    s_idleShown = true;
  } else {
    if (s_idleShown) {
      mediaShowIdle(false);
      if (s_tv) lv_obj_clear_flag(s_tv, LV_OBJ_FLAG_HIDDEN);
      s_idleShown = false;
    }
    updateTach(st, th);
    updateVitals(st);
    updateG(st);
    updateTimers(st);
    updateDtc(st);
    updateMediaControl(st);
    lv_label_set_text(s_mediaGear, gearLabel(st.telemetry.confirmedGear));
    if (st.config.floatingGearOverlay) lv_obj_clear_flag(s_mediaGear, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_mediaGear, LV_OBJ_FLAG_HIDDEN);
    if (st.wifiApActive) lv_label_set_text(s_wifiLbl, "AP  KaiGate_Setup");
    else lv_label_set_text(s_wifiLbl, "");
  }

  if (st.telemetry.locked) lv_obj_clear_flag(s_lockBadge, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(s_lockBadge, LV_OBJ_FLAG_HIDDEN);

  if (st.telemetry.shiftStrobe || st.telemetry.moneyShiftWarn) {
    const bool white = ((nowMs / 80) & 1) != 0;
    lv_obj_set_style_bg_color(s_strobe, white ? lv_color_white() : lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(s_strobe, LV_OPA_70, 0);
  } else {
    lv_obj_set_style_bg_opa(s_strobe, LV_OPA_0, 0);
  }

  uiLockIndev(st.swipeAllowed);
  lv_timer_handler();
}

void uiOnThemeChanged() { s_lastTheme = AppState::get().config.theme; }
