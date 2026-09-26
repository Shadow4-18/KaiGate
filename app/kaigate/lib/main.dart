import "package:flutter/material.dart";
import "package:flutter/services.dart";
import "package:provider/provider.dart";

import "providers/knob_controller.dart";
import "screens/home_screen.dart";
import "theme/brand.dart";

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setSystemUIOverlayStyle(Brand.systemUi);
  runApp(const KaiGateApp());
}

class KaiGateApp extends StatelessWidget {
  const KaiGateApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => KnobController()..scan(),
      child: MaterialApp(
        title: "KaiGate",
        debugShowCheckedModeBanner: false,
        theme: Brand.theme(),
        home: const HomeScreen(),
      ),
    );
  }
}
