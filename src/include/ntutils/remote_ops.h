// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_REMOTE_OPS_H
#define NTUTILS_REMOTE_OPS_H

#include <boost/utility.hpp>

#include "ntutils/basic.h"
#include "ntutils/WNet_error.h"
#include "ntutils/results.h"

namespace ntutils {

static inline bool GetUser(const_str_ptr resource, string & ret)
{
  string name;
  DWORD size = 64;
  while (true)
  {
    name.resize(size);
    const DWORD err = WNetGetUser(resource, &name[0], &size);
    if (err == ERROR_MORE_DATA)
      size *= 2;
    else if (err == NO_ERROR)
    {
      ret = name.c_str();
      return true;
    }
    else if (err == ERROR_NOT_CONNECTED || err == ERROR_NO_NET_OR_BAD_PATH)
      return false;
    else
    {
      results.report_warning(WNet_error(TEXT("WNetGetUser"), err));
      return false;
    }
  }
}

class remote_login: boost::noncopyable
{
  private:
    string resource;

  public:
    remote_login(const string & nhost, const_str_ptr username, const_str_ptr pwd)
    :resource(TEXT("\\\\") + nhost + TEXT("\\IPC$"))
    {
      // Attempt to determine if there is already a connection
      string existing_user;
      if (GetUser(resource, existing_user))
      {
        // Print a warning if the user specified a username for login
        if (username.ptr() && existing_user != username.ptr())
          results.report_warning(TEXT("Already logged on to ") + nhost + TEXT(" as ") + existing_user + TEXT("; ignoring request to log on as ") + username);
        resource.clear();
        return;
      }

      NETRESOURCE rsc;
      rsc.dwType = RESOURCETYPE_ANY;
      rsc.lpLocalName = 0;
      rsc.lpRemoteName = const_cast<char_t *>(resource.c_str());
      rsc.lpProvider = 0;

      const DWORD err = WNetAddConnection2(&rsc, pwd, username, 0);
      if (err != NO_ERROR)
        throw WNet_error(TEXT("WNetAddConnection2"), err);
    }

    ~remote_login()
    {
      if (resource.empty())
        return;

      const DWORD err = WNetCancelConnection2(resource.c_str(), 0, TRUE);
      if (err != NO_ERROR)
        results.report_warning(WNet_error(TEXT("WNetCancelConnection2"), err));
    }
};

class remote_file: boost::noncopyable
{
  private:
    const string remote_filename;

  public:
    remote_file(const string & host, const_str_ptr file_base_name)
    :remote_filename(TEXT("\\\\") + host + TEXT("\\ADMIN$\\") + file_base_name) { }

    BOOL Copy(const_str_ptr local_filename) const
    { return CopyFile(local_filename, remote_filename.c_str(), FALSE); }

    ~remote_file()
    {
      // DeleteFile will fail if the file is still in use
      // So if it fails, we wait here a short time and try again; if it fails again, then display the error
      if (!DeleteFile(remote_filename.c_str()))
      {
        Sleep(100);
        if (!DeleteFile(remote_filename.c_str()))
          results.report_warning(Win32_error(TEXT("DeleteFile")));
      }
    }
};

struct remote_service_install: service<owned>
{
  ~remote_service_install()
  {
    if (!DeleteService())
      results.report_warning(TEXT("Could not uninstall remote service: ") + Win32_error(TEXT("DeleteService")).twhat() + TEXT("\n"));
  }
};

struct remote_service_start: service<unowned>
{
  ~remote_service_start()
  {
    // Stop the service, and wait until it's stopped, if necessary
    // Note: incorporates synchronous wait
    SERVICE_STATUS status;
    if (!QueryServiceStatus(&status))
    {
      results.report_warning(Win32_error(TEXT("QueryServiceStatus")));
      return;
    }
    if (status.dwCurrentState == SERVICE_STOPPED)
      return;

    // If it's not already stopping, tell it to stop
    if (status.dwCurrentState != SERVICE_STOP_PENDING)
    {
      // There is a small possibility of a race condition here, where
      //  in-between the query above and here the service has stopped,
      //  which will cause the SERVICE_CONTROL_STOP control to fail.
      if (!ControlService(SERVICE_CONTROL_STOP, &status))
      {
        if (!QueryServiceStatus(&status))
        {
          results.report_warning(Win32_error(TEXT("QueryServiceStatus")));
          return;
        }
      }
    }

    // Wait for it to stop
    while (status.dwCurrentState == SERVICE_STOP_PENDING)
    {
      Sleep(100);
      if (!QueryServiceStatus(&status))
      {
        results.report_warning(Win32_error(TEXT("QueryServiceStatus")));
        return;
      }
    }

    // Ensure it did stop
    if (status.dwCurrentState != SERVICE_STOPPED)
      results.report_warning(TEXT("Remote service would not stop"));
  }
};

}

#endif
