// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#include <windows.h>

#include <boost/array.hpp>

#include "ntutils/ntsuspend.inc"
#include "ntutils/remote_framework.h"

static const DWORD max_response_size = 8192;

static const string name = TEXT("ntsuspend");

struct server: server_framework<server>
{
  static inline const string & name() { return ::name; }
  static inline string handle_message(boost::scoped_array<char> & msg, const DWORD msg_size)
  {
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
      response = ntsuspend(false, name, exact_match, pid, resume, test);
    }
    catch (const error & e)
    {
      response = TEXT("Error: ") + e.twhat() + TEXT("\n");
    }
    catch (const std::exception & e)
    {
      response = TEXT("Error: ") + to_string(e.what()) + TEXT("\n");
    }

    return response;
  }
};

struct client_def: client_framework<client_def>
{
  static inline const string & name() { return server::name(); }
};

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
    client_def client;
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
        case TEXT('u'):
        case TEXT('p'):
          client.handle_option(options);
          break;
      }
    }

    if (pid == 0 && name.empty() && !test)
      throw option_error(TEXT("Neither process id nor process name specified"));
    else if (pid != 0 && !name.empty())
      throw option_error(TEXT("Both process id and process name specified"));

    // Handle local requests
    if (!client.is_remote())
    {
      tcout(ntsuspend(true, name, exact_match, pid, resume, test));
      return 0;
    }

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

    // Handle remote requests
    client.start(msg, max_response_size);
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
    return server::start();
  else
    return command_line_main(argc, argv);
}
