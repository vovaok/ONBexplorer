#pragma once

#include <QTcpSocket>
#include <cassert>

namespace API
{

static constexpr size_t SampleBufferSize = 16ull * 1024 * 1024;

int readAll(QTcpSocket* psock, char* buf, size_t sz);
int peekAll(QTcpSocket* psock, char* buf, size_t sz);
int writeAll(QTcpSocket* psock, const char* buf, size_t sz);

enum MessageT: char
{
  // <-- | sz | op | iparam | dev + \0 | param + \0 |
  // --> | sz | op | value + \0 |
  GetParam = 'r',
  
  // <-- | sz | op | iparam | ivalue | dev + \0 | param + \0 | value + \0 |
  // --> | sz | op |
  SetParam = 'w',
  
  // <-- | sz | op |
  // --> | sz | op | [dev\0]* | \0 |
  ListDevices = 'd',

  // <-- | sz | op | dev + \0 |
  // --> | sz | op | [param\0]* | \0 |
  ListParams = 'p',

  // <-- | sz | op | iparam | ivalue | dev + \0 | param + \0 | value + \0 |
  // --> | sz | op | value + \0 |
  Invoke = 'i',

  // <-- | sz | op | devlen | dev + \0 | int | [param + \0]* | \0 |
  // --> | sz | op |
  SetAGRequest = 'a',

  // <-- | sz | op |
  // --> | sz | op | nsamples |
  // <-- | sz | op | sample | (nsamples times)
  GetSamples = 's',
  
  // <-- | sz | op | msg + \0 |
  Error = 'e'


};

struct AnalyzerSample
{
  float freq;
  float magn;
  float phase;
  float rms;
} __attribute__((packed));

}
