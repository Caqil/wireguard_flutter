//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <wireguard_flutter/wireguard_flutter_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) wireguard_flutter_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "WireguardFlutterPlugin");
  wireguard_flutter_plugin_register_with_registrar(wireguard_flutter_registrar);
}
