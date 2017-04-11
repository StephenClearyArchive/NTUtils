// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef BASIC_ERROR_H
#define BASIC_ERROR_H

#include <string>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace basic {

typedef std::string ANSI_string;
typedef std::wstring UNICODE_string;

#ifdef UNICODE
typedef wchar_t char_t;
typedef UNICODE_string string;
#else
typedef char char_t;
typedef ANSI_string string;
#endif

// Removes whitespace (" \r\n\t") from the beginning and end of a string
inline string trim(const string & x)
{
  const string::size_type begin = x.find_first_not_of(TEXT(" \r\n\t"));
  if (begin == string::npos)
    return string();
  return x.substr(begin, x.find_last_not_of(TEXT(" \r\n\t")) - begin + 1);
}

string to_string(const unsigned long x);

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
      string message;
#ifdef UNICODE
      ANSI_string message_ansi;
#endif

      explicit error_impl(const string & nmessage) :message(nmessage) { }
    };

    boost::shared_ptr<error_impl> impl;

    error(const string & type, const string & desc, const string & func)
    :impl(new error_impl(type + TEXT(" error: '") + desc + TEXT("' from ") + func)) { }

  public:
    error(const string & nmessage):impl(new error_impl(nmessage)) { }

    virtual ~error() throw() { }

    virtual const char * what() const throw()
    {
#ifdef UNICODE
      if (!impl->message_ansi.empty())
        return impl->message_ansi.c_str();
      const int buffer_size = WideCharToMultiByte(CP_ACP, 0, impl->message.c_str(), -1, 0, 0, 0, 0);
      if (buffer_size == 0)
      {
        impl->message_ansi = "<Could not convert error message>";
        return impl->message_ansi.c_str();
      }
      impl->message_ansi.reserve(buffer_size);
      if (!WideCharToMultiByte(CP_ACP, 0, impl->message.c_str(), -1, (CHAR *) impl->message_ansi.data(), buffer_size, 0, 0))
        impl->message_ansi = "<Could not convert error message>";
      return impl->message_ansi.c_str();
#else
      return impl->message.c_str();
#endif
    }

    virtual const string & twhat() const { return impl->message; }
};

struct Win32_error: public error
{
  DWORD code;

  static string format_message(const DWORD code, const HMODULE source = 0)
  {
    struct fixed_local_memory_guard: boost::noncopyable
    {
      LPTSTR ptr;

      fixed_local_memory_guard():ptr(0) { }
      ~fixed_local_memory_guard() { LocalFree((HLOCAL) ptr); }
    };
    fixed_local_memory_guard buf;

    DWORD format_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
    if (source != 0)
      format_flags |= FORMAT_MESSAGE_FROM_HMODULE;
    else
      format_flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    if (!FormatMessage(format_flags, source, code, 0, (char_t *) &buf.ptr, 0, 0))
      return TEXT("Unknown error ") + to_string(code);
    const string ret(buf.ptr);
    return trim(ret);
  }

  static DWORD get_and_clear_error()
  {
    const DWORD ret = GetLastError();
    SetLastError(0);
    return ret;
  }

  explicit Win32_error(const string & func, const DWORD ncode = get_and_clear_error())
  :error(TEXT("Win32"), format_message(ncode), func), code(ncode) { }
};

inline void ods(const string & msg) { OutputDebugString(msg.c_str()); }

}

#endif
