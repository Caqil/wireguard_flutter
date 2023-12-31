#ifndef FLUTTER_PLUGIN_WIREGUARD_FLUTTER_PLUGIN_H_
#define FLUTTER_PLUGIN_WIREGUARD_FLUTTER_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

#include "service_control.h"

namespace wireguard_flutter {

class WireguardFlutterPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  WireguardFlutterPlugin();

  virtual ~WireguardFlutterPlugin();

  // Disallow copy and assign.
  WireguardFlutterPlugin(const WireguardFlutterPlugin &) = delete;
  WireguardFlutterPlugin &operator=(const WireguardFlutterPlugin &) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  std::unique_ptr<ServiceControl> tunnel_service_;
};

}  // namespace wireguard_dart

#endif  // FLUTTER_PLUGIN_WIREGUARD_DART_PLUGIN_H_
