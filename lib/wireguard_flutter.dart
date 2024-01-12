import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:wireguard_flutter/linux/wireguard_flutter_linux.dart';
import 'package:wireguard_flutter/wireguard_flutter_method_channel.dart';

import 'wireguard_flutter_platform_interface.dart';

class WireGuardFlutter extends WireGuardFlutterInterface {
  late final WireGuardFlutterInterface _instance;

  WireGuardFlutter() {
    if (kIsWeb) {
      throw UnsupportedError('The web platform is not supported');
    } else if (Platform.isLinux) {
      _instance = WireGuardFlutterLinux();
    } else {
      _instance = WireGuardFlutterMethodChannel();
    }
  }

  @override
  Stream<String> vpnStageSnapshot() => _instance.vpnStageSnapshot();

  @override
  Future<void> initialize({
    String? localizedDescription,
    String? win32ServiceName,
  }) {
    return _instance.initialize(
      localizedDescription: localizedDescription,
      win32ServiceName: win32ServiceName,
    );
  }

  @override
  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
    String? localizedDescription,
    String? win32ServiceName,
  }) async {
    return _instance.startVpn(
      serverAddress: serverAddress,
      wgQuickConfig: wgQuickConfig,
      providerBundleIdentifier: providerBundleIdentifier,
      localizedDescription: localizedDescription,
      win32ServiceName: win32ServiceName,
    );
  }

  @override
  Future<void> stopVpn() => _instance.stopVpn();

  @override
  Future<void> refreshStage() => _instance.refreshStage();

  @override
  Future<String> stage() => _instance.stage();
}
