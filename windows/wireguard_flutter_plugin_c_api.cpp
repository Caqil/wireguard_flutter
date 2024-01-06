#include "include/wireguard_flutter/wireguard_flutter_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "wireguard_flutter_plugin.h"

void WireguardFlutterPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  wireguard_flutter::WireguardFlutterPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
