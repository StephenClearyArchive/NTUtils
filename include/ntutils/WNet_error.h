// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef NTUTILS_WNET_ERROR_H
#define NTUTILS_WNET_ERROR_H

#include <vector>

#include "ntutils/basic.h"

namespace ntutils {

struct WNet_error: public error
{
  static string format_last_message(const DWORD code)
  {
    if (code != ERROR_EXTENDED_ERROR)
      return Win32_error::format_message(code);
    
    int message_buf_size = 256;
    int provider_buf_size = 256;

    while (true)
    {
      std::vector<char_t> message_buf(message_buf_size);
      std::vector<char_t> provider_buf(provider_buf_size);

      DWORD junk;
      if (WNetGetLastError(&junk, &message_buf[0], message_buf_size,
          &provider_buf[0], provider_buf_size) != NO_ERROR)
        return TEXT("Unknown error");

      if (message_buf[message_buf_size - 1] != 0)
        message_buf_size *= 2;
      else if (provider_buf[provider_buf_size - 1] != 0)
        provider_buf_size *= 2;
      else
        return string(&message_buf[0]) + TEXT(" (") + &provider_buf[0] + TEXT(")");
    }
  }

  explicit WNet_error(const string & func, const DWORD code = Win32_error::get_and_clear_error())
  :error(TEXT("WNet"), format_last_message(code), func) { }
};

}

#endif
