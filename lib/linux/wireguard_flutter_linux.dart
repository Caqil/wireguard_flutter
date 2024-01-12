import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';
import 'package:process_run/shell.dart';

import '../wireguard_flutter_platform_interface.dart';

class WireGuardFlutterLinux extends WireGuardFlutterInterface {
  String? name;
  File? configFile;

  String? _stage;
  final _stageController = StreamController<String>.broadcast();
  void _setStage(String stage) {
    _stage = stage;
    _stageController.add(stage);
  }

  final shell = Shell(runInShell: true, verbose: kDebugMode);

  @override
  Future<void> initialize({required String interfaceName}) async {
    name = interfaceName;
    await refreshStage();
  }

  @override
  Future<void> startVpn({
    required String serverAddress,
    required String wgQuickConfig,
    required String providerBundleIdentifier,
  }) async {
    final isAlreadyConnected = await isConnected();
    if (!isAlreadyConnected) {
      _setStage(WireGuardFlutterInterface.vpnPrepare);
    } else {
      debugPrint('Already connected');
    }

    final tempDir = await getTemporaryDirectory();
    final filePath = '${tempDir.path}${Platform.pathSeparator}$name.conf';

    try {
      configFile = await File(filePath).create();
      await configFile!.writeAsString(wgQuickConfig);
    } on PathAccessException {
      debugPrint('Denied to write file. Trying to start interface');
      if (isAlreadyConnected) {
        return _setStage(WireGuardFlutterInterface.vpnConnected);
      }

      try {
        await shell.run('sudo wg-quick up $name');
      } catch (_) {
      } finally {
        _setStage(WireGuardFlutterInterface.vpnDenied);
      }
    }

    if (!isAlreadyConnected) {
      _setStage(WireGuardFlutterInterface.vpnConnecting);
      await shell.run('sudo wg-quick up ${configFile?.path ?? filePath}');
      _setStage(WireGuardFlutterInterface.vpnConnected);
    }
  }

  @override
  Future<void> stopVpn() async {
    assert(
      (await isConnected()),
      'Bad state: vpn has not been started. Call startVpn',
    );
    _setStage(WireGuardFlutterInterface.vpnDisconnecting);
    try {
      await shell.run('sudo wg-quick down ${configFile?.path ?? name!}');
    } catch (e) {
      await refreshStage();
      rethrow;
    }
    await refreshStage();
  }

  @override
  Future<String> stage() async =>
      _stage ?? WireGuardFlutterInterface.vpnNoConnection;

  @override
  Stream<String> get vpnStageSnapshot => _stageController.stream;

  @override
  Future<void> refreshStage() async {
    if (await isConnected()) {
      _setStage(WireGuardFlutterInterface.vpnConnected);
    } else if (name == null) {
      _setStage(WireGuardFlutterInterface.vpnWaitConnection);
    } else if (configFile == null) {
      _setStage(WireGuardFlutterInterface.vpnNoConnection);
    } else {
      _setStage(WireGuardFlutterInterface.vpnDisconnected);
    }
  }

  @override
  Future<bool> isConnected() async {
    assert(
      name != null,
      'Bad state: not initialized. Call "initialize" before calling this command',
    );
    final processResultList = await shell.run('sudo wg');
    final process = processResultList.first;
    return process.outLines.any((line) => line.trim() == 'interface: $name');
  }
}
