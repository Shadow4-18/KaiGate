Import os

Import("env")

# Ensure the LittleFS data folder exists so `pio run -t uploadfs` does not fail
# on a fresh clone.
data = os.path.join(env["PROJECT_DIR"], "data")
os.makedirs(data, exist_ok=True)
keep = os.path.join(data, ".gitkeep")
if not os.path.exists(keep):
    with open(keep, "w", encoding="utf-8") as handle:
        handle.write("Place /boot.gif, /idle.gif, /wallpaper.bin here.\n")
