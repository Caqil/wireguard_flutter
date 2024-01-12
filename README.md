# wireguard_flutter

forks from [mysteriumnetwork](https://github.com/mysteriumnetwork/wireguard_dart/) tunnel
A flutter plugin to setup and control VPN connection via [Wireguard](https://www.wireguard.com/) tunnel.

|             | Android | iOS   | macOS | Windows | Linux |
| ----------- | ------- | ----- | ----- | ------- | ----- |
| **Support** | 21+     | 15.0+ | 12+   | 7+      | Any   |

## Usage

To use this plugin, add `wireguard_flutter` or visit [Flutter Tutorial](https://flutterflux.com/).

"WireGuard" is a registered trademark of Jason A. Donenfeld.


### Windows

On windows, the app must be run as administrator to be able to create the tunnel. To debug the app, run `flutter run` from an elevated command prompt. To run the app normally, the system will request your app to be run as administrator. No changes in the code are required.

### Linux

The required dependencies need to be installed: `wireguard` and `wireguard-tools`.

On Ubuntu/Debian, use the following command:

```bash
sudo apt install wireguard wireguard-tools openresolv
```

For other Linux distros, see [this](https://www.wireguard.com/install/).

## FAQ & Troubleshooting

On Linux, you may receive the error `resolvconf: command not found`. This is because wireguard tried to adjust the nameserver. Make sure to install `openresolv` or not provide the "DNS" field.