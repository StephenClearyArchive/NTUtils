// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_TLHELP32_DLL_H
#define NTUTILS_TLHELP32_DLL_H

#include <tlhelp32.h>

#include "ntutils/kernel32_dll.h"
#include "ntutils/ntdll_dll.h"

namespace ntutils {

typedef HANDLE (* __stdcall kernel32_CreateToolhelp32SnapshotProc)(DWORD, DWORD);
TBA_DEFINE_OPTIONAL_PROC(kernel32, CreateToolhelp32Snapshot);

typedef BOOL (* __stdcall kernel32_Process32FirstProc)(HANDLE, LPPROCESSENTRY32);
TBA_DEFINE_OPTIONAL_PROC(kernel32, Process32First);

typedef BOOL (* __stdcall kernel32_Process32NextProc)(HANDLE, LPPROCESSENTRY32);
TBA_DEFINE_OPTIONAL_PROC(kernel32, Process32Next);

typedef BOOL (* __stdcall kernel32_Thread32FirstProc)(HANDLE, LPTHREADENTRY32);
TBA_DEFINE_OPTIONAL_PROC(kernel32, Thread32First);

typedef BOOL (* __stdcall kernel32_Thread32NextProc)(HANDLE, LPTHREADENTRY32);
TBA_DEFINE_OPTIONAL_PROC(kernel32, Thread32Next);

// Note: this is not an exact duplication of the Win32 Toolhelp API:
//  . The returned handle may not be closeable using CloseHandle;
//    close with PortableCloseToolhelp32Snapshot instead
//  . The only supported flags are TH32CS_SNAPPROCESS and TH32CS_SNAPTHREAD
//  . dwSize of the structures is ignored, and only the reliable fields are set
inline HANDLE PortableCreateToolhelp32Snapshot(const DWORD flags, const DWORD process_id)
{
  if (singleton<kernel32_CreateToolhelp32Snapshot>::instance()())
    return singleton<kernel32_CreateToolhelp32Snapshot>::instance()()(flags, process_id);

  // Check to make sure no unsupported flags are passed
  if ((flags & ~TH32CS_SNAPPROCESS & ~TH32CS_SNAPTHREAD) != 0)
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
  }

  if (!singleton<ntdll_NtQuerySystemInformation>::instance()())
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
  }

  try
  {
    unsigned size = 64 * sizeof(SYSTEM_PROCESSES_NT4);
    char * ret = new char[size];

    NTSTATUS err;
    while ((err = singleton<ntdll_NtQuerySystemInformation>::instance()()(
        SystemProcessesAndThreadsInformation, ret, size, 0)) == STATUS_INFO_LENGTH_MISMATCH)
    {
      size *= 2;
      delete [] ret;
      ret = new char[size];
    }

    if (!NT_SUCCESS(err))
    {
      SetLastError(PortableRtlNtStatusToDosError(err));
      return 0;
    }

    return (HANDLE) ret;
  }
  catch (const std::bad_alloc &)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

inline BOOL PortableCloseToolhelp32Snapshot(const HANDLE handle)
{
  if (singleton<kernel32_CreateToolhelp32Snapshot>::instance()())
    return CloseHandle(handle);

  delete [] (char *) handle;
  return TRUE;
}

inline BOOL PortableProcess32_copy_data(const LPPROCESSENTRY32 data, const SYSTEM_PROCESSES_NT4 * const proc)
{
  data->th32ProcessID = proc->ProcessId;
  data->cntThreads = proc->ThreadCount;
  data->th32ParentProcessID = proc->InheritedFromProcessId;
  data->pcPriClassBase = proc->BasePriority;
#ifdef UNICODE
  const int n = std::min<int>(MAX_PATH - 1, proc->ProcessName.Length);
  wcsncpy(data->szExeFile, proc->ProcessName.Buffer, n);
  data->szExeFile[n] = 0;
#else
  if (proc->ProcessName.Buffer == 0)
    strcpy(data->szExeFile, "Idle");
  else if (!WideCharToMultiByte(CP_ACP, 0, proc->ProcessName.Buffer, proc->ProcessName.Length, data->szExeFile, MAX_PATH, 0, 0))
    return FALSE;
#endif
  return TRUE;
}

inline void PortableThread32_copy_data(const LPTHREADENTRY32 data, const SYSTEM_PROCESSES_NT4 * const proc, const unsigned thread)
{
  data->th32ThreadID = (DWORD) proc->Threads[thread].ClientId.UniqueThread;
  data->th32OwnerProcessID = proc->ProcessId;
  data->tpBasePri = proc->Threads[thread].BasePriority;
}

inline BOOL PortableProcess32First(const HANDLE handle, const LPPROCESSENTRY32 data)
{
  if (singleton<kernel32_Process32First>::instance()())
    return singleton<kernel32_Process32First>::instance()()(handle, data);

  return PortableProcess32_copy_data(data, (const SYSTEM_PROCESSES_NT4 *) handle);
}

inline BOOL PortableProcess32Next(const HANDLE handle, const LPPROCESSENTRY32 data)
{
  if (singleton<kernel32_Process32Next>::instance()())
    return singleton<kernel32_Process32Next>::instance()()(handle, data);

  // Find our current spot in our snapshot
  const SYSTEM_PROCESSES_NT4 * proc;
  for (proc = (const SYSTEM_PROCESSES_NT4 *) handle; proc != 0;
      proc = ((proc->NextEntryOffset == 0) ? 0 : (const SYSTEM_PROCESSES_NT4 *) ((const char *) proc + proc->NextEntryOffset)))
    if (proc->ProcessId == data->th32ProcessID)
      break;
  if (proc == 0)
  {
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
  }

  // Increment (if possible) and return

  if (proc->NextEntryOffset == 0)
  {
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
  }

  proc = (const SYSTEM_PROCESSES_NT4 *) ((const char *) proc + proc->NextEntryOffset);
  return PortableProcess32_copy_data(data, proc);
}

inline BOOL PortableThread32First(const HANDLE handle, const LPTHREADENTRY32 data)
{
  if (singleton<kernel32_Thread32First>::instance()())
    return singleton<kernel32_Thread32First>::instance()()(handle, data);

  PortableThread32_copy_data(data, (const SYSTEM_PROCESSES_NT4 *) handle, 0);
  return TRUE;
}

inline BOOL PortableThread32Next(const HANDLE handle, const LPTHREADENTRY32 data)
{
  if (singleton<kernel32_Thread32Next>::instance()())
    return singleton<kernel32_Thread32Next>::instance()()(handle, data);

  // Find the current process
  const SYSTEM_PROCESSES_NT4 * proc;
  for (proc = (const SYSTEM_PROCESSES_NT4 *) handle; proc != 0;
      proc = ((proc->NextEntryOffset == 0) ? 0 : (const SYSTEM_PROCESSES_NT4 *) ((const char *) proc + proc->NextEntryOffset)))
    if (proc->ProcessId == data->th32OwnerProcessID)
      break;
  if (proc == 0)
  {
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
  }

  // Find the current thread
  unsigned thread;
  for (thread = 0; thread != proc->ThreadCount; ++thread)
    if ((DWORD) proc->Threads[thread].ClientId.UniqueThread == data->th32ThreadID)
      break;
  if (thread == proc->ThreadCount)
  {
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
  }

  // Increment (if possible) and return

  ++thread;
  if (thread != proc->ThreadCount)
  {
    PortableThread32_copy_data(data, proc, thread);
    return TRUE;
  }

  // Reached the end of the thread list for that process, so start with next process

  if (proc->NextEntryOffset == 0)
  {
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
  }

  proc = (const SYSTEM_PROCESSES_NT4 *) ((const char *) proc + proc->NextEntryOffset);
  PortableThread32_copy_data(data, proc, 0);
  return TRUE;
}

}

#endif
