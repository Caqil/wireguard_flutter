import 'dart:async';
import 'dart:io';

import 'package:path_provider/path_provider.dart';
import 'package:process_run/shell.dart';

import '../wireguard_flutter.dart';

class WireGuardFlutterLinux extends WireGuardFlutter {
  String? name;
  File? configFile;

  String? _stage;
  final _stageController = StreamController<String>.broadcast();
  void _setStage(String stage) {
    _stage = stage;
    _stageController.add(stage);
  }

  @override
  Future<void> initialize({
    String? localizedDescription,
    String? win32ServiceName,
  }) async {
    name = localizedDescription;
  }

  @override
  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
    String? localizedDescription,
    String? win32ServiceName,
  }) async {
    _setStage(WireGuardFlutter.vpnPrepare);
    final tempDir = await getTemporaryDirectory();
    configFile = await File(
      '${tempDir.path}${Platform.pathSeparator}$name.conf',
    ).create();
    await configFile!.writeAsString(wgQuickConfig);

    _setStage(WireGuardFlutter.vpnConnecting);
    final shell = Shell();
    await shell.run('wg-quick up ${configFile!.path}');
    _setStage(WireGuardFlutter.vpnConnected);
  }

  @override
  Future<void> stopVpn() async {
    assert(configFile != null, 'Not started');
    _setStage(WireGuardFlutter.vpnDisconnecting);
    final shell = Shell();
    await shell.run('wg-quick down ${configFile!.path}');
    _setStage(WireGuardFlutter.vpnDisconnected);
  }

  @override
  Future<String> stage() async => _stage ?? WireGuardFlutter.vpnNoConnection;

  @override
  Stream<String> vpnStageSnapshot() => _stageController.stream;
}
