// Copyright 2005, Stephen Cleary
// See the accompanying file "readme.html" for licence information

#ifndef NTUTILS_TOOLHELP_H
#define NTUTILS_TOOLHELP_H

#include <boost/iterator/iterator_facade.hpp>

#include "ntutils/tlhelp32_dll.h"

namespace ntutils {

class tool_help_process_iterator: public boost::iterator_facade<tool_help_process_iterator,
    const PROCESSENTRY32, boost::single_pass_traversal_tag>
{
  private:
    HANDLE snapshot;
    PROCESSENTRY32 data;

    friend class boost::iterator_core_access;

    void increment()
    {
      if (!PortableProcess32Next(snapshot, &data))
        if (GetLastError() == ERROR_NO_MORE_FILES)
          snapshot = 0;
        else
          throw Win32_error(TEXT("Process32Next"));
    }

    bool equal(const tool_help_process_iterator & other) const
    { return (snapshot == other.snapshot); }

    const PROCESSENTRY32 & dereference() const { return data; }

  public:
    tool_help_process_iterator():snapshot(0) { }

    explicit tool_help_process_iterator(const HANDLE nsnapshot)
    :snapshot(nsnapshot)
    {
      data.dwSize = sizeof(data);
      if (!PortableProcess32First(snapshot, &data))
        if (GetLastError() == ERROR_NO_MORE_FILES)
          snapshot = 0;
        else
          throw Win32_error(TEXT("Process32First"));
    }
};

class tool_help_thread_iterator: public boost::iterator_facade<tool_help_thread_iterator,
    const THREADENTRY32, boost::single_pass_traversal_tag>
{
  private:
    HANDLE snapshot;
    THREADENTRY32 data;

    friend class boost::iterator_core_access;

    void increment()
    {
      if (!PortableThread32Next(snapshot, &data))
        if (GetLastError() == ERROR_NO_MORE_FILES)
          snapshot = 0;
        else
          throw Win32_error(TEXT("Thread32Next"));
    }

    bool equal(const tool_help_thread_iterator & other) const
    { return (snapshot == other.snapshot); }

    const THREADENTRY32 & dereference() const { return data; }

  public:
    tool_help_thread_iterator():snapshot(0) { }

    explicit tool_help_thread_iterator(const HANDLE nsnapshot)
    :snapshot(nsnapshot)
    {
      data.dwSize = sizeof(data);
      if (!PortableThread32First(snapshot, &data))
        if (GetLastError() == ERROR_NO_MORE_FILES)
          snapshot = 0;
        else
          throw Win32_error(TEXT("Thread32First"));
    }
};

template <typename Owned = unowned>
struct tool_help_snapshot: null_handle_base<tool_help_snapshot<Owned> >
{
  typedef null_handle_base<tool_help_snapshot<Owned> > base_type;
  TBA_DEFINE_HANDLE_CLASS(tool_help_snapshot, HANDLE)

  BOOL Close() { return PortableCloseToolhelp32Snapshot(this->Handle()); }
  bool close() { if (!Close()) throw Win32_error(TEXT("CloseHandle")); }

  tool_help_process_iterator processes_begin() const { return tool_help_process_iterator(this->Handle()); }
  tool_help_process_iterator processes_end() const { return tool_help_process_iterator(); }

  tool_help_thread_iterator threads_begin() const { return tool_help_thread_iterator(this->Handle()); }
  tool_help_thread_iterator threads_end() const { return tool_help_thread_iterator(); }

  void Create(const DWORD flags, const DWORD process_id = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    this->Reset(PortableCreateToolhelp32Snapshot(flags, process_id));
  }
  void create(const DWORD flags, const DWORD process_id = 0)
  {
    BOOST_STATIC_ASSERT(Owned::value);
    const HANDLE nhandle = PortableCreateToolhelp32Snapshot(flags, process_id);
    if (nhandle == 0)
      throw Win32_error(TEXT("CreateToolhelp32Snapshot"));
    this->Reset(nhandle);
  }
};

}

#endif
