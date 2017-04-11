// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#include <windows.h>

#include <boost/array.hpp>

#include "ntutils/ntpriority.inc"
#include "ntutils/remote_framework.h"

static const DWORD max_response_size = 8192;

static const string name = TEXT("ntpriority");

struct server: server_framework<server>
{
  static inline const string & name() { return ::name; }
  static inline string handle_message(boost::scoped_array<char> & msg, const DWORD msg_size)
  {
    // Message format:
    //  Action (1 char): l (set level, followed by DWORD of level) or t(est)
    //  Target (optional for Test action):
    //    i, followed by DWORD of pid
    //    n, followed by length-prefixed string of name
    //    s, followed by length-prefixed string of name (substring match)

    DWORD level;
    bool test = false;

    unsigned i = 0;

    switch (msg[0])
    {
      case TEXT('l'):
        if (msg_size < 1 + sizeof(level))
          throw error(TEXT("Invalid message received: incomplete level action"));
        CopyMemory(&level, msg.get() + 1, sizeof(level));
        i += 1 + sizeof(level);
        break;
      case TEXT('t'):
        ++i;
        test = true;
        break;
      default:
        throw error(TEXT("Invalid message received: unknown action"));
    }

    if (msg_size == i && !test)
      throw error(TEXT("Invalid message received: no target"));

    process_selector selector;
    selector.get_target_from_message(i, msg, msg_size);

    if (i != msg_size)
      throw error(TEXT("Invalid message received: extra data"));

    string response;
    try
    {
      response = ntpriority(false, selector, level, test);
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
  tcerr(TEXT("Usage: ntpriority [options]\n"));
  tcerr(TEXT("Options:\n"));
  tcerr(TEXT("  -h [ --help ]           : Display this information\n"));
  tcerr(TEXT("  -i [ --pid ] arg        : Specify process id\n"));
  tcerr(TEXT("  -n [ --name ] arg       : Specify process name\n"));
  tcerr(TEXT("  -s [ --substr ]         :   Process name is a substring match\n"));
  tcerr(TEXT("  -l [ --level ] arg      : Set priority level of process(es)\n"));
  tcerr(TEXT("                          'arg' may be a numerical value or IDLE, BELOW_NORMAL,\n"));
  tcerr(TEXT("                          NORMAL, ABOVE_NORMAL, HIGH, or REALTIME\n"));
  tcerr(TEXT("  -t [ --test ]           : Test priority level of process(es)\n"));
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
      { TEXT('l'), TEXT("level"), option_def::required_argument },
      { TEXT('t'), TEXT("test") },
      { TEXT('c'), TEXT("computer"), option_def::required_argument },
      { TEXT('u'), TEXT("username"), option_def::required_argument },
      { TEXT('p'), TEXT("password"), option_def::optional_argument }
  } };

  try
  {
    option_parser options(argc, argv + 1, option_defs.begin(), option_defs.end());

    DWORD level;
    bool test = false;
    process_selector selector;
    client_def client;
    while (options.getopt())
    {
      if (!options.option)
        throw option_error(string(TEXT("Missing option for argument '")) + options.argument + TEXT("'"));

      switch (options.option->short_option)
      {
        case TEXT('h'):
          return usage();
        case TEXT('l'):
        {
          char_t * test;
          level = _tcstoul(options.argument, &test, 0);
          if (*test != 0)
          {
            if (!_tcsicmp(options.argument, TEXT("ABOVE_NORMAL")))
              level = ABOVE_NORMAL_PRIORITY_CLASS;
            else if (!_tcsicmp(options.argument, TEXT("BELOW_NORMAL")))
              level = BELOW_NORMAL_PRIORITY_CLASS;
            else if (!_tcsicmp(options.argument, TEXT("HIGH")))
              level = HIGH_PRIORITY_CLASS;
            else if (!_tcsicmp(options.argument, TEXT("IDLE")))
              level = IDLE_PRIORITY_CLASS;
            else if (!_tcsicmp(options.argument, TEXT("NORMAL")))
              level = NORMAL_PRIORITY_CLASS;
            else if (!_tcsicmp(options.argument, TEXT("REALTIME")))
              level = REALTIME_PRIORITY_CLASS;
            else
              throw option_error(string(TEXT("Invalid argument '")) + options.argument + TEXT("' for option --level"));
          }
          break;
        }
        case TEXT('t'):
          test = true;
          break;
        default:
          if (selector.handle_option(options))
            break;
          if (client.handle_option(options))
            break;
      }
    }

    selector.validate_options(test);

    // Handle local requests
    if (!client.is_remote())
    {
      tcout(ntpriority(true, selector, level, test));
      return 0;
    }

    // Construct the message to be sent
    string msg;
    if (test)
      msg += TEXT('t');
    else
    {
      msg.resize(1 + sizeof(level));
      msg[0] = TEXT('l');
      CopyMemory(&msg[0] + 1, &level, sizeof(level));
    }

    selector.add_target_to_message(msg);

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
