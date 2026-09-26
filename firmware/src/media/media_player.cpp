#include "media/media_player.h"

#include "config.h"
#include "hal/display_hal.h"
#include "pins.h"

#include <AnimatedGIF.h>
#include <LittleFS.h>
#include <cstring>

static AnimatedGIF s_gif;
static File s_gifFile;
static bool s_fs = false;
static uint16_t *s_lineBuf = nullptr;

static void *gifOpen(const char *name, int32_t *size) {
  s_gifFile = LittleFS.open(name, "r");
  if (!s_gifFile) return nullptr;
  *size = s_gifFile.size();
  return (void *)&s_gifFile;
}
static void gifClose(void *h) {
  (void)h;
  if (s_gifFile) s_gifFile.close();
}
static int32_t gifRead(GIFFILE *f, uint8_t *buf, int32_t len) {
  File *fp = static_cast<File *>(f->fHandle);
  return fp->read(buf, len);
}
static int32_t gifSeek(GIFFILE *f, int32_t pos) {
  File *fp = static_cast<File *>(f->fHandle);
  fp->seek(pos);
  return pos;
}

static void gifDraw(GIFDRAW *p) {
  if (!p || p->y < 0 || p->iWidth <= 0) return;
  uint16_t line[LCD_WIDTH];
  const int w = p->iWidth > LCD_WIDTH ? LCD_WIDTH : p->iWidth;
  if (p->pPalette) {
    const uint16_t *pal = reinterpret_cast<const uint16_t *>(p->pPalette);
    for (int x = 0; x < w; ++x) line[x] = pal[p->pPixels[x]];
  } else {
    memcpy(line, p->pPixels, w * sizeof(uint16_t));
  }
  displayDrawBuffer(static_cast<int16_t>(p->iX), static_cast<int16_t>(p->iY + p->y),
                    static_cast<int16_t>(w), 1, line);
}

bool mediaBegin() {
  s_fs = LittleFS.begin(true, "/littlefs", 10, "littlefs");
  if (!s_fs) s_fs = LittleFS.begin(true);
  s_gif.begin(GIF_PALETTE_RGB565_LE);
  s_lineBuf = static_cast<uint16_t *>(heap_caps_malloc(LCD_WIDTH * 2, MALLOC_CAP_INTERNAL));
  Serial.printf("[fs] LittleFS %s  idle=%d wallpaper=%d\n", s_fs ? "ok" : "FAIL", mediaHasIdle(),
                mediaHasWallpaper());
  return s_fs;
}

bool mediaHasIdle() { return s_fs && LittleFS.exists(LITTLEFS_IDLE_GIF); }
bool mediaHasWallpaper() { return s_fs && LittleFS.exists(LITTLEFS_WALLPAPER); }

void mediaPlayBoot() {
  if (!s_fs || !LittleFS.exists(LITTLEFS_BOOT_GIF)) return;
  if (s_gif.open(LITTLEFS_BOOT_GIF, gifOpen, gifClose, gifRead, gifSeek, gifDraw)) {
    const uint32_t start = millis();
    while (s_gif.playFrame(true, nullptr) && (millis() - start) < 4000) {
      delay(1);
    }
    s_gif.close();
  }
}

void mediaShowIdle(bool on) {
  static bool open = false;
  if (!on) {
    if (open) {
      s_gif.close();
      open = false;
    }
    return;
  }
  if (!mediaHasIdle()) return;
  if (!open) {
    open = s_gif.open(LITTLEFS_IDLE_GIF, gifOpen, gifClose, gifRead, gifSeek, gifDraw);
  }
  if (open) {
    int delayMs = 0;
    if (!s_gif.playFrame(false, &delayMs)) {
      s_gif.close();
      open = s_gif.open(LITTLEFS_IDLE_GIF, gifOpen, gifClose, gifRead, gifSeek, gifDraw);
    }
  }
}

void mediaDrawWallpaper(lv_obj_t *parent) {
  if (!mediaHasWallpaper() || !parent) return;
  File f = LittleFS.open(LITTLEFS_WALLPAPER, "r");
  if (!f) return;
  const size_t need = static_cast<size_t>(LCD_WIDTH) * LCD_HEIGHT * 2;
  uint16_t *buf = static_cast<uint16_t *>(heap_caps_malloc(need, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!buf) {
    f.close();
    return;
  }
  f.read(reinterpret_cast<uint8_t *>(buf), need);
  f.close();
  static lv_img_dsc_t dsc;
  dsc.header.always_zero = 0;
  dsc.header.w = LCD_WIDTH;
  dsc.header.h = LCD_HEIGHT;
  dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  dsc.data_size = need;
  dsc.data = reinterpret_cast<const uint8_t *>(buf);
  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, &dsc);
  lv_obj_center(img);
}
