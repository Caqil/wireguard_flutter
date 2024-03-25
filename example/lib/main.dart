import 'dart:async';

import 'package:flutter/material.dart';
import 'package:wireguard_flutter/wireguard_flutter.dart';

void main() {
  runApp(
    MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('WireGuard Example App'),
        ),
        body: const MyApp(),
      ),
    ),
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final wireguard = WireGuardFlutter.instance;

  late String name;

  @override
  void initState() {
    super.initState();
    wireguard.vpnStageSnapshot.listen((event) {
      debugPrint("status changed $event");
      if (mounted) {
        ScaffoldMessenger.of(context).clearSnackBars();
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
          content: Text('status changed: $event'),
        ));
      }
    });
    name = 'my_wg_vpn';
  }

  Future<void> initialize() async {
    try {
      await wireguard.initialize(interfaceName: name);
      debugPrint("initialize success $name");
    } catch (error, stack) {
      debugPrint("failed to initialize: $error\n$stack");
    }
  }

  void startVpn() async {
    try {
      await wireguard.startVpn(
        serverAddress: '167.235.55.239:51820',
        wgQuickConfig: conf,
        providerBundleIdentifier: 'com.billion.wireguardvpn.WGExtension',
      );
    } catch (error, stack) {
      debugPrint("failed to start $error\n$stack");
    }
  }

  void disconnect() async {
    try {
      await wireguard.stopVpn();
    } catch (e, str) {
      debugPrint('Failed to disconnect $e\n$str');
    }
  }

  void getStatus() async {
    debugPrint("getting stage");
    final stage = await wireguard.stage();
    debugPrint("stage: $stage");

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(
        content: Text('stage: $stage'),
      ));
    }
  }

  @override
  Widget build(BuildContext context) {
    return Container(
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
