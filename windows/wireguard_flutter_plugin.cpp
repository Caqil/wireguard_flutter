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
using namespace std;

namespace wireguard_flutter
{

  // static
  void WireguardFlutterPlugin::RegisterWithRegistrar(PluginRegistrarWindows *registrar)
  {
    auto channel = make_unique<MethodChannel<EncodableValue>>(
        registrar->messenger(), "billion.group.wireguard_flutter/wgcontrol", &StandardMethodCodec::GetInstance());
    auto eventChannel = make_unique<EventChannel<EncodableValue>>(
        registrar->messenger(), "billion.group.wireguard_flutter/wgstage", &StandardMethodCodec::GetInstance());

    auto plugin = make_unique<WireguardFlutterPlugin>();

    channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto &call, auto result)
                                  { plugin_pointer->HandleMethodCall(call, move(result)); });

    auto eventsHandler = make_unique<StreamHandlerFunctions<EncodableValue>>(
        [plugin_pointer = plugin.get()](
            const EncodableValue *arguments,
            unique_ptr<EventSink<EncodableValue>> &&events)
            -> unique_ptr<StreamHandlerError<EncodableValue>>
        {
          return plugin_pointer->OnListen(arguments, move(events));
        },
        [plugin_pointer = plugin.get()](const EncodableValue *arguments)
            -> unique_ptr<StreamHandlerError<EncodableValue>>
        {
          return plugin_pointer->OnCancel(arguments);
        });

    eventChannel->SetStreamHandler(move(eventsHandler));

    registrar->AddPlugin(move(plugin));
  }

  WireguardFlutterPlugin::WireguardFlutterPlugin() {}

  WireguardFlutterPlugin::~WireguardFlutterPlugin() {}

  void WireguardFlutterPlugin::HandleMethodCall(const MethodCall<EncodableValue> &call,
                                                unique_ptr<MethodResult<EncodableValue>> result)
  {
    const auto *args = get_if<EncodableMap>(call.arguments());

    if (call.method_name() == "initialize")
    {
      const auto *arg_service_name = get_if<string>(ValueOrNull(*args, "win32ServiceName"));
      if (arg_service_name == NULL)
      {
        result->Error("Argument 'win32ServiceName' is required");
        return;
      }
      if (this->tunnel_service_ != nullptr)
      {
        this->tunnel_service_->service_name_ = Utf8ToWide(*arg_service_name);
      }
      else
      {
        this->tunnel_service_ = make_unique<ServiceControl>(Utf8ToWide(*arg_service_name));
        this->tunnel_service_->RegisterListener(move(events_));
      }

      result->Success();
      return;
    }
    else if (call.method_name() == "start")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'initialize' first");
        return;
      }
      const auto *wgQuickConfig = get_if<string>(ValueOrNull(*args, "wgQuickConfig"));
      if (wgQuickConfig == NULL)
      {
        result->Error("Argument 'wgQuickConfig' is required");
        return;
      }

      this->tunnel_service_->EmitState("prepare");

      wstring wg_config_filename;
      try
      {
        wg_config_filename = WriteConfigToTempFile(*wgQuickConfig);
      }
      catch (exception &e)
      {
        this->tunnel_service_->EmitState("no_connection");
        result->Error(string("Could not write wireguard config: ").append(e.what()));
        return;
      }

      wchar_t module_filename[MAX_PATH];
      GetModuleFileName(NULL, module_filename, MAX_PATH);
      auto current_exec_dir = wstring(module_filename);
      current_exec_dir = current_exec_dir.substr(0, current_exec_dir.find_last_of(L"\\/"));
      wostringstream service_exec_builder;
      service_exec_builder << current_exec_dir << "\\wireguard_svc.exe" << L" -service"
                           << L" -config-file=\"" << wg_config_filename << "\"";
      wstring service_exec = service_exec_builder.str();
      cout << "Starting service with command line: " << WideToAnsi(service_exec) << endl;
      try
      {
        CreateArgs csa;
        csa.description = tunnel_service->service_name_ + L" WireGuard tunnel";
        csa.executable_and_args = service_exec;
        csa.dependencies = L"Nsi\0TcpIp\0";
        csa.first_time = true;

        tunnel_service->CreateAndStart(csa);
      }
      catch (exception &e)
      {
        result->Error(string(e.what()));
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
        result->Error("Invalid state: call 'initialize' first");
        return;
      }

      try
      {
        tunnel_service->Stop();
      }
      catch (exception &e)
      {
        result->Error(string(e.what()));
      }

      result->Success();
      return;
    }
    else if (call.method_name() == "stage")
    {
      auto tunnel_service = this->tunnel_service_.get();
      if (tunnel_service == nullptr)
      {
        result->Error("Invalid state: call 'initialize' first");
        return;
      }

      result->Success(tunnel_service->GetStatus());
      return;
    }

    result->NotImplemented();
  }

  unique_ptr<StreamHandlerError<EncodableValue>> WireguardFlutterPlugin::OnListen(
      const EncodableValue *arguments,
      unique_ptr<EventSink<EncodableValue>> &&events)
  {
    events_ = move(events);
    auto tunnel_service = this->tunnel_service_.get();
    if (tunnel_service != nullptr)
    {
      tunnel_service->RegisterListener(move(events_));
      return nullptr;
    }

    return nullptr;
  }

  unique_ptr<StreamHandlerError<EncodableValue>> WireguardFlutterPlugin::OnCancel(
      const EncodableValue *arguments)
  {
    events_ = nullptr;
    auto tunnel_service = this->tunnel_service_.get();
    if (tunnel_service != nullptr)
    {
      tunnel_service->UnregisterListener();
      return nullptr;
    }

    return nullptr;
  }

} // namespace wireguard_flutter
