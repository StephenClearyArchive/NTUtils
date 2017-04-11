// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_SHLWAPI_H
#define NTUTILS_SHLWAPI_H

namespace ntutils {

static inline void PortablePathRemoveExtension(const LPTSTR str)
{
  const LPTSTR str_end = str + _tcslen(str);
  for (LPTSTR i = str_end - 1; i >= str; --i)
  {
    if (*i == TEXT('/') || *i == TEXT('\\'))
      return;
    if (*i != TEXT('.'))
      continue;
    *i = 0;
    return;
  }
}

}

#endif
