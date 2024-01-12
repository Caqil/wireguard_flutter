import 'package:flutter/services.dart';

import 'wireguard_flutter_platform_interface.dart';

class WireGuardFlutterMethodChannel extends WireGuardFlutterInterface {
  static const _methodChannelVpnControl =
      "billion.group.wireguard_flutter/wgcontrol";
  static const _methodChannel = MethodChannel(_methodChannelVpnControl);
  static const _eventChannelVpnStage =
      'billion.group.wireguard_flutter/wgstage';
  static const _eventChannel = EventChannel(_eventChannelVpnStage);

  @override
  Stream<String> get vpnStageSnapshot => _eventChannel
      .receiveBroadcastStream()
      .map((event) => event == WireGuardFlutterInterface.vpnDenied
          ? WireGuardFlutterInterface.vpnDisconnected
          : event);

  @override
  Future<void> initialize({required String interfaceName}) {
    return _methodChannel.invokeMethod("initialize", {
      "localizedDescription": interfaceName,
      "win32ServiceName": interfaceName,
    });
  }

  @override
  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
  }) async {
    return _methodChannel.invokeMethod("start", {
      "serverAddress": serverAddress,
      "wgQuickConfig": wgQuickConfig,
      "providerBundleIdentifier": providerBundleIdentifier,
    });
  }

  @override
  Future<void> stopVpn() => _methodChannel.invokeMethod('stop');

  @override
  Future<void> refreshStage() => _methodChannel.invokeMethod("refresh");

  @override
  Future<String> stage() => _methodChannel.invokeMethod("stage").then(
        (value) => value ?? WireGuardFlutterInterface.vpnDisconnected,
      );
}
