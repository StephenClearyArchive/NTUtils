// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef BASIC_PATH_H
#define BASIC_PATH_H

#include "basic/error.h"

namespace basic {

static inline string get_full_path_name(const_str_ptr filename)
{
  LPTSTR junk;
  const DWORD size = GetFullPathName(filename, 0, 0, &junk);
  if (size == 0)
    throw Win32_error(TEXT("GetFullPathName"));

  string ret;
  ret.resize(size);
  const DWORD nsize = GetFullPathName(filename, size, &ret[0], &junk);
  if (nsize == 0)
    throw Win32_error(TEXT("GetFullPathName"));
  if (nsize != size - 1)
    throw error(TEXT("Unexpected result from GetFullPathName"));

  ret.resize(ret.size() - 1);
  return ret;
}

static inline string get_module_file_name(const HMODULE module = 0)
{
  string ret;

  ret.resize(32);
  DWORD nsize = GetModuleFileName(module, &ret[0], ret.size());
  if (nsize == 0)
    throw Win32_error(TEXT("GetModuleFileName"));

  while (nsize == ret.size())
  {
    ret.resize(ret.size() * 2);
    nsize = GetModuleFileName(module, &ret[0], ret.size());
    if (nsize == 0)
      throw Win32_error(TEXT("GetModuleFileName"));
  }

  ret.resize(nsize);
  return ret;
}

}

#endif
