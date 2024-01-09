#include "wireguard_flutter_plugin.h"

// This must be included before many other Windows headers.
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <libbase64.h>
#include <windows.h>

#include <memory>
#include <sstream>
#include <wireguard.h>
#include <tunnel.h>

#include "config_writer.h"
#include "service_control.h"
#include "utils.h"

namespace wireguard_flutter
{

  // static
  void WireguardFlutterPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar)
  {
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "billion.group.wireguard_flutter/wgcontrol", &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<WireguardFlutterPlugin>();

    channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto &call, auto result)
                                  { plugin_pointer->HandleMethodCall(call, std::move(result)); });

    registrar->AddPlugin(std::move(plugin));
  }

  WireguardFlutterPlugin::WireguardFlutterPlugin() {}

  WireguardFlutterPlugin::~WireguardFlutterPlugin() {}

  void WireguardFlutterPlugin::HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &call,
                                                std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    const auto *args = std::get_if<flutter::EncodableMap>(call.arguments());

    if (call.method_name() == "initialize")
    {
      const auto *arg_service_name = std::get_if<std::string>(ValueOrNull(*args, "win32ServiceName"));
      if (arg_service_name == nullptr)
      {
        result->Error("Argument 'win32ServiceName' is required");
        return;
      }
      this->tunnel_service_ = std::make_unique<ServiceControl>(Utf8ToWide(*arg_service_name));

      // Disable packet forwarding that conflicts with WireGuard
      ServiceControl remoteAccessService = ServiceControl(L"RemoteAccess");
      try
      {
        remoteAccessService.Stop();
      }
      catch (std::exception &e)
      {
        result->Error(std::string("Could not stop packet forwarding: ").append(e.what()));
        return;
      }
      try
      {
        remoteAccessService.Disable();
      }
      catch (std::exception &e)
      {
        result->Error(std::string("Could not disable packet forwarding: ").append(e.what()));
        return;
      }

      result->Success();
      return;
    }

    if (call.method_name() == "start")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'initialize' first");
        return;
      }
      const auto *wgQuickConfig = std::get_if<std::string>(ValueOrNull(*args, "wgQuickConfig"));
      if (wgQuickConfig == nullptr)
      {
        result->Error("Argument 'wgQuickConfig' is required");
        return;
      }

      std::wstring wg_config_filename;
      try
      {
        wg_config_filename = WriteConfigToTempFile(*wgQuickConfig);
      }
      catch (std::exception &e)
      {
        result->Error(std::string("Could not write wireguard config: ").append(e.what()));
        return;
      }

      wchar_t module_filename[MAX_PATH];
      GetModuleFileName(NULL, module_filename, MAX_PATH);
      auto current_exec_dir = std::wstring(module_filename);
      current_exec_dir = current_exec_dir.substr(0, current_exec_dir.find_last_of(L"\\/"));
      std::wcout << current_exec_dir << std::endl;
      std::wostringstream service_exec_builder;
      service_exec_builder << current_exec_dir << "\\wireguard_svc.exe" << L" -service"
                           << L" -config-file=\"" << wg_config_filename << "\"";
      std::wstring service_exec = service_exec_builder.str();
      std::wcout << service_exec << std::endl;
      try
      {
        CreateArgs csa;
        csa.description = tunnel_service->service_name_ + L" WireGuard tunnel";
        csa.executable_and_args = service_exec;
        csa.dependencies = L"Nsi\0TcpIp\0";

        tunnel_service->Create(csa);
        std::wcout << "create tunnel" << std::endl;
      }
      catch (const std::exception &e)
      {
        result->Error(std::string("Could not create tunnel: ").append(e.what()));
        return;
      }

      try
      {
        tunnel_service->Start();
      }
      catch (std::exception &e)
      {
        result->Error(std::string("Could not start tunnel: ").append(e.what()));
        return;
      }

      result->Success();
      return;
    }

    if (call.method_name() == "stop")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'setupTunnel' first");
        return;
      }

      try
      {
        tunnel_service->Stop();
      }
      catch (std::exception &e)
      {
        result->Error(std::string("Could not stop tunnel").append(e.what()));
        return;
      }

      result->Success();
      return;
    }

    if (call.method_name() == "stage")
    {
      // TODO: implement
      result->Success();
      return;
    }

    result->NotImplemented();
  }

} // namespace wireguard_flutter
