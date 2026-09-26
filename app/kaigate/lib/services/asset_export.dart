import "dart:io";
import "dart:typed_data";

import "package:http/http.dart" as http;
import "package:image/image.dart" as img;

const knobPixels = 466;
const apBase = "http://192.168.4.1";

/// Crop a source image into a 466×466 RGB565 little-endian framebuffer
/// suitable for `/wallpaper.bin` on the knob LittleFS partition.
Uint8List toRgb565Bin(img.Image src, {double scale = 1, double panX = 0, double panY = 0}) {
  final square = img.Image(width: knobPixels, height: knobPixels);
  final cx = src.width / 2 + panX;
  final cy = src.height / 2 + panY;
  final span = (src.width < src.height ? src.width : src.height) / scale;
  final left = (cx - span / 2).round();
  final top = (cy - span / 2).round();
  for (var y = 0; y < knobPixels; y++) {
    for (var x = 0; x < knobPixels; x++) {
      final dx = x - knobPixels / 2;
      final dy = y - knobPixels / 2;
      if (dx * dx + dy * dy > (knobPixels / 2) * (knobPixels / 2)) {
        square.setPixelRgba(x, y, 0, 0, 0, 255);
        continue;
      }
      final sx = left + (x * span / knobPixels).round();
      final sy = top + (y * span / knobPixels).round();
      if (sx < 0 || sy < 0 || sx >= src.width || sy >= src.height) {
        square.setPixelRgba(x, y, 0, 0, 0, 255);
        continue;
      }
      final p = src.getPixel(sx, sy);
      square.setPixelRgba(x, y, p.r.toInt(), p.g.toInt(), p.b.toInt(), 255);
    }
  }
  final out = Uint8List(knobPixels * knobPixels * 2);
  var i = 0;
  for (var y = 0; y < knobPixels; y++) {
    for (var x = 0; x < knobPixels; x++) {
      final p = square.getPixel(x, y);
      final r = (p.r.toInt() >> 3) & 0x1F;
      final g = (p.g.toInt() >> 2) & 0x3F;
      final b = (p.b.toInt() >> 3) & 0x1F;
      final rgb = (r << 11) | (g << 5) | b;
      out[i++] = rgb & 0xFF;
      out[i++] = (rgb >> 8) & 0xFF;
    }
  }
  return out;
}

Uint8List toPreviewPng(img.Image src, {double scale = 1, double panX = 0, double panY = 0}) {
  final bin = toRgb565Bin(src, scale: scale, panX: panX, panY: panY);
  final preview = img.Image(width: knobPixels, height: knobPixels);
  for (var i = 0, y = 0; y < knobPixels; y++) {
    for (var x = 0; x < knobPixels; x++, i += 2) {
      final rgb = bin[i] | (bin[i + 1] << 8);
      final r = ((rgb >> 11) & 0x1F) << 3;
      final g = ((rgb >> 5) & 0x3F) << 2;
      final b = (rgb & 0x1F) << 3;
      preview.setPixelRgba(x, y, r, g, b, 255);
    }
  }
  return Uint8List.fromList(img.encodePng(preview));
}

class WifiTransfer {
  Future<bool> ping() async {
    try {
      final r = await http.get(Uri.parse("$apBase/status")).timeout(const Duration(seconds: 3));
      return r.statusCode == 200;
    } catch (_) {
      return false;
    }
  }

  Future<void> uploadBytes(Uint8List bytes, String destPath, {String filename = "asset.bin"}) async {
    final req = http.MultipartRequest("POST", Uri.parse("$apBase/upload"));
    req.fields["dest"] = destPath;
    req.files.add(http.MultipartFile.fromBytes("file", bytes, filename: filename));
    final res = await req.send();
    if (res.statusCode != 200) {
      throw HttpException("Upload failed (${res.statusCode})");
    }
  }
}
