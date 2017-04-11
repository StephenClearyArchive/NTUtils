// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_NTDLL_DLL_H
#define NTUTILS_NTDLL_DLL_H

#include <ddk/ntapi.h>

#include "ntutils/basic.h"

namespace ntutils {

// All times in these structures are equivalent to FILETIME structures
struct SYSTEM_THREADS_NT4
{
  LARGE_INTEGER KernelTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER CreateTime;
  ULONG WaitTime;
  PVOID StartAddress;
  CLIENT_ID ClientId;
  ULONG Priority;
  ULONG BasePriority;
  ULONG ContextSwitchCount;
  ULONG State;
  ULONG WaitReason;
};

struct SYSTEM_PROCESSES_NT4
{
  ULONG NextEntryOffset;
  ULONG ThreadCount;
  ULONG Reserved1[6];
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ProcessName;
  ULONG BasePriority;
  ULONG ProcessId;
  ULONG InheritedFromProcessId;
  ULONG HandleCount;
  ULONG SessionId;
  ULONG PageDirectoryFrame;
  ULONG PeakVirtualSize;
  ULONG VirtualSize;
  ULONG PageFaultCount;
  ULONG PeakWorkingSetSize;
  ULONG WorkingSetSize;
  ULONG QuotaPeakPagedPoolUsage;
  ULONG QuotaPagedPoolUsage;
  ULONG QuotaPeakNonPagedPoolUsage;
  ULONG QuotaNonPagedPoolUsage;
  ULONG PagefileUsage;
  ULONG PeakPagefileUsage;
  ULONG unknown_; // Some claim this is "PrivatePageCount", others "CommitCharge" (in bytes)
  SYSTEM_THREADS_NT4 Threads[1];
};

TBA_DEFINE_OPTIONAL_DLL(ntdll);

typedef NTSTATUS (* __stdcall ntdll_NtOpenThreadProc)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
TBA_DEFINE_OPTIONAL_PROC(ntdll, NtOpenThread);

typedef NTSTATUS (* __stdcall ntdll_NtQuerySystemInformationProc)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
TBA_DEFINE_OPTIONAL_PROC(ntdll, NtQuerySystemInformation);

typedef ULONG (* __stdcall ntdll_RtlNtStatusToDosErrorProc)(NTSTATUS);
TBA_DEFINE_OPTIONAL_PROC(ntdll, RtlNtStatusToDosError);

inline ULONG PortableRtlNtStatusToDosError(const NTSTATUS err)
{
  if (singleton<ntdll_RtlNtStatusToDosError>::instance()())
    return singleton<ntdll_RtlNtStatusToDosError>::instance()()(err);
  return ERROR_MR_MID_NOT_FOUND;
}

}

#endif
