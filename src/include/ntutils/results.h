// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_RESULTS_H
#define NTUTILS_RESULTS_H

#include <vector>

#include "ntutils/basic.h"
#include "ntutils/console.h"
#include "ntutils/message.h"

namespace ntutils {

struct program_results
{
  // Whether or not we are generating XML output
  bool xml;

  // Whether or not an error has been seen
  bool error_seen;

  // Buffer for the output (xml or normal)
  string buffer;

  // Stack of error/warning/result contexts (only used for normal output)
  std::vector<string> context;

  program_results()
  :xml(false), error_seen(false) { }

  bool handle_option(const option_parser & options)
  {
    if (options.option->short_option != TEXT('x'))
      return false;

    xml = true;
    return true;
  }

  string get_context_string() const
  {
    string ret;
    for (std::vector<string>::const_iterator i = context.begin(); i != context.end(); ++i)
      ret += *i + TEXT(": ");
    return ret;
  }

  void report_error(const error & e)
  {
    error_seen = true;

    if (xml)
      buffer += e.xml();
    else
      buffer += get_context_string() + e.twhat() + TEXT('\n');
  }

  void report_warning(const string & msg)
  {
    if (xml)
      buffer += TEXT("<warning message=") + make_xml_attribute_value(msg) + TEXT(" />");
    else
      buffer += get_context_string() + TEXT("Warning: ") + msg + TEXT('\n');
  }

  void report_warning(const error & e)
  {
    if (xml)
      buffer += TEXT("<warning>") + e.xml() + TEXT("</warning>");
    else
      buffer += get_context_string() + TEXT("Warning: ") + e.twhat() + TEXT('\n');
  }

  void report_result(const string & msg, const string & attributes = string())
  {
    if (xml)
    {
      if (attributes.empty())
        buffer += TEXT("<result value=" + make_xml_attribute_value(msg) + " />");
      else
        buffer += TEXT("<result ") + attributes + TEXT(" />");
    }
    else
      buffer += get_context_string() + msg + TEXT('\n');
  }

  void report_info(const string & attributes)
  {
    if (xml)
      buffer += TEXT("<info ") + attributes + TEXT(" />");
  }

  // Called at the end of the program
  int return_code()
  {
    // Return value is based on whether or not an error was seen
    return (error_seen ? -1 : 0);
  }

  void register_context(const string & msg, const string & attributes)
  {
    if (xml)
      buffer += TEXT("<context ") + attributes + TEXT(">");
    else
      context.push_back(msg);
  }

  void unregister_context()
  {
    if (xml)
      buffer += TEXT("</context>");
    else
      context.pop_back();
  }

  void encode_message(string & msg) const
  {
    if (xml)
      msg += TEXT('x');
    else
      msg += TEXT('n');
    encode_binary_data<unsigned>(msg, context.size());
    for (std::vector<string>::const_iterator i = context.begin(); i != context.end(); ++i)
      encode_string(msg, *i);
  }

  void decode_message(unsigned & i, const string & msg)
  {
    if (msg.size() == i)
      throw error(TEXT("Invalid message recieved: no output format"));

    switch (msg[i++])
    {
      case TEXT('x'): xml = true; break;
      case TEXT('n'): break;
      default: throw error(TEXT("Invalid message received: unknown output format"));
    }

    const unsigned context_length = decode_binary_data<unsigned>(i, msg, TEXT("context"));
    for (unsigned j = 0; j != context_length; ++j)
      context.push_back(decode_string(i, msg, TEXT("context")));
  }

  void encode_response(string & msg) const
  {
    if (error_seen)
      msg += TEXT('1');
    else
      msg += TEXT('0');
    encode_string(msg, buffer);
  }

  void decode_response(unsigned & i, const string & msg)
  {
    try
    {
      if (msg.size() == i)
        throw error(TEXT("Invalid message recieved: no result"));

      switch (msg[i++])
      {
        case TEXT('0'): break;
        case TEXT('1'): error_seen = true; break;
        default: throw error(TEXT("Invalid message received: unknown result"));
      }

      buffer += decode_string(i, msg, TEXT("result"));
    }
    catch (const error & e)
    {
      report_error(e);
    }
  }
};

}

extern ntutils::program_results results;

namespace ntutils {

struct result_context
{
  result_context(const string & msg, const string & attributes)
  { results.register_context(msg, attributes); }

  ~result_context() { results.unregister_context(); }
};

struct computer_context: result_context
{
  explicit computer_context(const string & computer)
  :result_context(computer, TEXT("computer=") + make_xml_attribute_value(computer)) { }
};

struct process_context: result_context
{
  process_context(const string & name, const DWORD id)
  :result_context(name + TEXT(" (") + to_string(id) + TEXT(")"),
       TEXT("process_name=") + make_xml_attribute_value(name) + TEXT(" process_id='") + to_string(id) + TEXT('\'')) { }
};

}

#endif
