// Copyright 2005, Stephen Cleary
// See the accompanying file "ntutils.chm" for licence information

#ifndef NTUTILS_MESSAGE_H
#define NTUTILS_MESSAGE_H

#include "ntutils/basic.h"

namespace ntutils {

template <typename T>
static inline void encode_binary_data(string & buf, const T data)
{
  buf.resize(buf.size() + sizeof(T));
  CopyMemory(&buf[0] + buf.size() - sizeof(T), &data, sizeof(T));
}

template <typename T>
static inline T decode_binary_data(unsigned & i, const string & buf, const_str name)
{
  if (buf.size() < i + sizeof(T))
    throw error(TEXT("Invalid message received: incomplete ") + name.as_string());
  T ret;
  CopyMemory(&ret, &buf[0] + i, sizeof(T));
  i += sizeof(T);
  return ret;
}

static inline void encode_string(string & buf, const_str data)
{
  buf.reserve(buf.size() + sizeof(unsigned) + data.length());
  encode_binary_data<unsigned>(buf, data.length());
  buf += data.ptr();
}

static inline string decode_string(unsigned & i, const string & buf, const_str name)
{
  const unsigned length = decode_binary_data<unsigned>(i, buf, name);
  if (buf.size() < i + length)
    throw error(TEXT("Invalid message received: incomplete ") + name.as_string());
  i += length;
  return string(&buf[0] + i - length, &buf[0] + i);
}

}

#endif
