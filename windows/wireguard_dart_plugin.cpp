#include "wireguard_dart_plugin.h"

// This must be included before many other Windows headers.
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <libbase64.h>
#include <windows.h>

#include <memory>
#include <sstream>

#include "config_writer.h"
#include "key_generator.h"
#include "service_control.h"
#include "tunnel.h"
#include "utils.h"
#include "wireguard.h"

namespace wireguard_dart {

// static
void WireguardDartPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar) {
  auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "wireguard_dart", &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<WireguardDartPlugin>();

  channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto &call, auto result) {
    plugin_pointer->HandleMethodCall(call, std::move(result));
  });

  registrar->AddPlugin(std::move(plugin));
}

WireguardDartPlugin::WireguardDartPlugin() {}

WireguardDartPlugin::~WireguardDartPlugin() {}

void WireguardDartPlugin::HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &call,
                                           std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto *args = std::get_if<flutter::EncodableMap>(call.arguments());

  if (call.method_name() == "generateKeyPair") {
    std::pair public_private_keypair = GenerateKeyPair();
    std::map<flutter::EncodableValue, flutter::EncodableValue> return_value;
    return_value[flutter::EncodableValue("publicKey")] = flutter::EncodableValue(public_private_keypair.first);
    return_value[flutter::EncodableValue("privateKey")] = flutter::EncodableValue(public_private_keypair.second);
    result->Success(flutter::EncodableValue(return_value));
    return;
  }

  if (call.method_name() == "nativeInit") {
    // Disable packet forwarding that conflicts with WireGuard
    ServiceControl remoteAccessService = ServiceControl(L"RemoteAccess");
    try {
      remoteAccessService.Stop();
    } catch (std::exception &e) {
      result->Error(std::string("Could not stop packet forwarding: ").append(e.what()));
      return;
    }
    try {
      remoteAccessService.Disable();
    } catch (std::exception &e) {
      result->Error(std::string("Could not disable packet forwarding: ").append(e.what()));
      return;
    }
    result->Success();
    return;
  }

  if (call.method_name() == "setupTunnel") {
    const auto *arg_service_name = std::get_if<std::string>(ValueOrNull(*args, "win32ServiceName"));
    if (arg_service_name == NULL) {
      result->Error("Argument 'win32ServiceName' is required");
      return;
    }
    this->tunnel_service_ = std::make_unique<ServiceControl>(Utf8ToWide(*arg_service_name));
    result->Success();
    return;
  }

  if (call.method_name() == "connect") {
    auto tunnel_service = this->tunnel_service_.get();
    if (tunnel_service == nullptr) {
      result->Error("Invalid state: call 'setupTunnel' first");
      return;
    }
    const auto *cfg = std::get_if<std::string>(ValueOrNull(*args, "cfg"));
    if (cfg == NULL) {
      result->Error("Argument 'cfg' is required");
      return;
    }

    std::wstring wg_config_filename;
    try {
      wg_config_filename = WriteConfigToTempFile(*cfg);
    } catch (std::exception &e) {
      result->Error(std::string("Could not write wireguard config: ").append(e.what()));
      return;
    }

    wchar_t module_filename[MAX_PATH];
    GetModuleFileName(NULL, module_filename, MAX_PATH);
    auto current_exec_dir = std::wstring(module_filename);
    current_exec_dir = current_exec_dir.substr(0, current_exec_dir.find_last_of(L"\\/"));

    std::wostringstream service_exec_builder;
    service_exec_builder << current_exec_dir << "\\wireguard_svc.exe" << L" -service"
                         << L" -config-file=\"" << wg_config_filename << "\"";
    std::wstring service_exec = service_exec_builder.str();

    try {
      CreateArgs csa = {};
      csa.description = tunnel_service->service_name_ + L" WireGuard tunnel";
      csa.executable_and_args = service_exec;
      csa.dependencies = L"Nsi\0TcpIp\0";

      tunnel_service->Create(csa);
    } catch (std::exception &e) {
      result->Error(std::string(e.what()));
      return;
    }

    try {
      tunnel_service->Start();
    } catch (std::exception &e) {
      result->Error(std::string(e.what()));
      return;
    }

    result->Success();
    return;
  }

  if (call.method_name() == "disconnect") {
    auto tunnel_service = this->tunnel_service_.get();
    if (tunnel_service == nullptr) {
      result->Error("Invalid state: call 'setupTunnel' first");
      return;
    }

    try {
      tunnel_service->Stop();
    } catch (std::exception &e) {
      result->Error(std::string(e.what()));
    }

    result->Success();
    return;
  }

  result->NotImplemented();
}

}  // namespace wireguard_dart
