// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef BASIC_HANDLE_H
#define BASIC_HANDLE_H

#include <boost/static_assert.hpp>
#include <boost/utility.hpp>

#include "basic/error.h"

namespace basic {

struct owned { static const bool value = true; };
struct unowned { static const bool value = false; };

// Every most-derived handle class is templated on an Owned type, either owned or unowned
template <typename Derived> struct extract_owned;
template <template <typename> class Derived> struct extract_owned<Derived<owned> > { typedef owned type; };
template <template <typename> class Derived> struct extract_owned<Derived<unowned> > { typedef unowned type; };

template <typename Derived, typename Owned>
struct handle_owned_base
{
  void CloseIfOwned() { }
};

template <typename Derived>
struct handle_owned_base<Derived, owned>: boost::noncopyable
{
  void CloseIfOwned()
  {
    Derived * const This = static_cast<Derived *>(this);
    if (This->Valid())
      This->Close();
  }

  ~handle_owned_base() { CloseIfOwned(); }
};

template <typename Derived, typename HandleType>
struct handle_base: handle_owned_base<Derived, typename extract_owned<Derived>::type>
{
  private:
    HandleType handle_;

  protected:
    void TestClose() {  }

    explicit handle_base(const HandleType & nhandle)
    :handle_(nhandle) { }

  public:
    void Reset(const HandleType & nhandle)
    {
      this->CloseIfOwned();
      handle_ = nhandle;
    }

    HandleType Release()
    {
      const HandleType ret = handle_;
      handle_ = Derived::InvalidValue();
    }

    HandleType Handle() const { return handle_; }

    typedef HandleType handle_base<Derived, HandleType>::* unspecified_bool_type;
    operator unspecified_bool_type() const
    {
      if (static_cast<const Derived *>(this)->Valid())
        return &handle_base<Derived, HandleType>::handle_;
      else
        return 0;
    }
};

template <typename Derived>
struct null_handle_base: handle_base<Derived, HANDLE>
{
  public:
    static HANDLE InvalidValue() { return 0; }
    bool Valid() const { return (this->Handle() != InvalidValue()); }

  protected:
    explicit null_handle_base(const HANDLE nhandle = 0)
    :handle_base<Derived, HANDLE>(nhandle) { }
};

template <typename Derived>
struct generic_null_handle_base: null_handle_base<Derived>
{
  public:
    BOOL Close() { return CloseHandle(this->Handle()); }
    void close() { if (!Close()) throw Win32_error(TEXT("CloseHandle")); }

  protected:
    explicit generic_null_handle_base(const HANDLE nhandle = 0)
    :null_handle_base<Derived>(nhandle) { }
};

template <typename Derived>
struct invalid_handle_base: handle_base<Derived, HANDLE>
{
  public:
    static HANDLE InvalidValue() { return INVALID_HANDLE_VALUE; }
    bool Valid() const { return (this->Handle() != InvalidValue()); }

  protected:
    invalid_handle_base(const HANDLE nhandle = INVALID_HANDLE_VALUE)
    :handle_base<Derived, HANDLE>(nhandle) { }
};

template <typename Derived>
struct generic_invalid_handle_base: invalid_handle_base<Derived>
{
  public:
    BOOL Close() { return CloseHandle(this->Handle()); }
    void close() { if (!Close()) throw Win32_error(TEXT("CloseHandle")); }

  protected:
    explicit generic_invalid_handle_base(const HANDLE nhandle = INVALID_HANDLE_VALUE)
    :invalid_handle_base<Derived>(nhandle) { }
};

// Assumes a class definition templated on a single type, called "Owned" with a typedef "base_type"
// Defines default constructor, implicit conversion from handle type (for unowned types only),
//  and conversion constructor from owned to unowned.
#define TBA_DEFINE_HANDLE_CLASS(class_name, handle_type) \
  class_name() { } \
  class_name(const handle_type & nhandle):base_type(nhandle) { BOOST_STATIC_ASSERT(!Owned::value); } \
  template <typename OtherOwned> class_name(const class_name<OtherOwned> & other):base_type(other.Handle()) \
  { BOOST_STATIC_ASSERT(!Owned::value); }

// Does the same, defining an invalid value
#define TBA_DEFINE_HANDLE_CLASS_VALUE(class_name, handle_type, invalid_value) \
  class_name():base_type(invalid_value) { } \
  class_name(const handle_type & nhandle):base_type(nhandle) { BOOST_STATIC_ASSERT(!Owned::value); } \
  template <typename OtherOwned> class_name(const class_name<OtherOwned> & other):base_type(other.Handle()) \
  { BOOST_STATIC_ASSERT(!Owned::value); }

}

#endif
