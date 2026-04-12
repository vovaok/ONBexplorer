#pragma once

#include "objnetdevice.h"
#include <memory>
#include <thread>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <sstream>

#include "apicommon.h"
#include <pthread.h>

using namespace Objnet;

namespace API
{

class Server final: public QObject
{
  Q_OBJECT

public:
  struct ILogger
  {
    virtual void log (const std::string& msg) = 0;
    virtual ~ILogger() = default;
  };

private:
  struct ThreadDeleter
  {
    void operator()(std::thread* pthread);
  };

  using ThreadPtr = std::unique_ptr<std::thread, ThreadDeleter>;
  using Lock = std::unique_lock<std::mutex>;

private:
  std::unordered_map<std::string, ObjnetDevice*> devmap_; 
  
  uint16_t port_;
  std::unique_ptr<ILogger> logger_;

  std::mutex mutex_;

  bool receivedFlag_;
  std::condition_variable receivedCond_;
  QString requestedParam_;
  
  ThreadPtr thread_;

  std::vector<QVariantMap> samples_;
  std::mutex analMutex_;

  bool _the_endec = false;

public:
  Server(std::unique_ptr<ILogger>&& logger, uint16_t port=4242);
  virtual ~Server() = default;

  void start();
  void stop();

  void addDevice(ObjnetDevice* dev);

private:
  void onObjectReceived(QString name, QVariant);
  void onSampleReceived(QVariantMap values);

  ObjnetDevice* getDevice(const char* devname);
  ObjectInfo* getObject(ObjnetDevice* dev, const char* param);

  void sendError(QTcpSocket* sock, const char* msg);
  std::string buf2str(const char* buf, size_t sz);

  void task();
};
}
