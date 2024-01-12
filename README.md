# wireguard_flutter

A flutter plugin to setup and control VPN connection via [Wireguard](https://www.wireguard.com/) tunnel.

- [Usage](#usage)
  - [Initialize](#initialize)
  - [Connect](#connect)
  - [Disconnect](#disconnect)
  - [Stage](#stage)
- [Supported Platforms](#supported-platforms)


## Usage

To use this plugin, add `wireguard_flutter` or visit [Flutter Tutorial](https://flutterflux.com/).

```
flutter pub add wireguard_flutter
```

### Initialize

Initialize a wireguard instance with a name using `initialize`:

```dart
final wireguard = WireGuardFlutter.instance;

// initialize the interface
await wireguard.initialize(interfaceName: 'wg0');
```

and declare the `.conf` data:
```dart
const String conf = '''[Interface]
PrivateKey = 0IZmHsxiNQ54TsUs0EQ71JNsa5f70zVf1LmDvON1CXc=
Address = 10.8.0.4/32
DNS = 1.1.1.1


[Peer]
PublicKey = 6uZg6T0J1bHuEmdqPx8OmxQ2ebBJ8TnVpnCdV8jHliQ=
PresharedKey = As6JiXcYcqwjSHxSOrmQT13uGVlBG90uXZWmtaezZVs=
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 0
Endpoint = 38.180.13.85:51820''';
```

For more info on the configuration data, see [the documentation](https://man7.org/linux/man-pages/man8/wg-quick.8.html) with examples.

### Connect

After initializing, connect using `startVpn`:

```dart
await wireguard.startVpn(
  serverAddress: address, // the server address (e.g 'demo.wireguard.com:51820')
  wgQuickConfig: conf, // the quick-config file
  providerBundleIdentifier: 'com.example', // your app identifier
);
```

### Disconnect

After connecting, disconnect using `stopVpn`:

```dart
await wireguard.stopVpn();
```

### Stage

Listen to stage change using `vpnStageSnapshot`:

```dart
wireguard.vpnStageSnapshot.listen((event) {
  debugPrint("status changed $event");
});
```

Or get the current stage using `getStage`:

```dart
final stage = await wireguard.stage();
```

## Supported Platforms

|             | Android | iOS   | macOS | Windows | Linux |
| ----------- | ------- | ----- | ----- | ------- | ----- |
| **Version** | SDK 21+     | 15.0+ | 12+   | 7+      | Any   |

### Windows

On Windows, the app must be run as administrator to be able to create and manipulate the tunnel. To debug the app, run `flutter run` from an elevated command prompt. To run the app normally, the system will request your app to be run as administrator. No changes in the code are required.

### Linux

On Linux, the app must be run as a root user to be able to create and manipulate the tunnel. The required dependencies need to be installed: `wireguard` and `wireguard-tools`.

On Ubuntu/Debian, use the following command to install the dependencies:

```bash
sudo apt install wireguard wireguard-tools openresolv
```

For other Linux distros, see [this](https://www.wireguard.com/install/).

## FAQ & Troubleshooting

On Linux, you may receive the error `resolvconf: command not found`. This is because wireguard tried to adjust the nameserver. Make sure to install `openresolv` or not provide the "DNS" field.

---

"WireGuard" is a registered trademark of Jason A. Donenfeld.

Fork from [mysteriumnetwork](https://github.com/mysteriumnetwork/wireguard_dart/) tunnel.
