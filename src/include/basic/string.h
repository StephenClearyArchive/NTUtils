// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_STRING_H
#define BASIC_STRING_H

#include "basic/error.h"

namespace basic {

static inline string safe_sprintf(const char_t * const format, ...)
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

static inline string to_string(const unsigned long x) { return safe_sprintf(TEXT("%u"), x); }

static inline ANSI_string wide_char_to_multi_byte(const_UNICODE_str_ptr src, const UINT cp = CP_ACP)
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
static inline ANSI_string UNICODE_string_to_ANSI_string(const_UNICODE_str_ptr src)
{ return wide_char_to_multi_byte(src); }

static inline UNICODE_string multi_byte_to_wide_char(const_ANSI_str_ptr src, const UINT cp = CP_ACP)
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
static inline UNICODE_string ANSI_string_to_UNICODE_string(const_ANSI_str_ptr src)
{ return multi_byte_to_wide_char(src); }

static inline const ANSI_string & to_ANSI_string(const ANSI_string & src) { return src; }
static inline ANSI_string to_ANSI_string(const char * const src) { return ANSI_string(src); }
static inline ANSI_string to_ANSI_string(const_UNICODE_str_ptr src) { return UNICODE_string_to_ANSI_string(src); }

static inline const UNICODE_string & to_UNICODE_string(const UNICODE_string & src) { return src; }
static inline UNICODE_string to_UNICODE_string(const wchar_t * const src) { return UNICODE_string(src); }
static inline UNICODE_string to_UNICODE_string(const_ANSI_str_ptr src) { return ANSI_string_to_UNICODE_string(src); }

#ifdef UNICODE
static inline const string & to_string(const UNICODE_string & src) { return src; }
static inline string to_string(const wchar_t * const src) { return UNICODE_string(src); }
static inline string to_string(const_ANSI_str_ptr src) { return ANSI_string_to_UNICODE_string(src); }
#else
static inline const string & to_string(const ANSI_string & src) { return src; }
static inline string to_string(const char * const src) { return ANSI_string(src); }
static inline string to_string(const_UNICODE_str_ptr src) { return UNICODE_string_to_ANSI_string(src); }
#endif

}

#endif
