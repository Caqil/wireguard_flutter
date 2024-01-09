import 'package:flutter/services.dart';

class WireGuardFlutter {
  static const String _methodChannelVpnControl =
      "billion.group.wireguard_flutter/wgcontrol";
  static const _eventChannelVpnStage =
      'billion.group.wireguard_flutter/wgstage';
  static const EventChannel _eventChannel = EventChannel(_eventChannelVpnStage);

  Stream<String> vpnStageSnapshot() => _eventChannel
      .receiveBroadcastStream()
      .map((event) => event == vpnDenied ? vpnDisconnected : event);

  Future<void> initialize(
      {String? localizedDescription, String? win32ServiceName}) {
    return const MethodChannel(_methodChannelVpnControl)
        .invokeMethod("initialize", {
      "localizedDescription": localizedDescription!,
      "win32ServiceName": win32ServiceName,
    }).then((value) async {
      await stage();
    });
  }

  Future<void> startVpn(
      {required String serverAddress,
      required String wgQuickConfig,
      required String providerBundleIdentifier,
      String? localizedDescription,
      String? win32ServiceName}) async {
    // if (Platform.isAndroid || Platform.isIOS || Platform.isMacOS) {
    await initialize(localizedDescription: localizedDescription);
    // }
    return const MethodChannel(_methodChannelVpnControl).invokeMethod("start", {
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
    await const MethodChannel(_methodChannelVpnControl).invokeMethod('stop');
  }

  Future<void> refreshStage() =>
      const MethodChannel(_methodChannelVpnControl).invokeMethod("refresh");
  Future<String> stage() =>
      const MethodChannel(_methodChannelVpnControl).invokeMethod("stage").then(
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
