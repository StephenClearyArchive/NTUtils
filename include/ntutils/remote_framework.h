// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_REMOTE_FRAMEWORK_H
#define NTUTILS_REMOTE_FRAMEWORK_H

#include <boost/scoped_array.hpp>

#include "ntutils/console.h"
#include "ntutils/remote_ops.h"
#include "ntutils/sid.h"

namespace ntutils {

template <typename Derived>
class server_framework
{
  private:
    static SERVICE_STATUS_HANDLE service_status_handle;
    static SERVICE_STATUS service_status;

    static void set_service_status()
    {
      if (!SetServiceStatus(service_status_handle, &service_status))
        throw Win32_error(TEXT("SetServiceStatus"));
    }

    static VOID WINAPI service_control_handler(const DWORD) { SetServiceStatus(service_status_handle, &service_status); }
    static VOID WINAPI service_main(DWORD, LPTSTR *)
    {
      try
      {
        ZeroMemory(&service_status, sizeof(service_status));
        service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

        service_status.dwWaitHint = 100;
        service_status.dwCurrentState = SERVICE_START_PENDING;

        service_status_handle = RegisterServiceCtrlHandler(TEXT(""), &service_control_handler);
        if (service_status_handle == 0)
          throw Win32_error(TEXT("RegisterServiceCtrlHandler"));

        set_service_status();

        // We use a security descriptor with a DACL that prevents everyone (except the creator/owner)
        //  from taking ownership or changing the DACL, but allows everyone to do anything else;
        //  this allows proper named pipe connections to NT machines while maintaining security.
        sid<owned> everyone;
        SID_IDENTIFIER_AUTHORITY world_authority = SECURITY_WORLD_SID_AUTHORITY;
        everyone.allocate_and_initialize_sid(&world_authority, 1, SECURITY_WORLD_RID);
        const DWORD acl_size = everyone.GetLengthSid() * 2 + sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACCESS_DENIED_ACE) - sizeof(DWORD) * 2 + sizeof(ACL);
        boost::scoped_array<char> acl_buffer(new char[acl_size]);
        const PACL acl = (PACL) acl_buffer.get();
        if (!InitializeAcl(acl, acl_size, ACL_REVISION))
          throw Win32_error(TEXT("InitializeAcl"));
        if (!AddAccessDeniedAce(acl, ACL_REVISION, WRITE_OWNER | WRITE_DAC, everyone.Handle()))
          throw Win32_error(TEXT("AddAccessDeniedAce"));
        if (!AddAccessAllowedAce(acl, ACL_REVISION, GENERIC_ALL, everyone.Handle()))
          throw Win32_error(TEXT("AddAccessAllowedAce"));

        SECURITY_DESCRIPTOR sd;
        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
          throw Win32_error(TEXT("InitializeSecurityDescriptor"));
        if (!SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE))
          throw Win32_error(TEXT("SetSecurityDescriptorDacl"));

        SECURITY_ATTRIBUTES sa;
        ZeroMemory(&sa, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = &sd;

        named_pipe<owned> pipe;
        pipe.create_named_pipe(TEXT("\\\\.\\pipe\\TBA:") + Derived::name(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 0, 0, INFINITE, &sa);

        service_status.dwWaitHint = 0;
        service_status.dwCurrentState = SERVICE_RUNNING;
        set_service_status();

        // Wait for connection
        pipe.connect_named_pipe();

        // Wait for a message to arrive on the pipe
        DWORD junk;
        pipe.ReadFile(0, 0, junk);

        // Read in the message
        const DWORD msg_size = pipe.peek_msg();
        if (msg_size == 0)
          throw error(TEXT("Invalid message received: no action"));
        boost::scoped_array<char> msg(new char[msg_size]);
        if (pipe.read_file_sync(msg.get(), msg_size) != msg_size)
          throw error(TEXT("Improper message size returned from ReadFile"));

        // Impersonate, for security purposes
        pipe.impersonate_named_pipe_client();

        // Ensure that the impersonation has an effect
        token<owned> token;
        token.open_thread_token(GetCurrentThread(), TOKEN_QUERY);
        if (token.get_token_impersonation_level() < SecurityImpersonation)
          throw error(TEXT("Restricted impersonation level detected"));

        // Obey the message: yes, master, I hear and will obey
        const string response = Derived::handle_message(msg, msg_size);

        // Turn off impersonation
        RevertToSelf();

        // Send the response on the pipe
        pipe.write_file_sync(response.data(), response.size());

        // Make sure the client gets all the response before terminating
        pipe.flush_file_buffers();
        pipe.disconnect_named_pipe();

        service_status.dwCurrentState = SERVICE_STOPPED;
        set_service_status();
      }
      catch (const Win32_error & e)
      {
        ods(Derived::name() + TEXT(": ") + e.twhat());
        service_status.dwWin32ExitCode = e.code;
        service_status.dwWaitHint = 0;
        service_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(service_status_handle, &service_status);
      }
      catch (const error & e)
      {
        ods(Derived::name() + TEXT(": ") + e.twhat());
        service_status.dwWin32ExitCode = 0;
        service_status.dwWaitHint = 0;
        service_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(service_status_handle, &service_status);
      }
      catch (const std::exception & e)
      {
        ods(Derived::name() + TEXT(": ") + to_string(e.what()));
        service_status.dwWin32ExitCode = 0;
        service_status.dwWaitHint = 0;
        service_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(service_status_handle, &service_status);
      }
    }

  public:
    static int start()
    {
      SERVICE_TABLE_ENTRY services[] = { { TEXT(""), &service_main }, { 0, 0 } };
      return StartServiceCtrlDispatcher(services);
    }
};

template <typename Derived>
SERVICE_STATUS_HANDLE server_framework<Derived>::service_status_handle;
template <typename Derived>
SERVICE_STATUS server_framework<Derived>::service_status;

static inline string get_password()
{
  // Prompt for password
  tcout(TEXT("Password: "));

  // Turn off local echo, so password is not seen

  const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  if (in == INVALID_HANDLE_VALUE || in == 0)
    throw Win32_error(TEXT("GetStdHandle"));

  DWORD mode;
  if (!GetConsoleMode(in, &mode))
    throw Win32_error(TEXT("GetConsoleMode"));

  if (!SetConsoleMode(in, mode & ~ENABLE_ECHO_INPUT))
    throw Win32_error(TEXT("SetConsoleMode"));

  const string ret = read_from_console();

  // Restore the input mode to whatever state it was in before
  if (!SetConsoleMode(in, mode))
    throw Win32_error(TEXT("SetConsoleMode"));

  // Echo just a carriage return
  tcout(TEXT("\n"));

  return ret;
}

template <typename Derived>
struct client_framework
{
  // Command-line options
  string computer, username, password;
  bool password_specified, prompt_for_password;

  client_framework()
  :password_specified(false), prompt_for_password(false) { }

  void handle_option(const option_parser & options)
  {
    switch (options.option->short_option)
    {
      case TEXT('c'):
        computer = options.argument;
        break;
      case TEXT('u'):
        username = options.argument;
        break;
      case TEXT('p'):
        password_specified = true;
        if (options.argument)
          password = options.argument;
        else
          prompt_for_password = true;
        break;
    }
  }

  bool is_remote() const { return (!computer.empty()); }

  void start(const string & msg, const DWORD max_response_size)
  {
    const string ntutils_name = TEXT("ntutils.") + Derived::name();

    // Prompt for password if necessary
    if (prompt_for_password)
      password = get_password();

    // Log into remote computer
    //  (Log out when this object goes out of scope)
    remote_login login(computer, username.empty() ? 0 : username.c_str(), password_specified ? password.c_str() : 0);

    // Copy this currently-running exe file onto remote computer (displaying but ignoring errors)
    //  (Delete the remote file when this object goes out of scope)
    remote_file file(computer, ntutils_name + TEXT(".exe"));
    if (!file.Copy(get_module_file_name()))
      tcerr(TEXT("Warning: ") + Win32_error(TEXT("CopyFile")).twhat() + TEXT("\n"));

    // Connect to the remote service control manager
    service<owned> scm;
    scm.open_sc_manager(computer, SC_MANAGER_CREATE_SERVICE);

    // Install the remote service (allowing the service to already be installed)
    //  (Uninstall the remote service when this object goes out of scope)
    remote_service_install install;
    install.OpenService(scm.Handle(), ntutils_name);
    if (install.Valid())
      tcerr(TEXT("Warning: service already existed\n"));
    else
    {
      install.create_service(scm.Handle(), ntutils_name, TEXT(""), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
          SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, TEXT("%SystemRoot%\\") + ntutils_name + TEXT(".exe service"));
    }

    // Start up the service (allowing the service to already be running)
    //  (Stop the service when this object goes out of scope)
    remote_service_start service;
    service.Reset(install.Handle());
    if (!service.StartService())
      tcerr(TEXT("Warning: could not start service: ") + Win32_error(TEXT("StartService")).twhat() + TEXT("\n"));

    // Wait for the remote service to be running
    SERVICE_STATUS status = service.query_service_status();
    while (status.dwCurrentState == SERVICE_START_PENDING)
    {
      Sleep(100);
      status = service.query_service_status();
    }

    if (status.dwCurrentState != SERVICE_RUNNING)
      throw error(TEXT("Could not start remote service"));

    // Connect to the service's named pipe, send the message, and receive the response
    boost::scoped_array<char> response(new char[max_response_size]);
    DWORD response_size;
    if (!CallNamedPipe((TEXT("\\\\") + computer + TEXT("\\pipe\\TBA:") + Derived::name()).c_str(), (void *) &msg[0], msg.length(),
        response.get(), max_response_size, &response_size, NMPWAIT_WAIT_FOREVER))
      throw Win32_error(TEXT("CallNamedPipe"));

    // Output the response locally
    tcout(string(response.get(), response.get() + response_size));
  }
};

}

#endif
