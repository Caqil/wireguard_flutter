import 'dart:async';
import 'dart:developer' as developer;

import 'package:flutter/material.dart';
import 'package:wireguard_flutter/linux/wireguard_flutter_linux.dart';
import 'package:wireguard_flutter/wireguard_flutter.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final WireGuardFlutter wireGuardFlutter = WireGuardFlutterLinux();
  @override
  void initState() {
    super.initState();
  }

  late String name;
  // String get _randomName =>
  //     'wg_vpn_${DateTime.now().millisecondsSinceEpoch.toStringAsFixed(3).replaceAll('.', '_')}';

  Future<void> initialize() async {
    // name = _randomName;
    name = 'wg_vpn';
    try {
      await wireGuardFlutter.initialize(
        localizedDescription: name,
        win32ServiceName: name,
      );
      debugPrint("initialize success");
    } catch (e) {
      debugPrint("failed to initialize success $e");
      developer.log(
        'Setup tunnel',
        error: e,
      );
    }
  }

  void startVpn() async {
    try {
      await wireGuardFlutter.startVpn(
        serverAddress: '167.235.55.239:51820',
        wgQuickConfig: conf,
        providerBundleIdentifier: 'com.billion.wireguardvpn.WGExtension',
        localizedDescription: 'wg_example',
        win32ServiceName: name,
      );
    } catch (e) {
      debugPrint("failed to start $e");
      developer.log(
        'startVpn tunnel',
        error: e.toString(),
      );
    }
  }

  void disconnect() async {
    try {
      await wireGuardFlutter.stopVpn();
    } catch (e) {
      debugPrint(e.toString());
      developer.log(
        'Disconnect',
        error: e.toString(),
      );
    }
  }

  void getStatus() {
    debugPrint("getting stage");
    wireGuardFlutter.stage().then((value) {
      debugPrint("stage $value");
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('WireGurad example app'),
        ),
        body: Container(
          constraints: const BoxConstraints.expand(),
          padding: const EdgeInsets.all(16),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              const SizedBox(height: 20),
              TextButton(
                onPressed: initialize,
                style: ButtonStyle(
                    minimumSize:
                        MaterialStateProperty.all<Size>(const Size(100, 50)),
                    padding: MaterialStateProperty.all(
                        const EdgeInsets.fromLTRB(20, 15, 20, 15)),
                    backgroundColor:
                        MaterialStateProperty.all<Color>(Colors.blueAccent),
                    overlayColor: MaterialStateProperty.all<Color>(
                        Colors.white.withOpacity(0.1))),
                child: const Text(
                  'initialize',
                  style: TextStyle(color: Colors.white),
                ),
              ),
              const SizedBox(height: 20),
              TextButton(
                onPressed: startVpn,
                style: ButtonStyle(
                    minimumSize:
                        MaterialStateProperty.all<Size>(const Size(100, 50)),
                    padding: MaterialStateProperty.all(
                        const EdgeInsets.fromLTRB(20, 15, 20, 15)),
                    backgroundColor:
                        MaterialStateProperty.all<Color>(Colors.blueAccent),
                    overlayColor: MaterialStateProperty.all<Color>(
                        Colors.white.withOpacity(0.1))),
                child: const Text(
                  'Connect',
                  style: TextStyle(color: Colors.white),
                ),
              ),
              const SizedBox(height: 20),
              TextButton(
                onPressed: disconnect,
                style: ButtonStyle(
                    minimumSize:
                        MaterialStateProperty.all<Size>(const Size(100, 50)),
                    padding: MaterialStateProperty.all(
                        const EdgeInsets.fromLTRB(20, 15, 20, 15)),
                    backgroundColor:
                        MaterialStateProperty.all<Color>(Colors.blueAccent),
                    overlayColor: MaterialStateProperty.all<Color>(
                        Colors.white.withOpacity(0.1))),
                child: const Text(
                  'Disconnect',
                  style: TextStyle(color: Colors.white),
                ),
              ),
              const SizedBox(height: 20),
              TextButton(
                onPressed: getStatus,
                style: ButtonStyle(
                    minimumSize:
                        MaterialStateProperty.all<Size>(const Size(100, 50)),
                    padding: MaterialStateProperty.all(
                        const EdgeInsets.fromLTRB(20, 15, 20, 15)),
                    backgroundColor:
                        MaterialStateProperty.all<Color>(Colors.blueAccent),
                    overlayColor: MaterialStateProperty.all<Color>(
                        Colors.white.withOpacity(0.1))),
                child: const Text(
                  'Get status',
                  style: TextStyle(color: Colors.white),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

const String conf = '''[Interface]
PrivateKey = 0IZmHsxiNQ54TsUs0EQ71JNsa5f70zVf1LmDvON1CXc=
Address = 10.8.0.4/32
DNS = 1.1.1.1


[Peer]
PublicKey = 6uZg6T0J1bHuEmdqPx8OmxQ2ebBJ8TnVpnCdV8jHliQ=
PresharedKey = As6JiXcYcqwjSHxSOrmQT13uGVlBG90uXZWmtaezZVs=
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 0
Endpoint = 38.180.13.85:51820''';
