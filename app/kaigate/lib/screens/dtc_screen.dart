import "package:flutter/material.dart";
import "package:provider/provider.dart";

import "../providers/knob_controller.dart";
import "../services/dtc_decoder.dart";
import "../theme/brand.dart";

class DtcScreen extends StatelessWidget {
  const DtcScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final k = context.watch<KnobController>();
    final list = (k.dtcJson?["dtc"] as List?) ?? const [];
    return Scaffold(
      appBar: AppBar(title: const Text("DIAGNOSTICS")),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            Row(
              children: [
                Expanded(
                  child: FilledButton.tonal(
                    onPressed: () => k.cmd("queryDtc"),
                    child: const Text("READ CODES"),
                  ),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: FilledButton(
                    onPressed: () async {
                      final ok = await showDialog<bool>(
                        context: context,
                        builder: (c) => AlertDialog(
                          title: const Text("Clear fault codes?"),
                          content: const Text("This sends ELM327 mode 04 to the ECU. Confirm the engine is off or at idle."),
                          actions: [
                            TextButton(onPressed: () => Navigator.pop(c, false), child: const Text("CANCEL")),
                            TextButton(onPressed: () => Navigator.pop(c, true), child: const Text("CLEAR")),
                          ],
                        ),
                      );
                      if (ok == true) await k.cmd("clearDtc");
                    },
                    child: const Text("CLEAR"),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            if (list.isEmpty)
              const Text("No stored codes. Tap READ after the Vgate adapter is linked through the knob.")
            else
              Expanded(
                child: ListView(
                  children: list.map((e) {
                    final code = "${e["code"]}";
                    return Card(
                      color: Brand.card,
                      child: ListTile(
                        title: Text(code, style: const TextStyle(color: Brand.amber, fontSize: 20)),
                        subtitle: Text(decodeDtc(code)),
                      ),
                    );
                  }).toList(),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
