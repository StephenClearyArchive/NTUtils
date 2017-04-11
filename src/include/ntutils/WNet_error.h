// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_WNET_ERROR_H
#define NTUTILS_WNET_ERROR_H

#include <vector>

#include "ntutils/basic.h"

namespace ntutils {

struct WNet_error: public error
{
  static void format_last_message(const DWORD code, string & error_message, string & provider)
  {
    if (code != ERROR_EXTENDED_ERROR)
    {
      error_message = Win32_error::format_message(code);
      return;
    }
    
    int message_buf_size = 256;
    int provider_buf_size = 256;

    while (true)
    {
      std::vector<char_t> message_buf(message_buf_size);
      std::vector<char_t> provider_buf(provider_buf_size);

      DWORD junk;
      if (WNetGetLastError(&junk, &message_buf[0], message_buf_size,
          &provider_buf[0], provider_buf_size) != NO_ERROR)
      {
        error_message = TEXT("Unknown error");
        return;
      }

      if (message_buf[message_buf_size - 1] != 0)
        message_buf_size *= 2;
      else if (provider_buf[provider_buf_size - 1] != 0)
        provider_buf_size *= 2;
      else
      {
        error_message = &message_buf[0];
        provider = &provider_buf[0];
      }
    }
  }

  explicit WNet_error(const_str function, const DWORD code = Win32_error::get_and_clear_error())
  {
    string error_message, provider;
    format_last_message(code, error_message, provider);
    if (provider.empty())
      impl->message = error_impl::format_message(TEXT("WNet"), error_message, function);
    else
      impl->message = error_impl::format_message(TEXT("WNet"), error_message + TEXT(" (") + provider + TEXT(")"), function);
    impl->xml = TEXT("<error type='WNet' message=") + make_xml_attribute_value(error_message) +
        TEXT(" function=") + make_xml_attribute_value(function);
    if (provider.empty())
      impl->xml += TEXT(" code='") + to_string(code) + TEXT("' />");
    else
      impl->xml += TEXT(" provider=") + make_xml_attribute_value(provider) + TEXT(" />");
  }
};

}

#endif
