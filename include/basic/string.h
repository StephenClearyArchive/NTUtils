// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_STRING_H
#define BASIC_STRING_H

#include <tchar.h>

#include "basic/error.h"

namespace basic {

// The following structure makes it much easier to pass string + length data
template <typename CharT, typename StrT>
class const_str_impl
{
  private:
    const CharT * p;
    unsigned len;

  public:
    // Default constructor: equivalent to null pointer/empty string
    const_str_impl():p(0), len(0) { }

    // Constructor allowing character pointer/size of array combination
    const_str_impl(const CharT * const np, const unsigned nlen):p(np), len(nlen) { }

    // Default copy constructor

    // (implicit) cast from character pointer: get length from strlen
    const_str_impl(const CharT * const np):p(np), len(np == 0 ? 0 : _tcslen(np)) { }

    // (implicit) cast from character array: get length from array (subtract 1
    //   for the terminating '\0' byte)
    template <unsigned N>
    const_str_impl(const CharT (&np)[N]):p(np), len(N - 1) { }
    template <unsigned N>
    const_str_impl(CharT (&np)[N]):p(np), len(N - 1) { }

    // (implicit) cast from string: get length from string data member
    const_str_impl(const StrT & n):p(n.c_str()), len(n.length()) { }

    // Default assignment operator

    // Default destructor

    // Provide implicit cast back to char ptr
    operator const CharT *() const { return p; }

    // Accessor functions
    const CharT * const & ptr() const { return p; }
    unsigned length() const { return len; }

    bool empty() const { return (len == 0); }
    const CharT * begin() const { return p; }
    const CharT * end() const { return p + len; }
    const CharT & operator[](const unsigned i) const { return *(p + i); }

    // Translator function (provided as convenience; performs string copy)
    StrT as_string() const { return StrT(p, len); }

    // Convenience string-related operator overloads
    friend StrT & operator+=(StrT & a, const const_str_impl<CharT, StrT> & b)
    { return (a += b.as_string()); }
    friend StrT operator+(const StrT & a, const const_str_impl<CharT, StrT> & b)
    { return (a + b.as_string()); }
    friend StrT operator+(const const_str_impl<CharT, StrT> & a, const StrT & b)
    { return (a.as_string() + b); }
};

typedef const_str_impl<char, ANSI_string> const_ANSI_str;
typedef const_str_impl<wchar_t, UNICODE_string> const_UNICODE_str;
typedef const_str_impl<char_t, string> const_str;

// The following structure is the same, but it doesn't pass length info
template <typename CharT, typename StrT>
class const_str_ptr_impl
{
  private:
    const CharT * p;

  public:
    // Default constructor: equivalent to null pointer
    const_str_ptr_impl():p(0) { }

    // Default copy constructor

    // (implicit) cast from character pointer
    const_str_ptr_impl(const CharT * const np):p(np) { }

    // (implicit) cast from string
    const_str_ptr_impl(const StrT & n):p(n.c_str()) { }

    // (implicit) cast from const_str
    const_str_ptr_impl(const const_str_impl<CharT, StrT> np):p(np) { }

    // Default assignment operator

    // Default destructor

    // Provide implicit cast back to char ptr
    operator const CharT *() const { return p; }

    // Accessor functions
    const CharT * const & ptr() const { return p; }

    const CharT & operator[](const unsigned i) const { return *(p + i); }

    // Translator function (provided as convenience; performs string copy)
    StrT as_string() const { return StrT(p); }

    // Convenience string-related operator overloads
    friend StrT & operator+=(StrT & a, const const_str_ptr_impl<CharT, StrT> & b)
    { return (a += b.ptr()); }
    friend StrT operator+(const StrT & a, const const_str_ptr_impl<CharT, StrT> & b)
    { return (a + b.ptr()); }
    friend StrT operator+(const const_str_ptr_impl<CharT, StrT> & a, const StrT & b)
    { return (a.ptr() + b); }
};

typedef const_str_ptr_impl<char, ANSI_string> const_ANSI_str_ptr;
typedef const_str_ptr_impl<wchar_t, UNICODE_string> const_UNICODE_str_ptr;
typedef const_str_ptr_impl<char_t, string> const_str_ptr;

inline string safe_sprintf(const char_t * const format, ...)
{
  va_list ap;

  int written;
  string ret;
  ret.resize(32);

  while (true)
  {
    va_start(ap, format);
    written = _vsntprintf(&ret[0], ret.size(), format, ap);
    va_end(ap);

    if (written > 0)
      break;
    else if (written != -1)
      throw error(TEXT("Unexpected result from _vsntprintf"));

    ret.resize(ret.size() * 2);
  }

  ret.resize(written);
  return ret;
}

inline string to_string(const unsigned long x) { return safe_sprintf(TEXT("%u"), x); }

inline ANSI_string wide_char_to_multi_byte(const_UNICODE_str_ptr src, const UINT cp = CP_ACP)
{
  if (src.ptr() == 0)
    return ANSI_string();
  const int buffer_size = WideCharToMultiByte(cp, 0, src.ptr(), -1, 0, 0, 0, 0);
  if (buffer_size == 0)
    return ANSI_string();
  ANSI_string buffer;
  buffer.resize(buffer_size);
  if (!WideCharToMultiByte(cp, 0, src.ptr(), -1, &buffer[0], buffer_size, 0, 0))
    throw Win32_error(TEXT("WideCharToMultiByte"));
  buffer.resize(buffer_size - 1);
  return buffer;
}

// Converts a UNICODE string to an ANSI string
inline ANSI_string UNICODE_string_to_ANSI_string(const_UNICODE_str_ptr src)
{ return wide_char_to_multi_byte(src); }

inline UNICODE_string multi_byte_to_wide_char(const_ANSI_str_ptr src, const UINT cp = CP_ACP)
{
  if (src.ptr() == 0)
    return UNICODE_string();
  const int buffer_size = MultiByteToWideChar(cp, 0, src.ptr(), -1, 0, 0);
  if (buffer_size == 0)
    return UNICODE_string();
  UNICODE_string buffer;
  buffer.resize(buffer_size);
  if (!MultiByteToWideChar(cp, 0, src.ptr(), -1, &buffer[0], buffer_size))
    throw Win32_error(TEXT("MultiByteToWideChar"));
  buffer.resize(buffer_size - 1);
  return buffer;
}

// Converts an ANSI string to a UNICODE string
inline UNICODE_string ANSI_string_to_UNICODE_string(const_ANSI_str_ptr src)
{ return multi_byte_to_wide_char(src); }

inline const ANSI_string & to_ANSI_string(const ANSI_string & src) { return src; }
inline ANSI_string to_ANSI_string(const char * const src) { return ANSI_string(src); }
inline ANSI_string to_ANSI_string(const_UNICODE_str_ptr src) { return UNICODE_string_to_ANSI_string(src); }

inline const UNICODE_string & to_UNICODE_string(const UNICODE_string & src) { return src; }
inline UNICODE_string to_UNICODE_string(const wchar_t * const src) { return UNICODE_string(src); }
inline UNICODE_string to_UNICODE_string(const_ANSI_str_ptr src) { return ANSI_string_to_UNICODE_string(src); }

#ifdef UNICODE
inline const string & to_string(const UNICODE_string & src) { return src; }
inline string to_string(const wchar_t * const src) { return UNICODE_string(src); }
inline string to_string(const_ANSI_str_ptr src) { return ANSI_string_to_UNICODE_string(src); }
#else
inline const string & to_string(const ANSI_string & src) { return src; }
inline string to_string(const char * const src) { return ANSI_string(src); }
inline string to_string(const_UNICODE_str_ptr src) { return UNICODE_string_to_ANSI_string(src); }
#endif

}

#endif
