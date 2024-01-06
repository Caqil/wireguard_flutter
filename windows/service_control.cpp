#include "service_control.h"

#include <windows.h>

#include <stdexcept>
#include <string>

#include "utils.h"

namespace wireguard_flutter {

class ServiceControlException : public std::exception {
 private:
  char *message_;
  unsigned long *error_code_;

 public:
  ServiceControlException(char *msg) : message_(msg) {}
  ServiceControlException(char *msg, unsigned long errc) : message_(msg), error_code_(&errc) {}
  const char *what() {
    if (error_code_ != NULL) {
      return ErrorWithCode(message_, *error_code_).c_str();
    }
    return message_;
  }
};

void ServiceControl::Create(CreateArgs args) {
  SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (service_manager == NULL) {
    throw ServiceControlException("Failed to open service manager", GetLastError());
  }

  SC_HANDLE service = OpenService(service_manager, &service_name_[0], SC_MANAGER_ALL_ACCESS);
  if (service != NULL) {
    DeleteService(service);
    CloseServiceHandle(service);
  }

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
                          args.dependencies.c_str(),
                          NULL,  // LocalSystem account
                          NULL);
  if (service == NULL) {
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to create the service", GetLastError());
  }

  auto sid_type = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service, SERVICE_CONFIG_SERVICE_SID_INFO, &sid_type)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to configure servivce SID type", GetLastError());
  }

  SERVICE_DESCRIPTION description = {&args.description[0]};
  if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &description)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to configure service description", GetLastError());
  }

  CloseServiceHandle(service);
  CloseServiceHandle(service_manager);
}

void ServiceControl::Start() {
  SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (service_manager == NULL) {
    throw ServiceControlException("Failed to open service manager", GetLastError());
  }

  SC_HANDLE service = OpenService(service_manager, this->service_name_.c_str(), SC_MANAGER_ALL_ACCESS);
  if (service == NULL) {
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to start: service does not exist");
  }

  if (!StartService(service, 0, NULL)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to start the service", GetLastError());
  }

  CloseServiceHandle(service);
  CloseServiceHandle(service_manager);
}

void ServiceControl::Stop() {
  SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (service_manager == NULL) {
    throw ServiceControlException("Failed to open service manager", GetLastError());
  }

  SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_STOP | SERVICE_QUERY_STATUS);
  if (service == NULL) {
    CloseServiceHandle(service_manager);
    return;
  }

  SERVICE_STATUS_PROCESS service_status;
  DWORD service_status_bytes_needed;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                            &service_status_bytes_needed)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to query service status", GetLastError());
  }
  if (service_status.dwCurrentState == SERVICE_STOPPED) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    return;
  }

  DWORD start_time = GetTickCount();
  DWORD wait_for_stop_timeout = 15000;
  DWORD wait_time;
  while (service_status.dwCurrentState == SERVICE_STOP_PENDING) {
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
                              &service_status_bytes_needed)) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to query service status when stop pending", GetLastError());
    }

    if (service_status.dwCurrentState == SERVICE_STOPPED) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    if (GetTickCount() - start_time > wait_for_stop_timeout) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Disconnect timed out");
    }
  }
  if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&service_status)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Stop service command failed", GetLastError());
  }

  while (service_status.dwCurrentState != SERVICE_STOPPED) {
    // Sometimes dwWaitHint gives unreasonably large values (e.g. 120s), thus just checking every 1s.
    // Original logic:
    // Sleep(service_status.dwWaitHint);
    Sleep(1000);

    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&service_status, sizeof(SERVICE_STATUS_PROCESS),
                              &service_status_bytes_needed)) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Failed to query service status after issuing stop command", GetLastError());
    }

    if (service_status.dwCurrentState == SERVICE_STOPPED) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      return;
    }

    if (GetTickCount() - start_time > wait_for_stop_timeout) {
      CloseServiceHandle(service);
      CloseServiceHandle(service_manager);
      throw ServiceControlException("Disconnect timed out");
    }
  }

  CloseServiceHandle(service);
  CloseServiceHandle(service_manager);
}

void ServiceControl::Disable() {
  SC_HANDLE service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (service_manager == NULL) {
    throw ServiceControlException("Failed to open service manager", GetLastError());
  }

  SC_HANDLE service = OpenService(service_manager, &service_name_[0], SERVICE_CHANGE_CONFIG);
  if (service == NULL) {
    CloseServiceHandle(service_manager);
    return;
  }

  auto service_start_type = SERVICE_DISABLED;
  if (!ChangeServiceConfig(service, SERVICE_NO_CHANGE, service_start_type, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL)) {
    CloseServiceHandle(service);
    CloseServiceHandle(service_manager);
    throw ServiceControlException("Failed to disable service", GetLastError());
  }
}

}  // namespace wireguard_dart
