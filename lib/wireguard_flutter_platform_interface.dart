abstract class WireGuardFlutterInterface {
  Stream<String> vpnStageSnapshot();

  Future<void> initialize({required String interfaceName});

  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
  });

  Future<void> stopVpn();

  Future<void> refreshStage();
  Future<String> stage();
  Future<bool> isConnected() => stage().then(
        (value) =>
            value.toLowerCase() == WireGuardFlutterInterface.vpnConnected,
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
