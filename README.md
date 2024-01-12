
# wireguard_flutter

forks from [mysteriumnetwork](https://github.com/mysteriumnetwork/wireguard_dart/) tunnel
A flutter plugin to setup and control VPN connection via [Wireguard](https://www.wireguard.com/) tunnel.

|             | Android | iOS   | macOS | Windows |
|-------------|---------|-------|-------|---------|
| **Support** | 21+     | 15.0+ | 12+   | TBD     |

## Usage

To use this plugin, add `wireguard_flutter` or visit  [Flutter Tutorial](https://flutterflux.com/).

"WireGuard" is a registered trademark of Jason A. Donenfeld.

## Linux

Install the required dependencies:

```bash
sudo apt install wireguard wireguard-tools openresolv
```

### Troubleshooting

On Linux, you may receive the error `resolvconf: command not found`. This is because wireguard tried to adjust the nameserver. Make sure to install `openresolv` or not provide the "DNS" field.