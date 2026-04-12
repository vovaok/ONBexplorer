#include "apicommon.h"
#include <iostream>

int API::readAll(QTcpSocket* psock, char* buf, size_t sz)
{
  size_t nread = 0;

  while(nread < sz)
  {
    // std::cout << "waiting for read..." << std::endl;
    
    psock->waitForReadyRead(1);
    int ret = psock->read(buf + nread, sz - nread);
    nread += ret;

    // std::cout << "read " << ret << std::endl;
    if (ret < 0) return ret;
  }

  return 0;
}

int API::peekAll(QTcpSocket* psock, char* buf, size_t sz)
{
  size_t nread = 0;

  while(nread < sz)
  {
    // std::cout << "waiting for read..." << std::endl;
    
    psock->waitForReadyRead(1);
    int ret = psock->peek(buf + nread, sz - nread);
    nread += ret;

    // std::cout << "peek " << ret << std::endl;
    if (ret < 0) return ret;
  }

  return 0;
}

int API::writeAll(QTcpSocket* psock, const char* buf, size_t sz)
{
  size_t nwrite = 0;

  while(nwrite < sz)
  {
    int ret = psock->write(buf + nwrite, sz - nwrite);
    psock->waitForBytesWritten(1);
    // std::cout << "written " << ret << std::endl;
    nwrite += ret;
    
    if (ret < 0) return ret;
  }

  psock->flush();
  // std::cout << "flushed" << std::endl;

  return 0;
}
