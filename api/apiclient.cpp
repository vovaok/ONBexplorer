#include "apiclient.h"
#include "apicommon.h"
#include "objectinfo.h"

#include <QVariant>
#include <QTcpSocket>

#include <iostream>
#include <sstream>

#define PYASSERT(expr) pyassert((expr), #expr, __FILE__, __LINE__)

using namespace Objnet;
using namespace std::string_literals;

namespace API
{

using namespace Objnet;

struct Client::Impl
{
  QTcpSocket socket_;
};

void Client::setPacketSize(uint16_t sz)
{
  *reinterpret_cast<uint16_t*>(buf_.data()) = sz;
}

uint16_t Client::getPacketSize()
{
  return *reinterpret_cast<uint16_t*>(buf_.data());
}

uint16_t Client::readPacket()
{
  peekAll(&pimpl_->socket_, buf_.data(), 2);
  uint16_t sz = getPacketSize();

  readAll(&pimpl_->socket_, buf_.data(), sz);

  if (buf_[2] == Error)
    throw std::runtime_error(buf_.data() + 3);

  return sz;
}

Client::Client(const std::string& addr, size_t port): pimpl_{new Impl()}, buf_(SampleBufferSize, 0)
{
  pimpl_->socket_.connectToHost(addr.c_str(), port);
  pimpl_->socket_.waitForConnected(3000);
}

std::string Client::getParamStr(const std::string& dev, const std::string& param)
{
  size_t devLen = dev.size() + 1;
  size_t paramLen = param.size() + 1;

  size_t sz = 2 + 1 + 1 + devLen + paramLen;
  setPacketSize(sz);

  buf_[2] = MessageT::GetParam;

  size_t iparam = 4 + devLen;
  buf_[3] = iparam;
  
  memcpy(buf_.data() + 4, dev.c_str(), devLen);
  memcpy(buf_.data() + iparam, param.c_str(), paramLen);

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  sz = readPacket();
  PYASSERT(buf_[2] == MessageT::GetParam);

  return buf_.data() + 3;
}

void Client::setParamStr(const std::string& dev, const std::string& param, const std::string& valueStr)
{
  size_t devLen   = dev.size() + 1;
  size_t paramLen = param.size() + 1;

  uint8_t iparam = 5 + devLen;
  uint8_t ivalue = iparam + paramLen;
  
  uint8_t valueLen = valueStr.size() + 1;

  size_t sz = 2 + 1 + 1 + 1 + devLen + paramLen + valueLen;
  setPacketSize(sz);

  buf_[2] = MessageT::SetParam;
  buf_[3] = iparam;
  buf_[4] = ivalue;
  
  memcpy(buf_.data() + 5, dev.c_str(), devLen);
  memcpy(buf_.data() + iparam, param.c_str(), paramLen);
  memcpy(buf_.data() + ivalue, valueStr.c_str(), valueLen);

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  sz = readPacket();
  PYASSERT(buf_[2] == MessageT::SetParam);
}

std::string Client::invokeStr(const std::string& dev, const std::string& param, const std::string& valueStr)
{
  size_t devLen   = dev.size() + 1;
  size_t paramLen = param.size() + 1;

  uint8_t iparam = 5 + devLen;
  uint8_t ivalue = iparam + paramLen;
  
  uint8_t valueLen = valueStr.size() + 1;

  size_t sz = 2 + 1 + 1 + 1 + devLen + paramLen + valueLen;
  setPacketSize(sz);

  buf_[2] = MessageT::Invoke;
  buf_[3] = iparam;
  buf_[4] = ivalue;
  
  memcpy(buf_.data() + 5, dev.c_str(), devLen);
  memcpy(buf_.data() + iparam, param.c_str(), paramLen);
  memcpy(buf_.data() + ivalue, valueStr.c_str(), valueLen);

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  sz = readPacket();
  PYASSERT(buf_[2] == MessageT::Invoke);

  return buf_.data() + 3;
}

template<typename T>
T Client::getParam(const std::string& dev, const std::string& param)
{
  std::string valueStr = getParamStr(dev, param);
  
  T value;
  ObjectInfo object { "tmp", value };
  PYASSERT(object.fromString(valueStr.c_str()));

  return value;
}

template<typename T>
void Client::setParam(const std::string& dev, const std::string& param, const T& value)
{
  T copy = value;
  ObjectInfo object {"tmp", copy};

  std::string valueStr = object.toString().toStdString();
  
  setParamStr(dev, param, valueStr);
}

std::vector<std::string> Client::listDevices()
{
  setPacketSize(3);
  buf_[2] = MessageT::ListDevices;
  
  writeAll(&pimpl_->socket_, buf_.data(), 3);

  readPacket();
  PYASSERT(buf_[2] == MessageT::ListDevices);

  std::vector<std::string> ret;
  const char* pcurr = buf_.data() + 3;

  while(*pcurr != '\0')
  {
    ret.push_back(pcurr);
    pcurr += strlen(pcurr) + 1;
  }

  return ret;
}

std::vector<std::string> Client::listParams(const std::string& dev)
{
  size_t devLen = dev.size() + 1;
  size_t sz = 3 + devLen;
  setPacketSize(sz);

  buf_[2] = MessageT::ListParams;

  memcpy(buf_.data() + 3, dev.c_str(), devLen);

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  sz = readPacket();
  PYASSERT(buf_[2] == MessageT::ListParams);

  std::vector<std::string> ret;
  const char* pcurr = buf_.data() + 3;

  while(*pcurr != '\0')
  {
    ret.push_back(pcurr);
    pcurr += strlen(pcurr) + 1;
  }
  
  return ret;
}

void Client::fetchDevice(const std::string& dev)
{
  size_t devLen = dev.size() + 1;
  size_t sz = 3 + devLen;
  setPacketSize(sz);

  buf_[2] = MessageT::FetchDevice;

  memcpy(buf_.data() + 3, dev.c_str(), devLen);

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  readPacket();
  PYASSERT(buf_[2] == MessageT::FetchDevice);
}

void Client::setAGRequest(const std::string& dev, uint16_t interval, const std::vector<std::string>& params)
{
  uint16_t sz = 4 + dev.size() + 1 + 3;

  for(const auto& param: params)
    sz += param.size() + 1;

  setPacketSize(sz);
  buf_[2] = SetAGRequest;

  buf_[3] = dev.size() + 1;
  memcpy(buf_.data() + 4, dev.c_str(), dev.size() + 1);

  *reinterpret_cast<uint16_t*>(buf_.data() + 4 + dev.size() + 1)
    = interval;

  char* pdata = buf_.data() + 4 + dev.size() + 1 + 2;

  for(const auto& param: params)
  {
    memcpy(pdata, param.c_str(), param.size() + 1);
    pdata += param.size() + 1;
  }

  *pdata = 0;

  writeAll(&pimpl_->socket_, buf_.data(), sz);

  sz = readPacket();
  PYASSERT(buf_[2] = SetAGRequest);
}

auto Client::getSamples() -> Samples
{
  setPacketSize(3);
  buf_[2] = GetSamples;
  writeAll(&pimpl_->socket_, buf_.data(), 3);

  size_t sz = readPacket();
  PYASSERT(buf_[2] == GetSamples);
  PYASSERT(sz == 3 + 2*sizeof(size_t));

  size_t nSamples = *reinterpret_cast<size_t*>(buf_.data() + 3);
  size_t totalSize = *reinterpret_cast<size_t*>(buf_.data() + 3 + sizeof(size_t));
  
  readAll(&pimpl_->socket_, buf_.data(), totalSize);
  Samples samples;

  size_t k = 0;
  for (size_t i = 0; i < nSamples; ++i)
  {
    sz = *reinterpret_cast<uint16_t*>(&buf_[k]);
    PYASSERT(buf_[k + 2] == GetSamples);
    samples.emplace_back(buf_.data() + k + 3, buf_.data() + k + sz);
    k += sz;
  }

  return samples;
}

void Client::pyassert(bool cond, const char* msg, const char* file, size_t line)
{
  if(cond) return;
  std::stringstream ss;

  ss << "Assertion failed (" << file << ", " << line << "): " << msg;
  throw std::runtime_error(ss.str());
}

Client::~Client() = default;
Client::Client(Client&&) = default;
Client& Client::operator=(Client&&) = default;

#define INSTANTIATE(type)                                                                    \
  template void Client::setParam<type>(const std::string&, const std::string&, const type&); \
  template type Client::getParam<type>(const std::string&, const std::string&);

INSTANTIATE(int)
INSTANTIATE(bool)
INSTANTIATE(float)
INSTANTIATE(double)

}
