// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#include <set>
#include <map>

#include "ntutils/toolhelp.h"
#include "ntutils/thread.h"
#include "ntutils/shlwapi_dll.h"
#include "ntutils/token.h"

using namespace ntutils;

// Determines the suspend count for a process
static DWORD process_suspend_count(const DWORD process_id)
{
  // Examine all threads in our snapshot of that process to make sure they're all suspended
  // Since we cannot examine the running state of the thread, we suspend and resume each one
  //  to get the current suspend count.

  DWORD ret = 0xFFFFFFFF;

  std::set<DWORD> threads_examined;
  unsigned num_threads_examined;

  do
  {
    // Remember how many threads we've already examined
    num_threads_examined = threads_examined.size();

    // Grab a snapshot
    tool_help_snapshot<owned> snapshot;
    snapshot.create(TH32CS_SNAPTHREAD);

    // Examine all threads in our snapshot of that process
    for (tool_help_thread_iterator i = snapshot.threads_begin(); i != snapshot.threads_end(); ++i)
    {
      if (i->th32OwnerProcessID != process_id)
        continue;

      // Open the thread handle; if an error occurs, it could be that thread just exited or we
      //  don't have access to it
      thread<owned> thread;
      thread.open_thread(i->th32ThreadID, THREAD_SUSPEND_RESUME);

      // Suspend and resume the thread
      // If the suspend fails, we throw a normal error
      // If the resume fails, we throw a special error indicating that the process is messed up
      const DWORD suspend_count = thread.suspend_thread();
      if (thread.ResumeThread() == (DWORD) -1)
        throw error(TEXT("Process ") + to_string(process_id) + TEXT(" is now in an invalid state due to ") +
            Win32_error(TEXT("ResumeThread")).twhat());

      // Remember that we examined this thread
      threads_examined.insert(i->th32ThreadID);

      ret = std::min(ret, suspend_count);
    }

    // Loop (re-creating the snapshot) until we go through without seeing any additional threads
  } while (num_threads_examined != threads_examined.size());

  if (num_threads_examined == 0)
    throw error(TEXT("Process ") + to_string(process_id) + TEXT(" not found"));

  return ret;
}

// Returns true if a process is already suspended; throws an exception if the process id is unknown
static inline bool process_is_suspended(const DWORD process_id) { return (process_suspend_count(process_id) != 0); }

// Returns all processes matching the prefix 'name'
inline static std::map<DWORD, string> find_process(const string & name, const bool exact_match)
{
  std::map<DWORD, string> ret;
  tool_help_snapshot<owned> snapshot;
  snapshot.create(TH32CS_SNAPPROCESS);
  for (tool_help_process_iterator i = snapshot.processes_begin(); i != snapshot.processes_end(); ++i)
  {
    if (exact_match)
    {
      // This test only allows exact matches
      if (!_tcsicmp(i->szExeFile, name.c_str()))
        ret[i->th32ProcessID] = i->szExeFile;
      else
      {
        // Also allow exact matches on the process base name
        PROCESSENTRY32 proc = *i;
        PortablePathRemoveExtension(proc.szExeFile);
        if (!_tcsicmp(proc.szExeFile, name.c_str()))
          ret[i->th32ProcessID] = i->szExeFile;
      }
    }
    else
    {
      // This test allows, e.g., "proc.exe" to only match processes called "proc.exe"
      //  while also allowing "proc" to match processes called "proc.exe" or "proc.com"
      if (!_tcsnicmp(i->szExeFile, name.c_str(), name.length()))
        ret[i->th32ProcessID] = i->szExeFile;
    }
  }
  return ret;
}

// Returns all processes matching the process id
inline static std::map<DWORD, string> find_process(const DWORD pid)
{
  std::map<DWORD, string> ret;
  tool_help_snapshot<owned> snapshot;
  snapshot.create(TH32CS_SNAPPROCESS);
  for (tool_help_process_iterator i = snapshot.processes_begin(); i != snapshot.processes_end(); ++i)
  {
    if (i->th32ProcessID == pid)
    {
      ret[i->th32ProcessID] = i->szExeFile;
      break;
    }
  }
  return ret;
}

inline static void resume_process(const DWORD process_id)
{
  // We want to make sure this process is suspended before we resume it, or this could
  //  cause some rather nasty problems...
  if (!process_is_suspended(process_id))
    throw error(TEXT("Process ") + to_string(process_id) + TEXT(" is not suspended"));

  // Grab a snapshot of all threads
  tool_help_snapshot<owned> snapshot;
  snapshot.create(TH32CS_SNAPTHREAD);

  // All threads are suspended; resume each one once
  // If any error occurs after some of the threads have been resumed, wail in despair
  bool process_state_invalid = false;
  try
  {
    for (tool_help_thread_iterator i = snapshot.threads_begin(); i != snapshot.threads_end(); ++i)
    {
      if (i->th32OwnerProcessID != process_id)
        continue;

      // Open the thread handle
      thread<owned> thread;
      thread.open_thread(i->th32ThreadID, THREAD_SUSPEND_RESUME);

      // Resume the thread
      thread.resume_thread();

      // Once one thread has been successfully resumed, any errors will leave
      //  the process in an invalid state
      process_state_invalid = true;
    }
  }
  catch (const error & e)
  {
    if (process_state_invalid)
      throw error(TEXT("Process ") + to_string(process_id) + TEXT(" is now in an invalid state due to ") + e.twhat());
    else
      throw;
  }

  // If we didn't find any threads for this process, error
  if (!process_state_invalid)
    throw error(TEXT("Process ") + to_string(process_id) + TEXT(" not found"));
}

inline static void suspend_process(const int process_id)
{
  // In order to properly suspend a process, we first suspend all threads in that process,
  //  keeping track of which thread id's we suspended. Then, we re-examine the threads for
  //  that process and suspend any that we missed the first time. This is continued until
  //  there are no remaining threads to be suspended.
  // This loop is necessary to ensure that this program will work even in the unlikely
  //  (but possible) situation where one of the process's threads starts another thread
  //  after we take our first snapshot but before we suspend it.

  // We want to make sure this process is running before we suspend it, or this could
  //  cause problems as threads reach their maximum suspend count.
  if (process_is_suspended(process_id))
    throw error(TEXT("Process ") + to_string(process_id) + TEXT(" is already suspended"));

  std::set<DWORD> threads_suspended;
  unsigned num_threads_suspended;

  try
  {
    do
    {
      // Remember how many threads we've already suspended
      num_threads_suspended = threads_suspended.size();

      // Grab a snapshot
      tool_help_snapshot<owned> snapshot;
      snapshot.create(TH32CS_SNAPTHREAD);

      // Suspend all threads in our snapshot of that process
      for (tool_help_thread_iterator i = snapshot.threads_begin(); i != snapshot.threads_end(); ++i)
      {
        if (i->th32OwnerProcessID != process_id)
          continue;

        // Only suspend threads that we haven't already suspended
        if (threads_suspended.find(i->th32ThreadID) != threads_suspended.end())
          continue;

        // Open the thread handle
        thread<owned> thread;
        thread.open_thread(i->th32ThreadID, THREAD_SUSPEND_RESUME);

        // Suspend the thread
        thread.suspend_thread();

        // Remember that we suspended this thread
        threads_suspended.insert(i->th32ThreadID);
      }

      // Loop until we go through without suspending additional threads
    } while (num_threads_suspended != threads_suspended.size());
  }
  catch (const error & e)
  {
    if (!threads_suspended.empty())
      throw error(TEXT("Process ") + to_string(process_id) + TEXT(" is now in an invalid state due to ") + e.twhat());
    else
      throw;
  }

  if (num_threads_suspended == 0)
    throw error(TEXT("Process ") + to_string(process_id) + TEXT(" not found"));
}

// The main work function
static string ntsuspend(const string & name, const bool exact_match, const DWORD pid, const bool resume, const bool test)
{
  std::map<DWORD, string> processes;
  if (test && name.empty() && pid == 0)
  {
    tool_help_snapshot<owned> snapshot;
    snapshot.create(TH32CS_SNAPPROCESS);
    for (tool_help_process_iterator i = snapshot.processes_begin(); i != snapshot.processes_end(); ++i)
      processes[i->th32ProcessID] = i->szExeFile;
  }
  else if (!name.empty())
    processes = find_process(name, exact_match);
  else
    processes = find_process(pid);

  // Make sure none of the process ids are for our process; this could happen if the
  //  process to be suspended/resumed exited/was terminated just before this process
  //  was started.
  // We treat this just as though we could not find the process id.
  // We also filter out the idle process (id 0).
  processes.erase(GetCurrentProcessId());
  processes.erase(0);
  if (processes.empty())
  {
    if (name.empty())
      throw error(TEXT("Could not find process ") + to_string(pid));
    else
      throw error(TEXT("Could not find process ") + name);
  }

  // Enable the debug privilege, if we have it available to us
  // This allows us to test/suspend/resume processes running under other user accounts
  // If the debug privilege is not available, this code will do nothing
  token<owned> process_token;
  process_token.OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES);
  process_token.Enable_Privilege(SE_DEBUG_NAME);

  string ret;
  for (std::map<DWORD, string>::const_iterator i = processes.begin(); i != processes.end(); ++i)
  {
    if (test)
    {
      ret += i->second + TEXT(" (") + to_string(i->first) + TEXT(") ");
      try
      {
        if (process_is_suspended(i->first))
          ret += TEXT("suspended\n");
        else
          ret += TEXT("running\n");
      }
      catch (const error & e)
      {
        ret += TEXT("unknown: ") + e.twhat() + TEXT("\n");
      }
    }
    else if (resume)
    {
      resume_process(i->first);
      ret += i->second + TEXT(" (") + to_string(i->first) + TEXT(") resumed\n");
    }
    else
    {
      suspend_process(i->first);
      ret += i->second + TEXT(" (") + to_string(i->first) + TEXT(") suspended\n");
    }
  }

  return ret;
}