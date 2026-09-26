const Map<String, String> kDtcPlainEnglish = {
  "P0100": "Mass or volume air flow circuit fault — check the MAF sensor wiring.",
  "P0101": "MAF sensor range/performance — dirty sensor or intake leak.",
  "P0102": "MAF sensor circuit low — unplugged sensor or broken 5V reference.",
  "P0113": "Intake air temperature sensor high — open circuit on IAT.",
  "P0118": "Coolant temperature sensor high — open ECT circuit, often a cold-engine limp.",
  "P0128": "Coolant below thermostat regulating temperature — thermostat stuck open.",
  "P0171": "System too lean (Bank 1) — vacuum leak, weak fuel pump, or dirty MAF.",
  "P0172": "System too rich (Bank 1) — leaking injector or high fuel pressure.",
  "P0174": "System too lean (Bank 2).",
  "P0300": "Random/multiple cylinder misfire — coils, plugs, or vacuum leak.",
  "P0301": "Cylinder 1 misfire.",
  "P0302": "Cylinder 2 misfire.",
  "P0303": "Cylinder 3 misfire.",
  "P0304": "Cylinder 4 misfire.",
  "P0325": "Knock sensor circuit fault.",
  "P0335": "Crankshaft position sensor circuit fault — no RPM / no-start risk.",
  "P0340": "Camshaft position sensor circuit fault.",
  "P0420": "Catalyst efficiency below threshold (Bank 1) — aging cat or exhaust leak.",
  "P0430": "Catalyst efficiency below threshold (Bank 2).",
  "P0442": "EVAP small leak — gas cap or purge valve.",
  "P0455": "EVAP gross leak — missing gas cap.",
  "P0500": "Vehicle speed sensor fault — speedometer/OBD speed may be invalid.",
  "P0705": "Transmission range sensor circuit malfunction.",
  "P1131": "Lack of HO2S switching / adaptive fuel at limit (Ford).",
  "P1300": "Ignitor circuit malfunction (Toyota).",
  "P1310": "Ignitor circuit (Toyota coil pack).",
  "P1349": "VVT system malfunction (Toyota 1ZZ/2ZZ).",
  "P0170": "Fuel trim malfunction.",
  "P2096": "Post-catalyst fuel trim too lean (Bank 1).",
  "P2181": "Cooling system performance — thermostat or ECT plausibility.",
  "P2610": "ECM/PCM internal engine-off timer performance.",
};

String decodeDtc(String code) {
  final key = code.toUpperCase().trim();
  if (kDtcPlainEnglish.containsKey(key)) return kDtcPlainEnglish[key]!;
  if (key.startsWith("P03")) return "Misfire related — inspect coils, plugs, and compression on the indicated cylinder.";
  if (key.startsWith("P01")) return "Fuel/air metering — MAF, MAP, coolant, or fuel trim related.";
  if (key.startsWith("P02")) return "Fuel injector or fuel pressure circuit.";
  if (key.startsWith("P04")) return "Emissions control (EGR, EVAP, catalyst, secondary air).";
  if (key.startsWith("P05")) return "Idle control, speed sensor, or A/C related.";
  if (key.startsWith("P06")) return "ECM/PCM or internal control-module fault.";
  if (key.startsWith("P07") || key.startsWith("P08") || key.startsWith("P09")) {
    return "Transmission control — range switch, solenoid, or ratio error.";
  }
  if (key.startsWith("C")) return "Chassis code (ABS/TCS). Not a powertrain misfire.";
  if (key.startsWith("B")) return "Body control code.";
  if (key.startsWith("U")) return "Network/communication code — CAN bus or module offline.";
  return "No dictionary entry. Look up $key in the OEM service manual.";
}
