// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_KERNEL32_DLL_H
#define NTUTILS_KERNEL32_DLL_H

#include "ntutils/basic.h"

namespace ntutils {

TBA_DEFINE_OPTIONAL_DLL(kernel32);

typedef HANDLE (* __stdcall kernel32_OpenThreadProc)(DWORD, BOOL, DWORD);
TBA_DEFINE_OPTIONAL_PROC(kernel32, OpenThread);

}

#endif
