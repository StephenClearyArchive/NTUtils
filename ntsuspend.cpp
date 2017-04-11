// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#include <windows.h>

#include <boost/array.hpp>
#include <boost/scoped_array.hpp>

#include "ntutils/ntsuspend.inc"
#include "ntutils/console.h"
#include "ntutils/remote_ops.h"

static const DWORD max_response_size = 8192;

int usage()
{
  tcerr(TEXT("Usage: ntsuspend [options]\n"));
  tcerr(TEXT("Options:\n"));
  tcerr(TEXT("  -h [ --help ]           : Display this information\n"));
  tcerr(TEXT("  -i [ --pid ] arg        : Specify process id\n"));
  tcerr(TEXT("  -n [ --name ] arg       : Specify process name\n"));
  tcerr(TEXT("  -s [ --substr ]         :   Process name is a substring match\n"));
  tcerr(TEXT("  -r [ --resume ]         : Resume instead of suspend\n"));
  tcerr(TEXT("  -t [ --test ]           : Test process(es) for suspension\n"));
  tcerr(TEXT("  -c [ --computer ] arg   : Execute on remote computer\n"));
  tcerr(TEXT("  -u [ --username ] arg   :   Username for remote computer\n"));
  tcerr(TEXT("  -p [ --password ] [arg] :   Password for remote computer\n"));
  return 1;
}

SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;

VOID WINAPI service_ctrl_handler(const DWORD code)
{
  SetServiceStatus(service_status_handle, &service_status);
}

VOID WINAPI service_main(DWORD, LPTSTR *)
{
  try
  {
    ZeroMemory(&service_status, sizeof(service_status));
    service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    service_status.dwWaitHint = 100;
    service_status.dwCurrentState = SERVICE_START_PENDING;

    service_status_handle = RegisterServiceCtrlHandler(TEXT(""), &service_ctrl_handler);
    if (service_status_handle == 0)
      throw Win32_error(TEXT("RegisterServiceCtrlHandler"));

    if (!SetServiceStatus(service_status_handle, &service_status))
      throw Win32_error(TEXT("SetServiceStatus"));

    // For now, we use a security descriptor with a null DACL; this allows proper named pipe connections to NT machines.

    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
      throw Win32_error(TEXT("InitializeSecurityDescriptor"));
    if (!SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE))
      throw Win32_error(TEXT("SetSecurityDescriptorDacl"));

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    named_pipe<owned> pipe;
    pipe.create_named_pipe(TEXT("\\\\.\\pipe\\TBA:ntsuspend"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 0, 0, INFINITE, &sa);

    service_status.dwWaitHint = 0;
    service_status.dwCurrentState = SERVICE_RUNNING;
    if (!SetServiceStatus(service_status_handle, &service_status))
      throw Win32_error(TEXT("SetServiceStatus"));

    // Wait for connection
    pipe.connect_named_pipe();

    // Impersonate, for security purposes
    pipe.impersonate_named_pipe_client();

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

    // Message format:
    //  Action (1 char): s(uspend), r(esume), or t(est)
    //  Target (optional for Test action):
    //    i, followed by DWORD of pid
    //    n, followed by length-prefixed string of name
    //    s, followed by length-prefixed string of name (substring match)

    DWORD pid = 0;
    string name;
    bool exact_match = true;
    bool resume = false;
    bool test = false;

    int i = 0;

    switch (msg[0])
    {
      case TEXT('s'): break;
      case TEXT('r'): resume = true; break;
      case TEXT('t'): test = true; break;
      default: throw error(TEXT("Invalid message received: unknown action"));
    }

    ++i;
    if (msg_size == 1 && !test)
      throw error(TEXT("Invalid message received: no target"));

    if (msg_size > 1)
    {
      switch (msg[1])
      {
        case TEXT('i'):
        {
          if (msg_size < 2 + sizeof(pid))
            throw error(TEXT("Invalid message received: incomplete pid target"));
          CopyMemory(&pid, msg.get() + 2, sizeof(pid));
          i += 1 + sizeof(pid);
          break;
        }
        case TEXT('n'):
        case TEXT('s'):
        {
          if (msg[1] == TEXT('s'))
            exact_match = false;
          if (msg_size < 3)
            throw error(TEXT("Invalid message received: incomplete name target length"));
          const unsigned length = msg[2];
          if (msg_size < 3 + length)
            throw error(TEXT("Invalid message received: incomplete name target"));
          name = string(msg.get() + 3, msg.get() + 3 + length);
          i += 2 + length;
          break;
        }
      }
    }

    if (i != msg_size)
      throw error(TEXT("Invalid message received: extra data"));

    string response;
    try
    {
      response = ntsuspend(name, exact_match, pid, resume, test);
    }
    catch (const error & e)
    {
      response = TEXT("Error: ") + e.twhat() + TEXT("\n");
    }
    catch (const std::exception & e)
    {
      response = TEXT("Error: ") + to_string(e.what()) + TEXT("\n");
    }

    // Turn off impersonation
    RevertToSelf();

    // Send the response on the pipe
    pipe.write_file_sync(response.data(), response.size());

    // Make sure the client gets all the response before terminating
    pipe.flush_file_buffers();
    pipe.disconnect_named_pipe();

    service_status.dwCurrentState = SERVICE_STOPPED;
    if (!SetServiceStatus(service_status_handle, &service_status))
      throw Win32_error(TEXT("SetServiceStatus"));
  }
  catch (const Win32_error & e)
  {
    ods(TEXT("ntsuspend: ") + e.twhat());
    service_status.dwWin32ExitCode = e.code;
    service_status.dwWaitHint = 0;
    service_status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(service_status_handle, &service_status);
  }
  catch (const error & e)
  {
    ods(TEXT("ntsuspend: ") + e.twhat());
    service_status.dwWin32ExitCode = 0;
    service_status.dwWaitHint = 0;
    service_status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(service_status_handle, &service_status);
  }
}

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

int command_line_main(int argc, char_t * argv[])
{
  boost::array<option_def, 9> option_defs = { {
      { TEXT('h'), TEXT("help") },
      { TEXT('i'), TEXT("pid"), option_def::required_argument },
      { TEXT('n'), TEXT("name"), option_def::required_argument },
      { TEXT('s'), TEXT("substr") },
      { TEXT('r'), TEXT("resume") },
      { TEXT('t'), TEXT("test") },
      { TEXT('c'), TEXT("computer"), option_def::required_argument },
      { TEXT('u'), TEXT("username"), option_def::required_argument },
      { TEXT('p'), TEXT("password"), option_def::optional_argument }
  } };

  try
  {
    option_parser options(argc, argv + 1, option_defs.begin(), option_defs.end());

    DWORD pid = 0;
    string name;
    bool exact_match = true;
    bool resume = false;
    bool test = false;
    string computer, username, password;
    bool password_specified = false;
    bool prompt_for_password = false;
    while (options.getopt())
    {
      if (!options.option)
        throw option_error(string(TEXT("Missing option for argument '")) + options.argument + TEXT("'"));

      switch (options.option->short_option)
      {
        case TEXT('h'):
          return usage();
        case TEXT('i'):
          pid = _tcstoul(options.argument, 0, 0);
          if (pid == 0)
            throw option_error(string(TEXT("Invalid argument '")) + options.argument + TEXT("' for option --pid"));
          break;
        case TEXT('n'):
          name = options.argument;
          break;
        case TEXT('s'):
          exact_match = false;
          break;
        case TEXT('r'):
          resume = true;
          break;
        case TEXT('t'):
          test = true;
          break;
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

    if (pid == 0 && name.empty() && !test)
      throw option_error(TEXT("Neither process id nor process name specified"));
    else if (pid != 0 && !name.empty())
      throw option_error(TEXT("Both process id and process name specified"));

    // Handle local requests
    if (computer.empty())
    {
      tcout(ntsuspend(name, exact_match, pid, resume, test));
      return 0;
    }

    // Prompt for password if necessary
    if (prompt_for_password)
      password = get_password();

    // Log into remote computer
    //  (Log out when this object goes out of scope)
    remote_login login(computer, username.empty() ? 0 : username.c_str(), password_specified ? password.c_str() : 0);

    // Copy this currently-running exe file onto remote computer (displaying but ignoring errors)
    //  (Delete the remote file when this object goes out of scope)
    remote_file file(computer, TEXT("ntutils.ntsuspend.exe"));
    if (!file.Copy(get_module_file_name()))
      tcerr(TEXT("Warning: ") + Win32_error(TEXT("CopyFile")).twhat() + TEXT("\n"));

    // Connect to the remote service control manager
    service<owned> scm;
    scm.open_sc_manager(computer, SC_MANAGER_CREATE_SERVICE);

    // Install the remote service (allowing the service to already be installed)
    //  (Uninstall the remote service when this object goes out of scope)
    remote_service_install install;
    install.OpenService(scm.Handle(), TEXT("ntutils.ntsuspend"));
    if (install.Valid())
      tcerr(TEXT("Warning: service already existed\n"));
    else
    {
      install.create_service(scm.Handle(), TEXT("ntutils.ntsuspend"), TEXT(""), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
          SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, TEXT("%SystemRoot%\\ntutils.ntsuspend.exe service"));
    }

    // Start up the service (allowing the service to already be running)
    //  (Stop the service when this object goes out of scope)
    remote_service_start service;
    service.Reset(install.Handle());
    if (!service.StartService())
      tcerr(TEXT("Warning: could not start service: ") + Win32_error(TEXT("StartService")).twhat() + TEXT("\n"));

    // Construct the message to be sent
    string msg;
    if (test)
      msg += TEXT('t');
    else if (resume)
      msg += TEXT('r');
    else
      msg += TEXT('s');
    if (pid != 0)
    {
      msg.resize(2 + sizeof(pid));
      msg[1] = TEXT('i');
      CopyMemory(&msg[0] + 2, &pid, sizeof(pid));
    }
    else if (!name.empty())
    {
      msg.resize(3 + name.length());
      if (exact_match)
        msg[1] = TEXT('n');
      else
        msg[1] = TEXT('s');
      msg[2] = name.length();
      CopyMemory(&msg[0] + 3, &name[0], name.length());
    }

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
    if (!CallNamedPipe((TEXT("\\\\") + computer + TEXT("\\pipe\\TBA:ntsuspend")).c_str(), &msg[0], msg.length(),
        response.get(), max_response_size, &response_size, NMPWAIT_WAIT_FOREVER))
      throw Win32_error(TEXT("CallNamedPipe"));

    // Output the response locally
    tcout(string(response.get(), response.get() + response_size));
  }
  catch (const option_error & e)
  {
    tcerr(TEXT("Error: ") + e.twhat() + TEXT("\n"));
    return usage();
  }
  catch (const error & e)
  {
    tcerr(TEXT("Error: ") + e.twhat() + TEXT("\n"));
    return -1;
  }
  catch (const std::exception & e)
  {
    tcerr("Error: " + ANSI_string(e.what()) + "\n");
    return -1;
  }

  return 0;
}

int main(int argc, char * argv[])
{
  // We can act as though invoked from the command line if StartServiceCtrlDispatcher fails,
  //  and that would work; however, StartServiceCtrlDispatcher may delay for several seconds.
  // So instead, we pass a secret parameter "service" when running as a service

  if (argc == 2 && !_tcscmp(argv[1], TEXT("service")))
  {
    SERVICE_TABLE_ENTRY services[] = { { TEXT(""), &service_main }, { 0, 0 } };
    return StartServiceCtrlDispatcher(services);
  }
  else
    return command_line_main(argc, argv);
}
