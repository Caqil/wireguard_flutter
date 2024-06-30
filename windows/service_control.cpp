#include "service_control.h"

#include <flutter/standard_method_codec.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/encodable_value.h>

#include <windows.h>

#include <stdexcept>
#include <string>

#include "utils.h"
#include <iostream>

namespace wireguard_flutter
{

  class ServiceControlException : public std::exception
  {
  private:
    std::string message_;
    unsigned long error_code_;

  public:
    explicit ServiceControlException(const std::string &msg) : message_(msg), error_code_(0) {}

    ServiceControlException(const std::string &msg, unsigned long errc) : message_(msg), error_code_(errc) {}

    const char *what() const noexcept override
    {
      std::string s = message_ + " (" + std::to_string(error_code_) + ")";
      return s.c_str();
    }

    unsigned long GetErrorCode() const noexcept
    {
      return error_code_;
    }
  };

  void ServiceControl::CreateAndStart(CreateArgs args) {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL) {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }
  
    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_ALL_ACCESS);
    if (service == NULL) {
      // Create a new service
      service = CreateService(service_manager,                   // SCM database
                              &service_name_[0],                 // name of service
                              &service_name_[0],                 // service name to display
                              SERVICE_ALL_ACCESS,                // desired access
                              SERVICE_WIN32_OWN_PROCESS,         // service type
                              SERVICE_DEMAND_START,              // start type
                              SERVICE_ERROR_NORMAL,              // error control type
                              args.executable_and_args.c_str(),  // path to service's binary
                              NULL,                              // no load ordering group
                              NULL,                              // no tag identifier
                              args.dependencies.c_str(),         // dependencies
                              NULL,                              // LocalSystem account
                              NULL);
      if (service == NULL) {
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to create the service", GetLastError());
      }
    } else {
      // Attempt to re-configure existing service
      if (!ChangeServiceConfig(service, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                               args.executable_and_args.c_str(), NULL, NULL, args.dependencies.c_str(), NULL, NULL,
                               &service_name_[0])) {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to re-configure the service", GetLastError());
      }
    }
  
    auto sid_type = SERVICE_SID_TYPE_UNRESTRICTED;
    if (!ChangeServiceConfig2(service, SERVICE_CONFIG_SERVICE_SID_INFO, &sid_type)) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to configure service SID type", GetLastError());
    }
  
    SERVICE_DESCRIPTION description = {&args.description[0]};
    if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &description)) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to configure service description", GetLastError());
    }
  
    // Query service status
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;
  
    if (!QueryServiceStatusEx(
            service,                        // handle to service
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))                // size needed if buffer is too small
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      EmitState("denied");
      std::cout << "wireguard_flutter: Failed to query service status" << GetLastError() << std::endl;
      return;
    }
  
    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      EmitState("connected");
      std::cout << "wireguard_flutter: Service is already running" << GetLastError() << std::endl;
      return;
    }
  
    EmitState("connecting");
  
    if (!StartService(service, 0, NULL)) {
      std::cout << "wireguard_flutter: Failed to start the service: " << GetLastError() << std::endl;
  
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      EmitState("denied");
      throw ServiceControlException("Failed to start the service", GetLastError());
    }
  
    // Wait for the service to start
    DWORD dwWaitTime = ssStatus.dwWaitHint / 10;
    if (dwWaitTime < 1000)
      dwWaitTime = 1000;
    else if (dwWaitTime > 1500)
      dwWaitTime = 1500;
    Sleep(dwWaitTime);
  
    if (GetStatus() == "disconnected") {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      EmitState("denied");
      std::cout << "wireguard_flutter: Failed to start the service" << GetLastError() << std::endl;
      throw ServiceControlException("Failed to start the service", GetLastError());
    }
  
    EmitState("connected");
  
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
  }

  void ServiceControl::Stop()
  {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      return;
    }

    EmitState("disconnecting");

    SERVICE_STATUS_PROCESS service_status;
    DWORD service_status_bytes_needed;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                              &service_status_bytes_needed))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to query service status", GetLastError());
    }
    if (service_status.dwCurrentState == SERVICE_STOPPED)
    {
      EmitState("disconnected");
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    DWORD start_time = GetTickCount();
    DWORD wait_for_stop_timeout = 15000;
    DWORD wait_time;
    while (service_status.dwCurrentState == SERVICE_STOP_PENDING)
    {
      // Sometimes dwWaitHint gives unreasonably large values (e.g. 120s), thus just checking every 1s.
      // Original logic:
      // wait_time = service_status.dwWaitHint / 10;
      // if (wait_time < 1000)
      //   wait_time = 1000;
      // else if (wait_time > 10000)
      //   wait_time = 10000;
      wait_time = 1000;

      Sleep(wait_time);

      if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                                &service_status_bytes_needed))
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to query service status when stop pending", GetLastError());
      }

      if (service_status.dwCurrentState == SERVICE_STOPPED)
      {
        EmitState("disconnected");
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return;
      }

      if (GetTickCount() - start_time > wait_for_stop_timeout)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Disconnect timed out");
      }
    }
    if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&service_status))
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Stop service command failed", GetLastError());
    }

    while (service_status.dwCurrentState != SERVICE_STOPPED)
    {
      // Sometimes dwWaitHint gives unreasonably large values (e.g. 120s), thus just checking every 1s.
      // Original logic:
      // Sleep(service_status.dwWaitHint);
      Sleep(1000);

      if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                                &service_status_bytes_needed))
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Failed to query service status after issuing stop command", GetLastError());
      }

      if (service_status.dwCurrentState == SERVICE_STOPPED)
      {
        EmitState("disconnected");
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        return;
      }

      if (GetTickCount() - start_time > wait_for_stop_timeout)
      {
        CloseServiceHandle(service);
        CloseServiceHandle(service_manager);
        throw ServiceControlException("Disconnect timed out");
      }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
  }

  std::string ServiceControl::GetStatus()
  {
    SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (service_manager == NULL)
    {
      throw ServiceControlException("Failed to open service manager", GetLastError());
    }

    SC_HANDLE service = OpenService(service_manager, &service_name_[0], SC_MANAGER_ALL_ACCESS);
    if (service == NULL)
    {
      CloseServiceHandle(service_manager);
      CloseServiceHandle(service);
      return "disconnected";
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    if (!QueryServiceStatusEx(
            service,                        // handle to service
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))                // size needed if buffer is too small
    {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return "denied";
    }

    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);

    if (ssStatus.dwCurrentState == SERVICE_STOPPED || ssStatus.dwCurrentState == SERVICE_PAUSED)
    {
      return "disconnected";
    }
    else if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING || ssStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
    {
      return "disconnecting";
    }
    else if (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
      return "connecting";
    }
    else if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
      return "connected";
    }
    else if (ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING)
    {
      return "reconnecting";
    }

    return "no_connection";
  }

  void ServiceControl::RegisterListener(std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
  {
    if (events_ == nullptr)
    {
      events_ = std::move(events);
    }
  }

  void ServiceControl::UnregisterListener()
  {
    events_ = nullptr;
  }

  void ServiceControl::EmitState(std::string state)
  {
    if (events_ == nullptr)
    {
      return;
    }
    events_->Success(flutter::EncodableValue(state));
  }

} // namespace wireguard_flutter
