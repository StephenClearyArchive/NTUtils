// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_THREAD_H
#define NTUTILS_THREAD_H

#include "ntutils/basic.h"
#include "ntutils/kernel32_dll.h"
#include "ntutils/ntdll_dll.h"

namespace ntutils {

inline HANDLE PortableOpenThread(const DWORD access, const BOOL inherit, const DWORD thread_id)
{
  // OpenThread exists in ME and 2K+
  if (singleton<kernel32_OpenThread>::instance()())
    return singleton<kernel32_OpenThread>::instance()()(access, inherit, thread_id);

  // NtOpenThread exists in NT
  if (singleton<ntdll_NtOpenThread>::instance()())
  {
    OBJECT_ATTRIBUTES attr;
    InitializeObjectAttributes(&attr, 0, 0, 0, 0);
    if (inherit)
      attr.Attributes = OBJ_INHERIT;
    CLIENT_ID id;
    id.UniqueProcess = 0;
    id.UniqueThread = (HANDLE) thread_id;
    HANDLE ret;
    const NTSTATUS err = singleton<ntdll_NtOpenThread>::instance()()(&ret, access, &attr, &id);
    if (!NT_SUCCESS(err))
    {
      SetLastError(PortableRtlNtStatusToDosError(err));
      return 0;
    }
    return ret;
  }

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

template <typename Owned = unowned>
struct thread: generic_null_handle_base<thread<Owned> >
{
  typedef generic_null_handle_base<thread<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(thread, HANDLE)

  void OpenThread(const DWORD thread_id, const DWORD access = THREAD_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(PortableOpenThread(access, inherit, thread_id));
  }
  void open_thread(const DWORD thread_id, const DWORD access = THREAD_ALL_ACCESS, const BOOL inherit = FALSE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = PortableOpenThread(access, inherit, thread_id);
    if (nhandle == 0)
      throw Win32_error(TEXT("OpenThread"));
    this->Reset(nhandle);
  }

  DWORD SuspendThread() const { return ::SuspendThread(this->Handle()); }
  DWORD suspend_thread() const
  {
    const DWORD ret = SuspendThread();
    if (ret == (DWORD) -1)
      throw Win32_error(TEXT("SuspendThread"));
    return ret;
  }

  DWORD ResumeThread() const { return ::ResumeThread(this->Handle()); }
  DWORD resume_thread() const
  {
    const DWORD ret = ResumeThread();
    if (ret == (DWORD) -1)
      throw Win32_error(TEXT("ResumeThread"));
    return ret;
  }
};

}

#endif
