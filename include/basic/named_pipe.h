// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_NAMED_PIPE_H
#define BASIC_NAMED_PIPE_H

#include "basic/io.h"

namespace basic {

template <typename Owned = unowned>
struct named_pipe: io_handle_base<named_pipe<Owned> >
{
  typedef io_handle_base<named_pipe<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(named_pipe, HANDLE)

  void CreateNamedPipe(const_str_ptr name, const DWORD open_mode, const DWORD pipe_mode, const DWORD max_instances,
      const DWORD outbuf_size = 0, const DWORD inbuf_size = 0, const DWORD timeout = INFINITE, const LPSECURITY_ATTRIBUTES sec = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::CreateNamedPipe(name, open_mode, pipe_mode, max_instances, outbuf_size, inbuf_size, timeout, sec));
  }
  void create_named_pipe(const_str_ptr name, const DWORD open_mode, const DWORD pipe_mode, const DWORD max_instances,
      const DWORD outbuf_size = 0, const DWORD inbuf_size = 0, const DWORD timeout = INFINITE, const LPSECURITY_ATTRIBUTES sec = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = ::CreateNamedPipe(name, open_mode, pipe_mode, max_instances, outbuf_size, inbuf_size, timeout, sec);
    if (nhandle == INVALID_HANDLE_VALUE)
      throw Win32_error(TEXT("CreateNamedPipe"));
    this->Reset(nhandle);
  }

  BOOL CreateFile(const LPCTSTR name, const DWORD flags = 0, const DWORD access = GENERIC_READ | GENERIC_WRITE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::CreateFile(name, access, 0, 0, OPEN_EXISTING, flags, 0));
  }
  void create_file(const LPCTSTR name, const DWORD flags = 0, const DWORD access = GENERIC_READ | GENERIC_WRITE)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = ::CreateFile(name, access, 0, 0, OPEN_EXISTING, flags, 0);
    if (nhandle == INVALID_HANDLE_VALUE)
      throw Win32_error(TEXT("CreateFile (") + string(name) + TEXT(")"));
    this->Reset(nhandle);
  }

  BOOL ConnectNamedPipe(ptr_or_ref<OVERLAPPED> ovl = ptr_or_ref<OVERLAPPED>()) const { return ::ConnectNamedPipe(this->Handle(), ovl.ptr()); }
  BOOL connect_named_pipe(ptr_or_ref<OVERLAPPED> ovl = ptr_or_ref<OVERLAPPED>()) const
  {
    BOOL ret = ConnectNamedPipe(ovl);
    if (!ret && GetLastError() == ERROR_PIPE_CONNECTED)
      ret = TRUE;
    if (!ret && GetLastError() != ERROR_IO_PENDING)
      throw Win32_error(TEXT("ConnectNamedPipe"));
    return ret;
  }

  static DWORD call_named_pipe(const LPCTSTR name, const LPVOID inbuf, const DWORD inbuf_size, const LPVOID outbuf, const DWORD outbuf_size,
      const DWORD timeout)
  {
    DWORD ret;
    if (!CallNamedPipe(name, inbuf, inbuf_size, outbuf, outbuf_size, &ret, timeout))
      throw Win32_error(TEXT("CallNamedPipe"));
    return ret;
  }

  BOOL ImpersonateNamedPipeClient() { return ::ImpersonateNamedPipeClient(this->Handle()); }
  void impersonate_named_pipe_client()
  {
    if (!ImpersonateNamedPipeClient())
      throw Win32_error(TEXT("ImpersonateNamedPipeClient"));
  }

  BOOL DisconnectNamedPipe() const { return ::DisconnectNamedPipe(this->Handle()); }
  void disconnect_named_pipe() const
  {
    if (!DisconnectNamedPipe())
      throw Win32_error(TEXT("DisconnectNamedPipe"));
  }

  BOOL PeekNamedPipe(const LPVOID buf, const DWORD buf_size, const LPDWORD read, const LPDWORD avail = 0, const LPDWORD msg = 0) const
  { return ::PeekNamedPipe(this->Handle(), buf, buf_size, read, avail, msg); }
  void peek_named_pipe(const LPVOID buf, const DWORD buf_size, const LPDWORD read, const LPDWORD avail = 0, const LPDWORD msg = 0) const
  {
    if (!PeekNamedPipe(buf, buf_size, read, avail, msg))
      throw Win32_error(TEXT("PeekNamedPipe"));
  }
  BOOL Peek_Avail(DWORD & ret) const { return PeekNamedPipe(0, 0, 0, &ret, 0); }
  DWORD peek_avail() const
  {
    DWORD ret;
    peek_named_pipe(0, 0, 0, &ret, 0);
    return ret;
  }
  BOOL Peek_Msg(DWORD & ret) const { return PeekNamedPipe(0, 0, 0, 0, &ret); }
  DWORD peek_msg() const
  {
    DWORD ret;
    peek_named_pipe(0, 0, 0, 0, &ret);
    return ret;
  }

  BOOL SetNamedPipeHandleState(const DWORD mode, const LPDWORD max_collect = 0, const LPDWORD max_count = 0) const
  { return ::SetNamedPipeHandleState(this->Handle(), (LPDWORD) &mode, max_collect, max_count); }
  void set_named_pipe_handle_state(const DWORD mode, const LPDWORD max_collect = 0, const LPDWORD max_count = 0) const
  {
    if (!SetNamedPipeHandleState(mode, max_collect, max_count))
      throw Win32_error(TEXT("SetNamedPipeHandleState"));
  }
};

}

#endif
