import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:wireguard_flutter/linux/wireguard_flutter_linux.dart';
import 'package:wireguard_flutter/wireguard_flutter_method_channel.dart';

import 'wireguard_flutter_platform_interface.dart';

export 'wireguard_flutter_platform_interface.dart' show VpnStage;

class WireGuardFlutter extends WireGuardFlutterInterface {
  static WireGuardFlutterInterface? __instance;
  static WireGuardFlutterInterface get _instance => __instance!;
  static WireGuardFlutterInterface get instance {
    registerWith();
    return _instance;
  }

  static void registerWith() {
    if (__instance == null) {
      if (kIsWeb) {
        throw UnsupportedError('The web platform is not supported');
      } else if (Platform.isLinux) {
        __instance = WireGuardFlutterLinux();
      } else {
        __instance = WireGuardFlutterMethodChannel();
      }
    }
  }

  WireGuardFlutter._();

  @override
  Stream<VpnStage> get vpnStageSnapshot => _instance.vpnStageSnapshot;

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
  Future<VpnStage> stage() => _instance.stage();
}
