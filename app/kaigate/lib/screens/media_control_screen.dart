import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../theme/brand.dart";

class MediaControlScreen extends StatelessWidget {
  const MediaControlScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final np = k.nowPlaying;
    final width = MediaQuery.sizeOf(context).width;
    final disc = (width - 48).clamp(220.0, 320.0);
    return Scaffold(
      appBar: AppBar(title: const Text("MEDIA")),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            const Text("Mirrors the knob media-control screen. Transport keys go to the phone player."),
            if (!np.listenerEnabled) ...[
              const SizedBox(height: 16),
              _accessCard(k),
            ],
            const SizedBox(height: 24),
            Container(
              width: disc,
              height: disc,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: Brand.red, width: 3),
              ),
              child: ClipOval(
                child: Stack(
                  fit: StackFit.expand,
                  children: [
                    if (np.artwork != null)
                      Image.memory(np.artwork!, fit: BoxFit.cover, gaplessPlayback: true)
                    else
                      const DecoratedBox(
                        decoration: BoxDecoration(
                          gradient: RadialGradient(colors: [Color(0xFF2A0000), Color(0xFF000000)]),
                        ),
                      ),
                    const DecoratedBox(
                      decoration: BoxDecoration(
                        gradient: LinearGradient(
                          begin: Alignment.topCenter,
                          end: Alignment.bottomCenter,
                          colors: [Color(0x66000000), Color(0xB3000000)],
                        ),
                      ),
                    ),
                    Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 28),
                          child: Text(
                            np.title,
                            maxLines: 2,
                            overflow: TextOverflow.ellipsis,
                            textAlign: TextAlign.center,
                            style: const TextStyle(fontSize: 22, color: Brand.white, height: 1.1),
                          ),
                        ),
                        const SizedBox(height: 4),
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 24),
                          child: Text(
                            np.artist,
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            textAlign: TextAlign.center,
                            style: const TextStyle(color: Brand.mist),
                          ),
                        ),
                        const SizedBox(height: 18),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            IconButton(
                              iconSize: 36,
                              color: Brand.amber,
                              onPressed: () => k.mediaAction("prev"),
                              icon: const Icon(Icons.skip_previous),
                            ),
                            IconButton(
                              iconSize: 56,
                              color: Brand.white,
                              onPressed: () => k.mediaAction("playPause"),
                              icon: Icon(np.playing ? Icons.pause_circle_filled : Icons.play_circle_filled),
                            ),
                            IconButton(
                              iconSize: 36,
                              color: Brand.amber,
                              onPressed: () => k.mediaAction("next"),
                              icon: const Icon(Icons.skip_next),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 24),
            Row(
              children: [
                IconButton(onPressed: () => k.mediaAction("volDown"), icon: const Icon(Icons.volume_down)),
                Expanded(
                  child: Slider(
                    value: np.volume.clamp(0, 100).toDouble(),
                    max: 100,
                    onChanged: (v) => k.setVolume(v.round()),
                  ),
                ),
                IconButton(onPressed: () => k.mediaAction("volUp"), icon: const Icon(Icons.volume_up)),
              ],
            ),
            Text("${np.volume}%", style: const TextStyle(color: Brand.amber)),
            const Spacer(),
            if (!k.connected) const Text("Connect the knob to push now-playing onto the glass."),
          ],
        ),
      ),
    );
  }

  Widget _accessCard(KnobController k) {
    return Material(
      color: Brand.card,
      borderRadius: BorderRadius.circular(14),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(14, 12, 14, 12),
        child: Row(
          children: [
            const Icon(Icons.notifications_active_outlined, color: Brand.amber),
            const SizedBox(width: 10),
            const Expanded(
              child: Text("Allow notification access so KaiGate can read the current song and cover art."),
            ),
            TextButton(onPressed: k.openNotificationAccess, child: const Text("ENABLE")),
          ],
        ),
      ),
    );
  }
}
