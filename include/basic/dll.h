// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_DLL_H
#define BASIC_DLL_H

#include "basic/handle.h"
#include "basic/string.h"
#include "basic/singleton.h"

namespace basic {

template <typename Owned = unowned>
struct module: handle_base<module<Owned>, HMODULE>
{
  typedef handle_base<module<Owned>, HMODULE> base_type;
  TBA_DEFINE_HANDLE_CLASS_VALUE(module, HMODULE, 0)

  static HMODULE InvalidValue() { return 0; }
  bool Valid() const { return (this->Handle() != InvalidValue()); }

  BOOL Close() { return FreeLibrary(this->Handle()); }
  bool close() { if (!Close()) throw Win32_error(TEXT("FreeLibrary")); }

  void LoadLibrary(const_str_ptr name)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::LoadLibrary(name));
  }
  void load_library(const_str_ptr name)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HMODULE nhandle = ::LoadLibrary(name);
    if (nhandle == 0)
      throw Win32_error(TEXT("LoadLibrary"));
    this->Reset(nhandle);
  }

  void GetModuleHandle(const_str_ptr name)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(::GetModuleHandle(name));
  }
  void get_module_handle(const_str_ptr name)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HMODULE nhandle = ::GetModuleHandle(name);
    if (nhandle == 0)
      throw Win32_error(TEXT("GetModuleHandle"));
    this->Reset(nhandle);
  }

  FARPROC GetProcAddress(const_ANSI_str_ptr name) const { return ::GetProcAddress(this->Handle(), name); }
  FARPROC get_proc_address(const_ANSI_str_ptr name) const
  {
    const FARPROC ret = GetProcAddress(name);
    if (ret == 0)
      throw Win32_error(TEXT("GetProcAddress"));
    return ret;
  }
};

struct optional_dll: module<owned>
{
  explicit optional_dll(const_str_ptr name)
  { module<owned>::LoadLibrary(name); }

  FARPROC GetProcAddress(const_ANSI_str_ptr name) const
  {
    if (!Valid())
      return 0;
    return module<owned>::GetProcAddress(name);
  }
};

template <typename FPtr, typename OptionalDll>
struct optional_proc
{
  typedef FPtr proc_type;
  FPtr proc;

  FPtr operator()() const { return proc; }

  explicit optional_proc(const_ANSI_str_ptr name)
  :proc((FPtr) singleton<OptionalDll>::instance().GetProcAddress(name)) { }
};

#define TBA_DEFINE_OPTIONAL_DLL(dll) \
struct dll ## _dll: optional_dll \
{ dll ## _dll():optional_dll(TEXT(#dll ".dll")) { } }

#define TBA_DEFINE_OPTIONAL_PROC(dll, proc) \
struct dll ## _ ## proc: optional_proc<dll ## _ ## proc ## Proc, dll ## _dll> \
{ dll ## _ ## proc():optional_proc<dll ## _ ## proc ## Proc, dll ## _dll>(#proc) { } }

}

#endif
