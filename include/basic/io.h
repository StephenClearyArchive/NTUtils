// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_IO_H
#define BASIC_IO_H

#include "basic/handle.h"
#include "basic/args.h"
#include "basic/sync.h"

namespace basic {

struct overlapped: OVERLAPPED
{
  overlapped() { ZeroMemory(this, sizeof(*this)); }
};

struct overlapped_event: overlapped
{
  event<owned> event;

  overlapped_event()
  {
    event.create_event(TRUE);
    hEvent = event.Handle();
  }
};

typedef ptr_or_ref_2<DWORD, OVERLAPPED> dword_or_ovl_arg;

template <typename Derived>
struct io_handle_base: generic_invalid_handle_base<Derived>
{
  io_handle_base() { }
  io_handle_base(const HANDLE nhandle):generic_invalid_handle_base<Derived>(nhandle) { }

  BOOL ReadFile(const LPVOID buf, const DWORD buf_size, dword_or_ovl_arg arg) const
  { return ::ReadFile(this->Handle(), buf, buf_size, arg.ptr1(), arg.ptr2()); }
  DWORD read_file_sync(const LPVOID buf, const DWORD buf_size) const
  {
    DWORD ret;
    if (!ReadFile(buf, buf_size, &ret))
      throw Win32_error(TEXT("ReadFile"));
    return ret;
  }
  BOOL read_file_async(const LPVOID buf, const DWORD buf_size, ptr_or_ref<OVERLAPPED> ovl) const
  {
    const BOOL ret = ReadFile(buf, buf_size, ovl.ptr());
    if (!ret && GetLastError() != ERROR_HANDLE_EOF && GetLastError() != ERROR_IO_PENDING)
      throw Win32_error(TEXT("ReadFile"));
    return ret;
  }

  BOOL WriteFile(const LPCVOID buf, const DWORD buf_size, dword_or_ovl_arg arg) const
  { return ::WriteFile(this->Handle(), buf, buf_size, arg.ptr1(), arg.ptr2()); }
  BOOL WriteFile_sync(const LPCVOID buf, const DWORD buf_size) const
  {
    DWORD junk;
    return WriteFile(buf, buf_size, &junk);
  }
  void write_file_sync(const LPCVOID buf, const DWORD buf_size) const
  {
    if (!WriteFile_sync(buf, buf_size))
      throw Win32_error(TEXT("WriteFile"));
  }
  BOOL write_file_async(const LPCVOID buf, const DWORD buf_size, ptr_or_ref<OVERLAPPED> ovl) const
  {
    const BOOL ret = WriteFile(buf, buf_size, ovl.ptr());
    if (!ret && GetLastError() != ERROR_IO_PENDING)
      throw Win32_error(TEXT("WriteFile"));
    return ret;
  }

  BOOL FlushFileBuffers() const { return ::FlushFileBuffers(this->Handle()); }
  void flush_file_buffers() const
  {
    if (!FlushFileBuffers())
      throw Win32_error(TEXT("FlushFileBuffers"));
  }

  BOOL CancelIo() const { return ::CancelIo(this->Handle()); }
  void cancel_io() const
  {
    if (!CancelIo())
      throw Win32_error(TEXT("CancelIo"));
  }
};

}

#endif
