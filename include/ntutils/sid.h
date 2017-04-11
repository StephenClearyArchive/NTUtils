// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_SID_H
#define NTUTILS_SID_H

#include "ntutils/basic.h"

namespace ntutils {

template <typename Owned = unowned>
struct sid: handle_base<sid<Owned>, PSID>
{
  typedef handle_base<sid<Owned>, PSID> base_type;
  TBA_DEFINE_HANDLE_CLASS_VALUE(sid, PSID, 0)

  static PSID InvalidValue() { return 0; }
  bool Valid() const { return (this->Handle() != InvalidValue()); }

  BOOL Close() { return !FreeSid(this->Handle()); }
  void close() { if (!Close()) throw error(TEXT("FreeSid failed")); }

  void AllocateAndInitializeSid(const PSID_IDENTIFIER_AUTHORITY auth, const BYTE count, const DWORD sub0, const DWORD sub1 = 0,
      const DWORD sub2 = 0, const DWORD sub3 = 0, const DWORD sub4 = 0, const DWORD sub5 = 0, const DWORD sub6 = 0, const DWORD sub7 = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    PSID nhandle;
    if (::AllocateAndInitializeSid(auth, count, sub0, sub1, sub2, sub3, sub4, sub5, sub6, sub7, &nhandle))
      this->Reset(nhandle);
    else
      this->Reset(0);
  }
  void allocate_and_initialize_sid(const PSID_IDENTIFIER_AUTHORITY auth, const BYTE count, const DWORD sub0, const DWORD sub1 = 0,
      const DWORD sub2 = 0, const DWORD sub3 = 0, const DWORD sub4 = 0, const DWORD sub5 = 0, const DWORD sub6 = 0, const DWORD sub7 = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    PSID nhandle;
    if (!::AllocateAndInitializeSid(auth, count, sub0, sub1, sub2, sub3, sub4, sub5, sub6, sub7, &nhandle))
      throw Win32_error(TEXT("AllocateAndInitializeSid"));
    this->Reset(nhandle);
  }

  BOOL IsValidSid() const { return ::IsValidSid(this->Handle()); }

  DWORD GetLengthSid() const { return ::GetLengthSid(this->Handle()); }
};

}

#endif
