// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef BASIC_SERVICE_H
#define BASIC_SERVICE_H

#include "basic/handle.h"

namespace basic {

template <typename Owned = unowned>
struct service: handle_base<service<Owned>, SC_HANDLE>
{
  typedef handle_base<service<Owned>, SC_HANDLE> base_type;
  TBA_DEFINE_HANDLE_CLASS_VALUE(service, SC_HANDLE, 0)

  static SC_HANDLE InvalidValue() { return 0; }
  bool Valid() const { return (this->Handle() != InvalidValue()); }

  BOOL Close() { return CloseServiceHandle(this->Handle()); }
  bool close() { if (!Close()) throw Win32_error(TEXT("CloseServiceHandle")); }

  void OpenSCManager(const_str_ptr host = TEXT(""),
      const DWORD access = SC_MANAGER_ALL_ACCESS,
      const char_t * const database = SERVICES_ACTIVE_DATABASE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::OpenSCManager(host, database, access));
  }
  void open_sc_manager(const_str_ptr host = TEXT(""),
      const DWORD access = SC_MANAGER_ALL_ACCESS,
      const char_t * const database = SERVICES_ACTIVE_DATABASE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const SC_HANDLE nhandle = ::OpenSCManager(host, database, access);
    if (nhandle == 0)
      throw Win32_error(TEXT("OpenSCManager"));
    this->Reset(nhandle);
  }

  void OpenService(const SC_HANDLE & manager, const_str_ptr name,
      const DWORD access = SERVICE_ALL_ACCESS)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::OpenService(manager, name, access));
  }
  void open_service(const SC_HANDLE & manager, const_str_ptr name,
      const DWORD access = SERVICE_ALL_ACCESS)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const SC_HANDLE nhandle = ::OpenService(manager, name, access);
    if (nhandle == 0)
      throw Win32_error(TEXT("OpenService"));
    this->Reset(nhandle);
  }

  void CreateService(const SC_HANDLE & manager, const_str_ptr name,
      const_str_ptr display_name, const DWORD access, const DWORD service_type,
      const DWORD start_type, const DWORD error_control,
      const_str_ptr binary_path_name, const_str_ptr load_order_group = const_str_ptr(),
      DWORD * tag_id = 0, const_str_ptr dependencies = const_str_ptr(),
      const_str_ptr service_start_name = const_str_ptr(), const_str_ptr password = const_str_ptr())
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::CreateService(manager, name, display_name,
        access, service_type, start_type, error_control, binary_path_name,
        load_order_group, tag_id, dependencies, service_start_name, password));
  }
  void create_service(const SC_HANDLE & manager, const_str_ptr name,
      const_str_ptr display_name, const DWORD access, const DWORD service_type,
      const DWORD start_type, const DWORD error_control,
      const_str_ptr binary_path_name, const_str_ptr load_order_group = const_str_ptr(),
      DWORD * tag_id = 0, const_str_ptr dependencies = const_str_ptr(),
      const_str_ptr service_start_name = const_str_ptr(), const_str_ptr password = const_str_ptr())
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const SC_HANDLE nhandle = ::CreateService(manager, name, display_name,
        access, service_type, start_type, error_control, binary_path_name,
        load_order_group, tag_id, dependencies, service_start_name, password);
    if (nhandle == 0)
      throw Win32_error(TEXT("CreateService"));
    this->Reset(nhandle);
  }

  BOOL StartService(const DWORD argc = 0, const char_t * * const argv = 0) const
  { return ::StartService(this->Handle(), argc, argv); }
  void start_service(const DWORD argc = 0, const char_t * * const argv = 0) const
  {
    if (!StartService(argc, argv))
      throw Win32_error(TEXT("StartService"));
  }

  BOOL QueryServiceStatus(SERVICE_STATUS * const ret) const
  { return ::QueryServiceStatus(this->Handle(), ret); }
  SERVICE_STATUS query_service_status() const
  {
    SERVICE_STATUS ret;
    if (!QueryServiceStatus(&ret))
      throw Win32_error(TEXT("QueryServiceStatus"));
    return ret;
  }

  BOOL ControlService(const DWORD control, SERVICE_STATUS * const status = 0) const
  { return ::ControlService(this->Handle(), control, status); }
  void control_service(const DWORD control, SERVICE_STATUS * const status = 0) const
  {
    if (!ControlService(control, status))
      throw Win32_error(TEXT("ControlService"));
  }

  BOOL DeleteService() const { return ::DeleteService(this->Handle()); }
  void delete_service() const
  {
    if (!DeleteService())
      throw Win32_error(TEXT("DeleteService"));
  }
};

}

#endif
