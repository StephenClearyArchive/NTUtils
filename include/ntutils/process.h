// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_PROCESS_H
#define NTUTILS_PROCESS_H

#include "ntutils/basic.h"

namespace ntutils {

template <typename Owned = unowned>
struct process: generic_null_handle_base<process<Owned> >
{
  typedef generic_null_handle_base<process<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(process, HANDLE)

  void OpenProcess(const DWORD pid, const DWORD access = PROCESS_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::OpenProcess(access, inherit, pid));
  }
  void open_process(const DWORD pid, const DWORD access = PROCESS_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = ::OpenProcess(access, inherit, pid);
    if (nhandle == 0)
      throw Win32_error(TEXT("OpenProcess"));
    this->Reset(nhandle);
  }

  DWORD GetPriorityClass() const { return ::GetPriorityClass(this->Handle()); }
  DWORD get_priority_class() const
  {
    const DWORD ret = GetPriorityClass();
    if (ret == 0)
      throw Win32_error(TEXT("GetPriorityClass"));
    return ret;
  }

  BOOL SetPriorityClass(const DWORD level) const { return ::SetPriorityClass(this->Handle(), level); }
  void set_priority_class(const DWORD level) const
  {
    if (!SetPriorityClass(level))
      throw Win32_error(TEXT("SetPriorityClass"));
  }
};

}

#endif
