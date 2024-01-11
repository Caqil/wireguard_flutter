import 'package:flutter/services.dart';

class WireGuardFlutter {
  static const String _methodChannelVpnControl =
      "billion.group.wireguard_flutter/wgcontrol";
  static const _methodChannel = MethodChannel(_methodChannelVpnControl);
  static const _eventChannelVpnStage =
      'billion.group.wireguard_flutter/wgstage';
  static const _eventChannel = EventChannel(_eventChannelVpnStage);

  Stream<String> vpnStageSnapshot() => _eventChannel
      .receiveBroadcastStream()
      .map((event) => event == vpnDenied ? vpnDisconnected : event);

  Future<void> initialize({
    required String localizedDescription,
    required String win32ServiceName,
  }) {
    return _methodChannel.invokeMethod("initialize", {
      "localizedDescription": localizedDescription,
      "win32ServiceName": win32ServiceName,
    });
  }

  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
    required String localizedDescription,
    required String win32ServiceName,
  }) async {
    await initialize(
      localizedDescription: localizedDescription,
      win32ServiceName: win32ServiceName,
    );
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

  Future<void> stopVpn() async {
    await _methodChannel.invokeMethod('stop');
  }

  // Future<void> refreshStage() => _methodChannel.invokeMethod("refresh");
  Future<String> stage() => _methodChannel.invokeMethod("stage").then(
        (value) => value ?? vpnDisconnected,
      );
  Future<bool> isConnected() => stage().then(
        (value) => value.toLowerCase() == vpnConnected,
      );

  static const String vpnConnected = "connected";
  static const String vpnDisconnecting = "disconnecting";
  static const String vpnDisconnected = "disconnected";
  static const String vpnWaitConnection = "wait_connection";
  static const String vpnAuthenticating = "authenticating";
  static const String vpnReconnect = "reconnect";
  static const String vpnNoConnection = "no_connection";
  static const String vpnConnecting = "connecting";
  static const String vpnPrepare = "prepare";
  static const String vpnDenied = "denied";
  static const String vpnExiting = "exiting";
}
