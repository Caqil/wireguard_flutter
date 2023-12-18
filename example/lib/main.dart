import 'dart:async';
import 'dart:developer' as developer;

import 'package:flutter/material.dart';
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
  final wireGuardFlutter = WireGuardFlutter();
  @override
  void initState() {
    super.initState();
  }

  Future<void> initialize() async {
    try {
      await wireGuardFlutter.initialize(
        localizedDescription: "wg_example",
      );
      debugPrint("initialize success");
    } catch (e) {
      developer.log(
        'Setup tunnel',
        error: e,
      );
    }
  }

  void startVpn() async {
    await wireGuardFlutter.startVpn(
        serverAddress: '167.235.55.239:51820',
        wgQuickConfig: conf,
        providerBundleIdentifier: 'com.billion.wireguardvpn.WGExtension',
        localizedDescription: 'wg_example');
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
            ],
          ),
        ),
      ),
    );
  }
}

const String conf = '''[Interface]
PrivateKey = oBYGJwdyTr26QDn/21X8Lv4VwkENmnL0SoB+38Loomg=
Address = 10.8.0.3/32
DNS = 1.1.1.1


[Peer]
PublicKey = 0IoSpwT2UJ/YVXXu00r9PVX8jD2VsWPXXxXGEFS0RBQ=
PresharedKey = ysGiZHVPixmm/bY89/uDnvtO+dKC7A7FsmD1EQRp8VE=
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 0
Endpoint = 167.235.55.239:51820''';
