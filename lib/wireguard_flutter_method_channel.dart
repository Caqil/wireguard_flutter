import 'dart:io';

import 'package:flutter/services.dart';

import 'wireguard_flutter_platform_interface.dart';

class WireGuardFlutterMethodChannel extends WireGuardFlutterInterface {
  static const String _methodChannelVpnControl =
      "billion.group.wireguard_flutter/wgcontrol";
  static const _methodChannel = MethodChannel(_methodChannelVpnControl);
  static const _eventChannelVpnStage =
      'billion.group.wireguard_flutter/wgstage';
  static const EventChannel _eventChannel = EventChannel(_eventChannelVpnStage);

  @override
  Stream<String> vpnStageSnapshot() => _eventChannel
      .receiveBroadcastStream()
      .map((event) => event == WireGuardFlutterInterface.vpnDenied
          ? WireGuardFlutterInterface.vpnDisconnected
          : event);

  @override
  Future<void> initialize(
      {String? localizedDescription, String? win32ServiceName}) {
    return _methodChannel.invokeMethod("initialize", {
      "localizedDescription": localizedDescription!,
      "win32ServiceName": win32ServiceName,
    }).then((value) {
      stage();
    });
  }

  @override
  Future<void> startVpn(
      {required String serverAddress,
      required String wgQuickConfig,
      required String providerBundleIdentifier,
      String? localizedDescription,
      String? win32ServiceName}) async {
    if (Platform.isAndroid || Platform.isIOS || Platform.isMacOS) {
      await initialize(localizedDescription: localizedDescription);
    }
    return _methodChannel.invokeMethod("start", {
      "serverAddress": serverAddress,
      "wgQuickConfig": wgQuickConfig,
      "providerBundleIdentifier": providerBundleIdentifier,
      "localizedDescription": localizedDescription,
      "win32ServiceName": win32ServiceName,
    }).then((value) {
      stage();
    });
  }

  @override
  Future<void> stopVpn() async {
    await _methodChannel.invokeMethod('stop');
  }

  @override
  Future<void> refreshStage() => _methodChannel.invokeMethod("refresh");

  @override
  Future<String> stage() => _methodChannel.invokeMethod("stage").then(
        (value) => value ?? WireGuardFlutterInterface.vpnDisconnected,
      );
}
