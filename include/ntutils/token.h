// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

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

  void OpenThreadToken(const HANDLE thread, const DWORD access = TOKEN_ALL_ACCESS, const BOOL as_self = TRUE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    HANDLE nhandle;
    if (::OpenThreadToken(thread, access, as_self, &nhandle))
      this->Reset(nhandle);
    else
      this->Reset(INVALID_HANDLE_VALUE);
  }
  void open_thread_token(const HANDLE thread, const DWORD access = TOKEN_ALL_ACCESS, const BOOL as_self = TRUE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    HANDLE nhandle;
    if (!::OpenThreadToken(thread, access, as_self, &nhandle))
      throw Win32_error(TEXT("OpenThreadToken"));
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
  BOOL Enable_Privilege(const_str_ptr name) const
  {
    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid))
      return FALSE;
    return AdjustTokenPrivileges(&priv);
  }
  void enable_privilege(const_str_ptr name) const
  {
    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(0, name, &priv.Privileges[0].Luid))
      throw Win32_error(TEXT("LookupPrivilegeValue"));
    adjust_token_privileges(&priv);
  }

  BOOL GetTokenInformation(const TOKEN_INFORMATION_CLASS type, const LPVOID buf, const DWORD buf_size, const PDWORD written) const
  { return ::GetTokenInformation(this->Handle(), type, buf, buf_size, written); }
  DWORD get_token_information(const TOKEN_INFORMATION_CLASS type, const LPVOID buf, const DWORD buf_size) const
  {
    DWORD ret;
    if (!GetTokenInformation(type, buf, buf_size, &ret))
      throw Win32_error(TEXT("GetTokenInformation"));
    return ret;
  }
  BOOL Get_Token_Impersonation_Level(ptr_or_ref<SECURITY_IMPERSONATION_LEVEL> ret) const
  {
    DWORD tmp;
    SetLastError(0);
    if (!GetTokenInformation(TokenImpersonationLevel, ret.ptr(), sizeof(SECURITY_IMPERSONATION_LEVEL), &tmp))
      return FALSE;
    if (tmp != sizeof(SECURITY_IMPERSONATION_LEVEL))
      return FALSE;
    return TRUE;
  }
  SECURITY_IMPERSONATION_LEVEL get_token_impersonation_level() const
  {
    SECURITY_IMPERSONATION_LEVEL ret;
    if (!Get_Token_Impersonation_Level(ret))
      throw Win32_error(TEXT("GetTokenInformation (TokenImpersonationLevel)"));
    return ret;
  }
};

// Enable the debug privilege, if we have it available to us
// This function attempts to enable the debug privilege for an impersonating
//  thread or, if the thread is not impersonating, for the executing process
//  (This allows us to operate on processes running under other user accounts)
// If the debug privilege is not available, this code will do nothing
static inline void enable_debug_privilege(const bool allow_process_token)
{
  token<owned> token;
  token.OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES);
  if (!token.Valid() && allow_process_token)
    token.OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES);
  token.Enable_Privilege(SE_DEBUG_NAME);
}

}

#endif
