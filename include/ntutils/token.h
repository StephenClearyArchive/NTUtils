// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef NTUTILS_TOKEN_H
#define NTUTILS_TOKEN_H

namespace ntutils {

template <typename Owned = unowned>
struct token: generic_invalid_handle_base<token<Owned> >
{
  typedef generic_invalid_handle_base<token<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(token, HANDLE)

  void OpenProcessToken(const HANDLE process, const DWORD access = TOKEN_ALL_ACCESS)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    HANDLE nhandle;
    if (::OpenProcessToken(process, access, &nhandle))
      this->Reset(nhandle);
    else
      this->Reset(INVALID_HANDLE_VALUE);
  }
  void open_process_token(const HANDLE process, const DWORD access = TOKEN_ALL_ACCESS)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    HANDLE nhandle;
    if (!::OpenProcessToken(process, access, &nhandle))
      throw Win32_error(TEXT("OpenProcessToken"));
    this->Reset(nhandle);
  }

  BOOL AdjustTokenPrivileges(const PTOKEN_PRIVILEGES new_state, const BOOL disable_all = FALSE,
      const PTOKEN_PRIVILEGES old_state = 0, const DWORD old_state_size = 0, const PDWORD old_state_written = 0) const
  { return ::AdjustTokenPrivileges(this->Handle(), disable_all, new_state, old_state_size, old_state, old_state_written); }
  void adjust_token_privileges(const PTOKEN_PRIVILEGES new_state, const BOOL disable_all = FALSE,
      const PTOKEN_PRIVILEGES old_state = 0, const DWORD old_state_size = 0, const PDWORD old_state_written = 0) const
  {
    if (!AdjustTokenPrivileges(new_state, disable_all, old_state, old_state_size, old_state_written))
      throw Win32_error(TEXT("AdjustTokenPrivileges"));
  }
  BOOL Enable_Privilege(const LPCTSTR name) const
  {
    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid))
      return FALSE;
    return AdjustTokenPrivileges(&priv);
  }
  void enable_privilege(const LPCTSTR name) const
  {
    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid))
      throw Win32_error(TEXT("LookupPrivilegeValue"));
    adjust_token_privileges(&priv);
  }
};

}

#endif
