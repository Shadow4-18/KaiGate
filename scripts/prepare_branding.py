"""Knock JPEG black to alpha, tight-crop in-app logos, generate launcher icons."""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SEAL = ROOT / "branding" / "kaigate_seal.jpg"
WORD = ROOT / "branding" / "kaigate_wordmark.jpg"
ASSETS = ROOT / "app" / "kaigate" / "assets" / "branding"
ANDROID_RES = ROOT / "app" / "kaigate" / "android" / "app" / "src" / "main" / "res"
IOS_ICON = ROOT / "app" / "kaigate" / "ios" / "Runner" / "Assets.xcassets" / "AppIcon.appiconset"

MIPMAP = {
    "mdpi": 48,
    "hdpi": 72,
    "xhdpi": 96,
    "xxhdpi": 144,
    "xxxhdpi": 192,
}

IOS_FILES = [
    ("Icon-App-20x20@1x.png", 20),
    ("Icon-App-20x20@2x.png", 40),
    ("Icon-App-20x20@3x.png", 60),
    ("Icon-App-29x29@1x.png", 29),
    ("Icon-App-29x29@2x.png", 58),
    ("Icon-App-29x29@3x.png", 87),
    ("Icon-App-40x40@1x.png", 40),
    ("Icon-App-40x40@2x.png", 80),
    ("Icon-App-40x40@3x.png", 120),
    ("Icon-App-60x60@2x.png", 120),
    ("Icon-App-60x60@3x.png", 180),
    ("Icon-App-76x76@1x.png", 76),
    ("Icon-App-76x76@2x.png", 152),
    ("Icon-App-83.5x83.5@2x.png", 167),
    ("Icon-App-1024x1024@1x.png", 1024),
]


def knock_black(src: Image.Image) -> Image.Image:
    im = src.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, _ = px[x, y]
            peak = max(r, g, b)
            if peak <= 16:
                alpha = 0
            elif peak <= 36:
                alpha = int(255 * (peak - 16) / 20)
            else:
                alpha = 255
            px[x, y] = (r, g, b, alpha)
    return im.filter(ImageFilter.UnsharpMask(radius=0.6, percent=80, threshold=2))


def tight_crop(im: Image.Image, pad: int = 12) -> Image.Image:
    bbox = im.getbbox()
    if not bbox:
        return im
    l, t, r, b = bbox
    l = max(0, l - pad)
    t = max(0, t - pad)
    r = min(im.width, r + pad)
    b = min(im.height, b + pad)
    return im.crop((l, t, r, b))


def row_filled(im: Image.Image, y: int) -> bool:
    px = im.load()
    return any(px[x, y][3] > 24 for x in range(im.width))


def content_blocks(im: Image.Image) -> list[tuple[int, int]]:
    blocks: list[tuple[int, int]] = []
    start = None
    for y in range(im.height):
        filled = row_filled(im, y)
        if filled and start is None:
            start = y
        elif not filled and start is not None:
            if y - start > 8:
                blocks.append((start, y))
            start = None
    if start is not None and im.height - start > 8:
        blocks.append((start, im.height))
    return blocks


def extract_emblem(seal: Image.Image) -> Image.Image:
    blocks = content_blocks(seal)
    if not blocks:
        return tight_crop(seal)
    top, bottom = blocks[0]
    band = seal.crop((0, top, seal.width, bottom))
    return tight_crop(band, pad=8)


def square_pad(im: Image.Image, size: int, fill: tuple[int, int, int, int], inset: float = 0.0) -> Image.Image:
    canvas = Image.new("RGBA", (size, size), fill)
    inner = max(1, int(size * (1.0 - inset * 2)))
    fitted = im.copy()
    fitted.thumbnail((inner, inner), Image.Resampling.LANCZOS)
    x = (size - fitted.width) // 2
    y = (size - fitted.height) // 2
    canvas.paste(fitted, (x, y), fitted)
    return canvas


def save_png(im: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path, "PNG", optimize=True)


def main() -> None:
    seal = tight_crop(knock_black(Image.open(SEAL)))
    word = tight_crop(knock_black(Image.open(WORD)), pad=6)
    ASSETS.mkdir(parents=True, exist_ok=True)
    save_png(seal, ASSETS / "kaigate_seal.png")
    save_png(word, ASSETS / "kaigate_wordmark.png")

    emblem = extract_emblem(seal)
    fg = square_pad(emblem, 432, (0, 0, 0, 0), inset=0.18)
    save_png(fg, ANDROID_RES / "drawable-xxxhdpi" / "ic_launcher_foreground.png")

    for density, size in MIPMAP.items():
        icon = square_pad(emblem, size, (0, 0, 0, 255), inset=0.12)
        folder = ANDROID_RES / f"mipmap-{density}"
        save_png(icon, folder / "ic_launcher.png")
        save_png(icon, folder / "ic_launcher_round.png")

    for name, size in IOS_FILES:
        icon = square_pad(emblem, size, (0, 0, 0, 255), inset=0.10)
        save_png(icon.convert("RGB"), IOS_ICON / name)

    print(f"seal {seal.size}  wordmark {word.size}  emblem {emblem.size}")


if __name__ == "__main__":
    main()
