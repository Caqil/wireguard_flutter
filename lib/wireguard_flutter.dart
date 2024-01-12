import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:wireguard_flutter/linux/wireguard_flutter_linux.dart';
import 'package:wireguard_flutter/wireguard_flutter_method_channel.dart';

import 'wireguard_flutter_platform_interface.dart';

class WireGuardFlutter extends WireGuardFlutterInterface {
  static bool _initialized = false;
  static late WireGuardFlutterInterface _instance;

  static void registerWith() {
    _instance = WireGuardFlutter();
  }

  WireGuardFlutter() {
    if (!_initialized) {
      if (kIsWeb) {
        throw UnsupportedError('The web platform is not supported');
      } else if (Platform.isLinux) {
        _instance = WireGuardFlutterLinux();
      } else {
        _instance = WireGuardFlutterMethodChannel();
      }
      _initialized = true;
    }
  }

  @override
  Stream<String> vpnStageSnapshot() => _instance.vpnStageSnapshot();

  @override
  Future<void> initialize({required String interfaceName}) {
    return _instance.initialize(interfaceName: interfaceName);
  }

  @override
  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
  }) async {
    return _instance.startVpn(
      serverAddress: serverAddress,
      wgQuickConfig: wgQuickConfig,
      providerBundleIdentifier: providerBundleIdentifier,
    );
  }

  @override
  Future<void> stopVpn() => _instance.stopVpn();

  @override
  Future<void> refreshStage() => _instance.refreshStage();

  @override
  Future<String> stage() => _instance.stage();
}
