#include "wireguard_flutter_plugin.h"

// This must be included before many other Windows headers.
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/encodable_value.h>
#include <libbase64.h>
#include <windows.h>

#include <memory>
#include <sstream>

#include "config_writer.h"
#include "service_control.h"
#include "utils.h"

using namespace flutter;

namespace wireguard_flutter
{

  // static
  void WireguardFlutterPlugin::RegisterWithRegistrar(PluginRegistrarWindows *registrar)
  {
    auto channel = std::make_unique<MethodChannel<EncodableValue>>(
        registrar->messenger(), "billion.group.wireguard_flutter/wgcontrol", &StandardMethodCodec::GetInstance());
    auto eventChannel = std::make_unique<EventChannel<EncodableValue>>(
        registrar->messenger(), "billion.group.wireguard_flutter/wgstage", &StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<WireguardFlutterPlugin>();

    channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto &call, auto result)
                                  { plugin_pointer->HandleMethodCall(call, std::move(result)); });

    auto eventsHandler = std::make_unique<StreamHandlerFunctions<EncodableValue>>(
        [plugin_pointer = plugin.get()](
            const EncodableValue *arguments,
            std::unique_ptr<EventSink<EncodableValue>> &&events)
            -> std::unique_ptr<StreamHandlerError<EncodableValue>>
        {
          return plugin_pointer->OnListen(arguments, std::move(events));
        },
        [plugin_pointer = plugin.get()](const EncodableValue *arguments)
            -> std::unique_ptr<StreamHandlerError<EncodableValue>>
        {
          return plugin_pointer->OnCancel(arguments);
        });

    eventChannel->SetStreamHandler(std::move(eventsHandler));

    registrar->AddPlugin(std::move(plugin));
  }

  WireguardFlutterPlugin::WireguardFlutterPlugin() {}

  WireguardFlutterPlugin::~WireguardFlutterPlugin() {}

  void WireguardFlutterPlugin::HandleMethodCall(const MethodCall<EncodableValue> &call,
                                                std::unique_ptr<MethodResult<EncodableValue>> result)
  {
    const auto *args = std::get_if<EncodableMap>(call.arguments());

    if (call.method_name() == "initialize")
    {
      const auto *arg_service_name = std::get_if<std::string>(ValueOrNull(*args, "win32ServiceName"));
      if (arg_service_name == NULL)
      {
        result->Error("Argument 'win32ServiceName' is required");
        return;
      }
      this->tunnel_service_ = std::make_unique<ServiceControl>(Utf8ToWide(*arg_service_name));
      result->Success();
      return;
    }
    else if (call.method_name() == "start")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'setupTunnel' first");
        return;
      }
      const auto *wgQuickConfig = std::get_if<std::string>(ValueOrNull(*args, "wgQuickConfig"));
      if (wgQuickConfig == NULL)
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
      std::wostringstream service_exec_builder;
      service_exec_builder << current_exec_dir << "\\wireguard_svc.exe" << L" -service"
                           << L" -config-file=\"" << wg_config_filename << "\"";
      std::wstring service_exec = service_exec_builder.str();
      std::cout << "Starting service with command line: " << WideToAnsi(service_exec) << std::endl;
      try
      {
        CreateArgs csa;
        csa.description = tunnel_service->service_name_ + L" WireGuard tunnel";
        csa.executable_and_args = service_exec;
        csa.dependencies = L"Nsi\0TcpIp\0";

        tunnel_service->CreateAndStart(csa);
      }
      catch (std::exception &e)
      {
        result->Error(std::string(e.what()));
        return;
      }

      result->Success();
      return;
    }
    else if (call.method_name() == "stop")
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
        result->Error(std::string(e.what()));
      }

      result->Success();
      return;
    }
    else if (call.method_name() == "stage")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'setupTunnel' first");
        return;
      }

      result->Success(tunnel_service->GetStatus());
      return;
    }

    result->NotImplemented();
  }

  std::unique_ptr<StreamHandlerError<EncodableValue>> WireguardFlutterPlugin::OnListen(
      const EncodableValue *arguments,
      std::unique_ptr<EventSink<EncodableValue>> &&events)
  {
    std::cout << "OnListen" << std::endl;

    return nullptr;
  }

  std::unique_ptr<StreamHandlerError<EncodableValue>> WireguardFlutterPlugin::OnCancel(
      const EncodableValue *arguments)
  {
    return nullptr;
  }

} // namespace wireguard_dart
