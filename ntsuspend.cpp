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

    bool resume = false;
    bool test = false;

    unsigned i = 0;

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

    process_selector selector;
    selector.get_target_from_message(i, msg, msg_size);

    if (i != msg_size)
      throw error(TEXT("Invalid message received: extra data"));

    string response;
    try
    {
      response = ntsuspend(false, selector, resume, test);
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

    bool resume = false;
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
        case TEXT('r'):
          resume = true;
          break;
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
      tcout(ntsuspend(true, selector, resume, test));
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
