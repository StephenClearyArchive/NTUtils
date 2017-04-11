// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef NTUTILS_REMOTE_OPS_H
#define NTUTILS_REMOTE_OPS_H

#include <boost/utility.hpp>

#include "ntutils/basic.h"
#include "ntutils/WNet_error.h"
#include "ntutils/console.h"

namespace ntutils {

class remote_login: boost::noncopyable
{
  private:
    const string resource;

  public:
    remote_login(const string & nhost, const_str_ptr username, const_str_ptr pwd)
    :resource(TEXT("\\\\") + nhost + TEXT("\\IPC$"))
    {
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
      const DWORD err = WNetCancelConnection2(resource.c_str(), 0, TRUE);
      if (err != NO_ERROR)
        tcerr(TEXT("Warning: Could not log out of remote computer: ") + WNet_error(TEXT("WNetCancelConnection2"), err).twhat() + TEXT("\n"));
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
      if (!DeleteFile(remote_filename.c_str()))
        tcerr(TEXT("Warning: Could not delete remote file: ") + Win32_error(TEXT("DeleteFile")).twhat() + TEXT("\n"));
    }
};

struct remote_service_install: service<owned>
{
  ~remote_service_install()
  {
    if (!DeleteService())
      tcerr(TEXT("Warning: Could not uninstall remote service: ") + Win32_error(TEXT("DeleteService")).twhat() + TEXT("\n"));
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
      tcerr(TEXT("Warning: Could not stop remote service: ") + Win32_error(TEXT("QueryServiceStatus")).twhat() + TEXT("\n"));
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
          tcerr(TEXT("Warning: Could not stop remote service: ") + Win32_error(TEXT("QueryServiceStatus")).twhat() + TEXT("\n"));
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
        tcerr(TEXT("Warning: Could not stop remote service: ") + Win32_error(TEXT("QueryServiceStatus")).twhat() + TEXT("\n"));
        return;
      }
    }

    // Ensure it did stop
    if (status.dwCurrentState != SERVICE_STOPPED)
      tcerr(TEXT("Warning: Remote service would not stop\n"));
  }
};

}

#endif
