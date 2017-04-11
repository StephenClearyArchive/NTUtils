// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_CONSOLE_H
#define NTUTILS_CONSOLE_H

#include <algorithm>

#include "ntutils/basic.h"

namespace ntutils {

inline void write_to_console(const DWORD handle_id, const_ANSI_str str)
{
  const HANDLE out = GetStdHandle(handle_id);
  if (out == INVALID_HANDLE_VALUE || out == 0)
    return;

  DWORD junk;
  if (!WriteFile(out, str.ptr(), str.length(), &junk, 0))
    throw Win32_error(TEXT("WriteFile (console)"));
}

inline string read_from_console(const DWORD max = 256)
{
  const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  if (in == INVALID_HANDLE_VALUE || in == 0)
    throw Win32_error(TEXT("GetStdHandle"));

  DWORD read;
  string ret;
  ret.resize(max);
  if (!ReadFile(in, &ret[0], max * sizeof(char_t), &read, 0))
    throw Win32_error(TEXT("ReadFile (console)"));
  // We subtract 2 for the CR/LF
  ret.resize(read / sizeof(char_t) - 2);
  return ret;
}

inline void tcout(const_UNICODE_str_ptr str)
{ write_to_console(STD_OUTPUT_HANDLE, wide_char_to_multi_byte(str, GetConsoleOutputCP())); }

inline void tcout(const_ANSI_str str)
{
  if (GetConsoleOutputCP() == CP_ACP)
    write_to_console(STD_OUTPUT_HANDLE, str);
  else
    tcout(ANSI_string_to_UNICODE_string(str));
}

inline void tcerr(const_UNICODE_str_ptr str)
{ write_to_console(STD_ERROR_HANDLE, wide_char_to_multi_byte(str, GetConsoleOutputCP())); }

inline void tcerr(const_ANSI_str str)
{
  if (GetConsoleOutputCP() == CP_ACP)
    write_to_console(STD_ERROR_HANDLE, str);
  else
    tcerr(ANSI_string_to_UNICODE_string(str));
}

struct option_error: error
{
  explicit option_error(const string & msg):error(msg) { }
};

struct option_def
{
  static const int no_argument = 0;
  static const int required_argument = 1;
  static const int optional_argument = 2;

  char_t short_option;
  const char_t * long_option;
  int argument_type;
  unsigned long_option_length;
};

class option_parser
{
  private:
    const option_def * const options_begin;
    const option_def * const options_end;

    const int argc;
    const char_t * const * const argv;

    const char_t * multi_opt;

    bool do_getopt(const char_t * const opt)
    {
      // Find the matching short option
      for (option = options_begin; option != options_end; ++option)
        if (option->short_option != 0 && option->short_option == *opt)
          break;
      if (option == options_end)
        throw option_error(string(TEXT("Unknown option -")) + *opt);

      // Test for arguments
      if (option->argument_type == option_def::required_argument)
      {
        // Arguments always end short option runs
        multi_opt = 0;
        ++index;

        if (*(opt + 1) == 0)
        {
          // We are at the end of this string, so the argument must be the next string
          if (argv[index] == 0 || argv[index][0] == '-')
            throw option_error(string(TEXT("Option -")) + *opt + TEXT(" requires an argument"));
          argument = argv[index];
          ++index;
          return true;
        }
        else
        {
          // Otherwise, the argument must be the remainder of this string
          argument = opt + 1;
          return true;
        }
      }
      else if (option->argument_type == option_def::optional_argument)
      {
        ++index;

        // We assume that any remaining string is an argument
        if (*(opt + 1) != 0)
        {
          // Arguments end short option runs
          multi_opt = 0;
          argument = opt + 1;
          return true;
        }

        // If there is no more string remaining, check the next string
        if (argv[index] != 0 && argv[index][0] != '-')
        {
          // Arguments end short option runs
          multi_opt = 0;
          argument = argv[index];
          ++index;
          return true;
        }

        // Otherwise, we must have an option without an argument
        argument = 0;
        return true;
      }

      // This is a short option that does not take arguments
      argument = 0;

      if (*(opt + 1) == 0)
      {
        // We're at the end of this string
        multi_opt = 0;
        ++index;
        return true;
      }

      // Since we're not at the end of the string, save our place for next time
      multi_opt = opt + 1;
      return true;
    }

  public:
    option_parser(const int nargc, const char_t * const * const nargv,
        option_def * const noptions_begin, option_def * const noptions_end)
    :argc(nargc),
     argv(nargv),
     options_begin(noptions_begin),
     options_end(noptions_end),
     index(0),
     multi_opt(0)
    {
      // Run through each option, caching the length of the long option string
      for (option_def * i = noptions_begin; i != noptions_end; ++i)
      {
        if (i->long_option == 0)
          i->long_option_length = 0;
        else
          i->long_option_length = _tcslen(i->long_option);
      }
    }

    bool getopt()
    {
      if (multi_opt == 0)
      {
        // We are not in the middle of parsing a string; so start with a new string
        if (argv[index] == 0)
        {
          // We are done with all the input options
          return false;
        }

        if (*argv[index] != TEXT('-') || argv[index][1] == 0)
        {
          // This string does not start with '-' or is a '-' all by itself
          option = 0;
          argument = argv[index];
          ++index;
          return true;
        }

        if (argv[index][1] == TEXT('-'))
        {
          // This string starts with '--'
          if (argv[index][2] == 0)
          {
            // This string is just a '--'
            option = 0;
            argument = argv[index];
            ++index;
            return true;
          }

          // Search for a match with long option definitions
          for (option = options_begin; option != options_end; ++option)
            if (option->long_option != 0 && !_tcsnicmp(option->long_option, argv[index] + 2, option->long_option_length))
              break;

          // If there's no match, it's an unknown argument
          if (option == options_end)
            throw option_error(string(TEXT("Unknown option --")) + argv[index]);

          // We have a match; check for an exact match or an equals sign
          if (*(argv[index] + 2 + option->long_option_length) == 0)
          {
            // This is an exact match; test for arguments
            ++index;
            if (option->argument_type == option_def::no_argument)
            {
              argument = 0;
              return true;
            }
            else if (option->argument_type == option_def::required_argument)
            {
              if (argv[index] == 0 || argv[index][0] == TEXT('-'))
                throw option_error(string(TEXT("Option --")) + option->long_option + TEXT(" requires an argument"));
              argument = argv[index];
              ++index;
              return true;
            }
            else
            {
              // Optional argument
              if (argv[index] == 0 || argv[index][0] == TEXT('-'))
                argument = 0;
              else
              {
                argument = argv[index];
                ++index;
              }
              return true;
            }
          }
          else if (*(argv[index] + 2 + option->long_option_length) == TEXT('='))
          {
            // Option match, with embedded argument
            if (option->argument_type == option_def::no_argument)
              throw option_error(string(TEXT("Option --")) + option->long_option + TEXT(" cannot take an argument"));
            argument = argv[index] + 3 + option->long_option_length;
            ++index;
            return true;
          }
          else
          {
            // If the string isn't an exact match or followed by an equals sign,
            //  then it was only a partial match, so it's an unknown option
            throw option_error(string(TEXT("Unknown option --")) + argv[index]);
          }
        }
        else
        {
          // Parse short options
          return do_getopt(argv[index] + 1);
        }
      }

      // Parse short options in the middle of a short option run
      return do_getopt(multi_opt);
    }

    // The index in argv of the NEXT option to parse
    int index;

    // The entry in options_def of the last option returned (0 if none)
    const option_def * option;

    // The argument of the last option returned (0 if none)
    const char_t * argument;
};

}

#endif
