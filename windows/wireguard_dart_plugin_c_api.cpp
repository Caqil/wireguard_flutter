#include "include/wireguard_dart/wireguard_dart_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "wireguard_dart_plugin.h"

void WireguardDartPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  wireguard_dart::WireguardDartPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
