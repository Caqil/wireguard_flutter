import 'dart:io';

import 'package:path_provider/path_provider.dart';
import 'package:process_run/shell.dart';

import '../wireguard_flutter.dart';

class WireGuardFlutterLinux extends WireGuardFlutter {
  String? name;
  File? configFile;

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
    final tempDir = await getTemporaryDirectory();
    configFile = await File(
      '${tempDir.path}${Platform.pathSeparator}$name.conf',
    ).create();
    await configFile!.writeAsString(wgQuickConfig);

    // final result = await Process.start('wg-quick', ['up', configFile!.path],
    //    runInShell: true);
    // print(result.stdout);
    // result.stdin;

    var shell = Shell();

    await shell.run('''

wg-quick up ${configFile!.path}

''');
  }

  @override
  Future<void> stopVpn() async {
    assert(configFile != null, 'Not started');
    // final result = await Process.run('wg-quick', ['down', configFile!.path]);
    // print(result.stdout);

    var shell = Shell();

    await shell.run('''

wg-quick down ${configFile!.path}

''');
  }

  @override
  Future<String> stage() async => 'bla';

  @override
  Stream<String> vpnStageSnapshot() => Stream.value('bla');
}
