// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#include <set>

#include "ntutils/process.h"
#include "ntutils/processes.h"
#include "ntutils/thread.h"
#include "ntutils/token.h"

using namespace ntutils;

static string priority_name(const DWORD level)
{
  switch (level)
  {
    case ABOVE_NORMAL_PRIORITY_CLASS: return "ABOVE_NORMAL";
    case BELOW_NORMAL_PRIORITY_CLASS: return "BELOW_NORMAL";
    case HIGH_PRIORITY_CLASS: return "HIGH";
    case IDLE_PRIORITY_CLASS: return "IDLE";
    case NORMAL_PRIORITY_CLASS: return "NORMAL";
    case REALTIME_PRIORITY_CLASS: return "REALTIME";
    default: return to_string(level);
  }
}

// The main work function
static string ntpriority(const bool running_local, const process_selector & selector, const DWORD level, const bool test)
{
  std::map<DWORD, string> processes = selector.select_processes();

  // Make sure none of the process ids are for our process; this could happen if the
  //  process to be suspended/resumed exited/was terminated just before this process
  //  was started.
  // We treat this just as though we could not find the process id.
  processes.erase(GetCurrentProcessId());
  selector.validate_process_list(processes.empty());

  enable_debug_privilege(running_local);

  string ret;
  for (std::map<DWORD, string>::const_iterator i = processes.begin(); i != processes.end(); ++i)
  {
    if (test)
    {
      ret += i->second + TEXT(" (") + to_string(i->first) + TEXT(") ");
      try
      {
        process<owned> process;
        // Win32 API bug: For some reason, NT wants additional access beyond what's documented
        process.OpenProcess(i->first, PROCESS_ALL_ACCESS);
        if (!process.Valid())
          process.open_process(i->first, PROCESS_QUERY_INFORMATION);
        ret += priority_name(process.get_priority_class()) + TEXT("\n");
      }
      catch (const error & e)
      {
        ret += TEXT("unknown: ") + e.twhat() + TEXT("\n");
      }
    }
    else
    {
      process<owned> process;
      process.open_process(i->first, PROCESS_SET_INFORMATION);
      process.set_priority_class(level);
      ret += i->second + TEXT(" (") + to_string(i->first) + TEXT(") priority set to ") + priority_name(level) + TEXT("\n");
    }
  }

  return ret;
}
