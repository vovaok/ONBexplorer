#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace API
{

struct Client final
{
public:
  using Samples = std::vector<std::vector<char>>;
private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;

  std::vector<char> buf_;

private:
  void     setPacketSize(uint16_t sz);
  uint16_t getPacketSize();
  uint16_t readPacket();

  // Seems like regular assert message
  // is not forwarded to python
  // This thing just throws an exception
  void pyassert(bool cond, const char* msg, const char* file, size_t line);
public:
  Client(const std::string& addr = "127.0.0.1", size_t port=4242);
  
  ~Client();
  Client(Client&&);
  Client& operator=(Client&&);

  std::string getParamStr(const std::string& dev, const std::string& param);
  void setParamStr(const std::string& dev, const std::string& param, const std::string& value);
  std::string invokeStr(const std::string& dev, const std::string& param, const std::string& value="");

  template<typename T>
  T getParam(const std::string& dev, const std::string& param);

  void fetchDevice(const std::string& dev);

  template<typename T>
  void setParam(const std::string& dev, const std::string& param, const T& value);

  std::vector<std::string> listDevices();
  std::vector<std::string> listParams(const std::string& dev);

  void setAGRequest(const std::string& dev, uint16_t interval, const std::vector<std::string>& params);

  Samples getSamples();
};

}
