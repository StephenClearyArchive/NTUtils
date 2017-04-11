// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_PROCESSES_H
#define NTUTILS_PROCESSES_H

#include <map>

#include <boost/scoped_array.hpp>

#include "ntutils/console.h"
#include "ntutils/toolhelp.h"
#include "ntutils/shlwapi_dll.h"

namespace ntutils {

// Returns all processes matching the prefix 'name'
inline static std::map<DWORD, string> find_process(const string & name, const bool exact_match)
{
  std::map<DWORD, string> ret;
  tool_help_snapshot<owned> snapshot;
  snapshot.create(TH32CS_SNAPPROCESS);
  for (tool_help_process_iterator i = snapshot.processes_begin(); i != snapshot.processes_end(); ++i)
  {
    if (i->th32ProcessID == 0)
      continue;

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

// Returns all processes
inline static std::map<DWORD, string> find_process()
{
  std::map<DWORD, string> ret;
  tool_help_snapshot<owned> snapshot;
  snapshot.create(TH32CS_SNAPPROCESS);
  for (tool_help_process_iterator i = snapshot.processes_begin(); i != snapshot.processes_end(); ++i)
  {
    if (i->th32ProcessID != 0)
      ret[i->th32ProcessID] = i->szExeFile;
  }
  return ret;
}

// Handles common command-line options and logic for selecting a set of processes
class process_selector
{
  private:
    // Selecting by process id
    DWORD pid;

    // Selecting by process name
    string name;
    bool exact_match;

  public:
    process_selector():pid(0), exact_match(true) { }

    bool handle_option(const option_parser & options)
    {
      switch (options.option->short_option)
      {
        case TEXT('i'):
        {
          char_t * test;
          pid = _tcstoul(options.argument, &test, 0);
          if (pid == 0 || *test != 0)
            throw option_error(string(TEXT("Invalid argument '")) + options.argument + TEXT("' for option --pid"));
          return true;
        }
        case TEXT('n'):
          name = options.argument;
          return true;
        case TEXT('s'):
          exact_match = false;
          return true;
        default:
          return false;
      }
    }

    void validate_options(const bool allow_all) const
    {
      if (pid == 0 && name.empty() && !allow_all)
        throw option_error(TEXT("Neither process id nor process name specified"));
      if (pid != 0 && !name.empty())
        throw option_error(TEXT("Both process id and process name specified"));
    }

    std::map<DWORD, string> select_processes() const
    {
      if (name.empty() && pid == 0)
        return find_process();
      else if (!name.empty())
        return find_process(name, exact_match);
      else
        return find_process(pid);
    }

    void add_target_to_message(string & msg) const
    {
      if (pid != 0)
      {
        const unsigned i = msg.size();
        msg.resize(i + 1 + sizeof(pid));
        msg[i] = TEXT('i');
        CopyMemory(&msg[0] + i + 1, &pid, sizeof(pid));
      }
      else if (!name.empty())
      {
        const unsigned i = msg.size();
        msg.resize(i + 2 + name.length());
        if (exact_match)
          msg[i] = TEXT('n');
        else
          msg[i] = TEXT('s');
        msg[i + 1] = name.length();
        CopyMemory(&msg[0] + i + 2, &name[0], name.length());
      }
    }

    void get_target_from_message(unsigned & i, const boost::scoped_array<char> & msg, const DWORD msg_size)
    {
      if (msg_size <= i)
        return;

      switch (msg[i])
      {
        case TEXT('i'):
        {
          if (msg_size < i + 1 + sizeof(pid))
            throw error(TEXT("Invalid message received: incomplete pid target"));
          CopyMemory(&pid, msg.get() + i + 1, sizeof(pid));
          i += 1 + sizeof(pid);
          break;
        }
        case TEXT('s'):
          exact_match = false;
          // (fallthrough)
        case TEXT('n'):
        {
          if (msg_size < i + 2)
            throw error(TEXT("Invalid message received: incomplete name target length"));
          const unsigned length = msg[i + 1];
          if (msg_size < i + 2 + length)
            throw error(TEXT("Invalid message received: incomplete name target"));
          name = string(msg.get() + i + 2, msg.get() + i + 2 + length);
          i += 2 + length;
          break;
        }
      }
    }

    void validate_process_list(bool empty) const
    {
      if (!empty)
        return;
      if (pid != 0)
        throw error(TEXT("Could not find process ") + to_string(pid));
      else
        throw error(TEXT("Could not find process ") + name);
    }
};

}

#endif
