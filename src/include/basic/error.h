// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_ERROR_H
#define BASIC_ERROR_H

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include "basic/string_def.h"

namespace basic {

static inline string to_string(const unsigned long x);

// The error class holds an allocated memory buffer for its error string in the character set
//  used by the application (ANSI or UNICODE). The C++ Standard only has a char return type
//  though, so if "what()" (an ANSI-only function) is called in a UNICODE application, the
//  error class will attempt to translate the error string to ANSI before returning.
// This error class provides "twhat()" to overcome this limitation.
// This class uses the Pointer-to-Implementation idiom to avoid const issues.

class error: public std::exception
{
  protected:
    struct error_impl
    {
      string message, xml;
#ifdef UNICODE
      ANSI_string message_ansi;
#endif

      static string format_message(const_str_ptr type, const string & description, const_str_ptr function = const_str_ptr())
      {
        string ret = type.as_string() + TEXT(" error: '") + description;
        if (function)
          ret += TEXT("' from ") + function;
        else
          ret += TEXT("'");
        return ret;
      }

      static string format_xml(const_str type, const string & description, const_str function = const_str_ptr())
      {
        string ret = TEXT("<error type=") + make_xml_attribute_value(type) + TEXT(" message=") + make_xml_attribute_value(description);
        if (function)
          ret += TEXT(" function=") + make_xml_attribute_value(function) + TEXT(" />");
        else
          ret += TEXT(" />");
        return ret;
      }

      error_impl() { }

      error_impl(const_str type, const string & description, const_str function = const_str())
      :message(format_message(type, description, function)),
       xml(format_xml(type, description, function))
      { }
    };

    boost::shared_ptr<error_impl> impl;

    error():impl(new error_impl()) { }

    error(const_str type, const string & description, const_str function = const_str())
    :impl(new error_impl(type, description, function)) { }

  public:
    error(const string & ndescription):impl(new error_impl(TEXT("General"), ndescription)) { }

    virtual ~error() throw() { }

    virtual const char * what() const throw()
    {
#ifdef UNICODE
      if (!impl->message_ansi.empty())
        return impl->message_ansi.c_str();
      const string & message = impl->message();
      const int buffer_size = WideCharToMultiByte(CP_ACP, 0, message.c_str(), -1, 0, 0, 0, 0);
      if (buffer_size == 0)
      {
        impl->message_ansi = "<Could not convert error message>";
        return impl->message_ansi.c_str();
      }
      impl->message_ansi.reserve(buffer_size);
      if (!WideCharToMultiByte(CP_ACP, 0, message.c_str(), -1, (CHAR *) impl->message_ansi.data(), buffer_size, 0, 0))
        impl->message_ansi = "<Could not convert error message>";
      return impl->message_ansi.c_str();
#else
      return impl->message.c_str();
#endif
    }

    const string & twhat() const { return impl->message; }
    const string & xml() const { return impl->xml; }
};

struct Win32_error: public error
{
  DWORD code;

  static bool FormatMessage(string & ret, const DWORD code, const HMODULE source = 0, const DWORD FormatFlags = 0)
  {
    struct fixed_local_memory_guard: boost::noncopyable
    {
      LPTSTR ptr;

      fixed_local_memory_guard():ptr(0) { }
      ~fixed_local_memory_guard() { LocalFree((HLOCAL) ptr); }
    };
    fixed_local_memory_guard buf;

    DWORD format_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FormatFlags;
    if (source != 0)
      format_flags |= FORMAT_MESSAGE_FROM_HMODULE;
    else
      format_flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    if (!::FormatMessage(format_flags, source, code, 0, (char_t *) &buf.ptr, 0, 0))
      return false;
    ret = buf.ptr;
    ret = trim(ret);
    return true;
  }

  static string format_message(const DWORD code, const HMODULE source = 0)
  {
    string ret;
    if (FormatMessage(ret, code, source))
      return ret;
    return TEXT("Unknown error ") + to_string(code);
  }

  static DWORD get_and_clear_error()
  {
    const DWORD ret = GetLastError();
    SetLastError(0);
    return ret;
  }

  explicit Win32_error(const_str function, const DWORD ncode = get_and_clear_error())
  {
    const string error_message = format_message(ncode);
    impl->message = error_impl::format_message(TEXT("Win32"), error_message, function);
    impl->xml = TEXT("<error type='Win32' message=") + make_xml_attribute_value(error_message) +
        TEXT(" function=") + make_xml_attribute_value(function) + TEXT(" code='") + to_string(ncode) + TEXT("' />");
  }
};

static inline void ods(const string & msg) { OutputDebugString(msg.c_str()); }

}

#endif
