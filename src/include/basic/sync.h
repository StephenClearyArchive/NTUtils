// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_SYNC_H
#define BASIC_SYNC_H

#include "basic/handle.h"

namespace basic {

template <typename Owned = unowned>
struct event: generic_null_handle_base<event<Owned> >
{
  typedef generic_null_handle_base<event<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(event, HANDLE)

  void CreateEvent(const BOOL manual, const BOOL initial = FALSE, const LPCTSTR name = 0, const LPSECURITY_ATTRIBUTES sec = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::CreateEvent(sec, manual, initial, name));
  }
  void create_event(const BOOL manual, const BOOL initial = FALSE, const LPCTSTR name = 0, const LPSECURITY_ATTRIBUTES sec = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = ::CreateEvent(sec, manual, initial, name);
    if (nhandle == 0)
      throw Win32_error(TEXT("CreateEvent"));
    this->Reset(nhandle);
  }

  void OpenEvent(const LPCTSTR name, const DWORD access = EVENT_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::OpenEvent(access, inherit, name));
  }
  void open_event(const LPCTSTR name, const DWORD access = EVENT_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = ::OpenEvent(access, inherit, name);
    if (nhandle == 0)
      throw Win32_error(TEXT("OpenEvent"));
    this->Reset(nhandle);
  }

  BOOL ResetEvent() const { return ::ResetEvent(this->Handle()); }
  void reset_event() const
  {
    if (!ResetEvent())
      throw Win32_error(TEXT("ResetEvent"));
  }

  BOOL SetEvent() const { return ::SetEvent(this->Handle()); }
  void set_event() const
  {
    if (!SetEvent())
      throw Win32_error(TEXT("SetEvent"));
  }

  BOOL PulseEvent() const { return ::PulseEvent(this->Handle()); }
  void pulse_event() const
  {
    if (!PulseEvent())
      throw Win32_error(TEXT("PulseEvent"));
  }
};

}

#endif
