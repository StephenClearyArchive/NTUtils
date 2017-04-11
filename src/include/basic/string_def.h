// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_STRING_DEF_H
#define BASIC_STRING_DEF_H

#include <string>

#include <tchar.h>

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

// Removes whitespace (" \r\n\t") from the beginning and end of a string
static inline string trim(const string & x)
{
  const string::size_type begin = x.find_first_not_of(TEXT(" \r\n\t"));
  if (begin == string::npos)
    return string();
  return x.substr(begin, x.find_last_not_of(TEXT(" \r\n\t")) - begin + 1);
}

// Escape special characters for an XML attribute
static inline string make_xml_attribute_value(const_str value)
{
  string ret;
  ret.reserve(value.length() + 2);
  ret = TEXT("'");
  for (const char_t * i = value.begin(); i != value.end(); ++i)
    if (*i == TEXT('<'))
      ret += TEXT("&lt;");
    else if (*i == TEXT('&'))
      ret += TEXT("&amp;");
    else if (*i == TEXT('\''))
      ret += TEXT("&apos;");
    else
      ret += *i;
  ret += TEXT('\'');
  return ret;
}

}

#endif
