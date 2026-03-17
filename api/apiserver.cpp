#include "apiserver.h"

using namespace Objnet;

namespace API
{

// dev->autoGropu
// return QVarMap

void Server::ThreadDeleter::operator()(std::thread* pthread)
{
  pthread_cancel(pthread->native_handle());
  pthread->join();
  delete pthread;
}

Server::Server(std::unique_ptr<ILogger>&& logger, uint16_t port):
  port_{port}, logger_{std::move(logger)} {}

void Server::start()
{
  _the_endec = false;
  thread_ = ThreadPtr{ new std::thread(&Server::task, this) };
}

void Server::stop()
{
  _the_endec = true;
  /// @todo proper thread cancel
  pthread_cancel(thread_->native_handle());
  thread_ = nullptr;
}

void Server::addDevice(ObjnetDevice* dev)
{
  assert(dev);
  Lock _(mutex_);

  devmap_[dev->name().toStdString()] = dev;

  connect(dev, &ObjnetDevice::objectReceived,
          this, &Server::onObjectReceived);

  connect(dev,  &ObjnetDevice::objectGroupReceived, 
          this, &Server::onSampleReceived);

  //dev->requestMetaInfo();
  //dev->requestObjects();
}

void Server::onObjectReceived(QString name, QVariant)
{
  Lock _ (mutex_);
  if (name == requestedParam_)
  {
    receivedFlag_ = true;
    receivedCond_.notify_all();
  }
}

void Server::onSampleReceived(QVariantMap values)
{
  Lock _ (analMutex_);
  samples_.push_back(values);
}

ObjnetDevice* Server::getDevice(const char* devname)
{
  Lock _(mutex_);

  auto mapIt = devmap_.find(devname);
  if (mapIt == devmap_.end())
    return nullptr;

  return mapIt->second;
}

ObjectInfo* Server::getObject(ObjnetDevice* dev, const char* param)
{
  if (!dev->hasObject(param))
    return nullptr;

  return dev->objectInfo(param);
}

void Server::sendError(QTcpSocket* sock, const char* msg)
{
  // | sz | op | msg + \0 |
  uint8_t len = strlen(msg);
  uint16_t sz = 2 + 1 + len + 1;
  char op = MessageT::Error;

  writeAll(sock, reinterpret_cast<char*>(&sz), 2);
  writeAll(sock, &op, 1);
  writeAll(sock, msg, len + 1);

  logger_->log(std::string("Error: ") + msg);
}

std::string Server::buf2str(const char* buf, size_t sz)
{
  std::stringstream ss {};

  for (size_t i = 0; i < sz; ++i)
    ss << +buf[i] << "(" << buf[i] << ")" << " ";

  return ss.str();
}

// May God help those who saw
// this abomination of a function
// My sincere apologies to you
void Server::task()
{
  logger_->log("Server started");

  QTcpServer server_;
  server_.setMaxPendingConnections(1);
  server_.listen(QHostAddress::Any, port_);

  while(!_the_endec)
  {
    bool timedOut;
    server_.waitForNewConnection(1000, &timedOut);
    if (timedOut)
        continue;

    QTcpSocket* psock = server_.nextPendingConnection();
    
    logger_->log("Client connected");

    std::array<char, 1024> buf;

    logger_->log("Starting handler loop");

    while(psock->isOpen() && !_the_endec)
    {
      if(peekAll(psock, buf.data(), 2) < 0) break;
      uint16_t in_sz = *reinterpret_cast<uint16_t*>(buf.data());

      logger_->log("Reading size " + std::to_string(in_sz));

      if(readAll(psock, buf.data(), in_sz) < 0) break;

      if (in_sz < 2)
      {
        sendError(psock, "Parse error");
        continue;
      }

      char optype = buf[2];

      logger_->log(std::string("<-- ") + buf2str(buf.data(), in_sz));
      
      switch (optype)
      {
      // <-- | sz | op | iparam | dev + \0 | param + \0 |
      // --> | sz | op | value + \0 |
      case GetParam:
      {
        if (in_sz < 2 + 1 + 1 + 2 + 2)
        {
          sendError(psock, "Parse error");
          break;
        }

        const char* devname = buf.data() + 4;
        const char* param   = buf.data() + buf[3];

        ObjnetDevice* dev = getDevice(devname);
        if (!dev) { sendError(psock, "No device"); break; }

        ObjectInfo* object = getObject(dev, param);
        if (!object) { sendError(psock, "No object"); break; }
        
        std::string value = "";

        if (!object->isInvokable() && object->isWritable())
        {
          Lock lock (mutex_);

          receivedFlag_ = false;
          requestedParam_ = param;
          
          while(!receivedFlag_)
          {
            dev->requestObject(param);
            receivedCond_.wait_for(lock, std::chrono::milliseconds(100));
          }
          value = object->toString().toStdString();
        }
        else
          logger_->log(std::string("Warning: can't read ")
            + devname + "/" + param);

        size_t sz = 3 + value.size() + 1;

        *reinterpret_cast<uint16_t*>(buf.data()) = sz;
        buf[2] = GetParam;
        memcpy(buf.data() + 3, value.c_str(), value.size() + 1);

        writeAll(psock, buf.data(), sz);
        break;
      }

      // <-- | sz | op | iparam | ivalue | dev + \0 | param + \0 | value + \0 |
      // --> | sz | op |
      case SetParam:
      {
        if (in_sz < 2 + 1 + 1 + 1 + 2 + 2 + 1)
        {
          sendError(psock, "Parse error");
          break;
        }
        const char* devname = buf.data() + 5;
        const char* param   = buf.data() + buf[3];
        const char* value   = buf.data() + buf[4];

        ObjnetDevice* dev = getDevice(devname);
        if (!dev) { sendError(psock, "No device"); break; }

        ObjectInfo* object = getObject(dev, param);
        if (!object) { sendError(psock, "No object"); break; }
        
        if (!object->isWritable())
        {
          sendError(psock, "Not writable");
          break;
        }

        if (object->wType() == ObjectInfo::Type::Common)
        {
          logger_->log(std::string("value: ") + value);
          auto arr = QByteArray::fromHex(value);
          QVariant v (arr);
          object->fromVariant(v);
        }
        else
          object->fromString(value);
        
        dev->sendObject(param);

        *reinterpret_cast<uint16_t*>(buf.data()) = 3;
        buf[2] = SetParam;

        writeAll(psock, buf.data(), 3);
        break;
      }

      // <-- | sz | op | iparam | ivalue | dev + \0 | param + \0 | value + \0 |
      // --> | sz | op | value + \0 |
      case Invoke:
      {
        if (in_sz < 2 + 1 + 1 + 1 + 2 + 2)
        {
          sendError(psock, "Parse error");
          break;
        }

        const char* devname = buf.data() + 5;
        const char* param   = buf.data() + buf[3];
        const char* value   = buf.data() + buf[4];

        ObjnetDevice* dev = getDevice(devname);
        if (!dev) { sendError(psock, "No device"); break; }

        ObjectInfo* object = getObject(dev, param);
        if (!object) { sendError(psock, "No object"); break; }
        
        if (!object->isInvokable())
        {
          break;
        }

        std::string valueStr = "";


        if (!object->isWritable())
        {
          if (strlen(value))
            object->fromString(value);
          dev->sendObject(param);
        }
        else
        {
          Lock lock (mutex_);

          receivedFlag_ = false;
          requestedParam_ = param;

          dev->requestObject(param);
          
          while(!receivedFlag_)
            receivedCond_.wait(lock);

          valueStr = object->toString().toStdString();
        }

        size_t sz = 3 + valueStr.size() + 1;

        *reinterpret_cast<uint16_t*>(buf.data()) = sz;
        buf[2] = Invoke;
        memcpy(buf.data() + 3, valueStr.c_str(), valueStr.size() + 1);

        writeAll(psock, buf.data(), sz);

        break;
      }

      // <-- | sz | op |
      // --> | sz | op | [dev\0]* |
      case ListDevices:
      {
        Lock _ (mutex_);
        char* pdata = buf.data() + 3;
        size_t offset = 0;

        for (auto elt: devmap_)
        {
          const std::string& name = elt.first;
          memcpy(pdata + offset, name.c_str(), name.size() + 1);
          offset += name.size() + 1;
        }
        size_t sz = 3 + offset + 1;

        *reinterpret_cast<uint16_t*>(buf.data()) = sz;
        buf[2] = ListDevices;
        buf[sz - 1] = 0;

        writeAll(psock, buf.data(), sz);
        break;
      }
      
      // <-- | sz | op | dev + \0 |
      // --> | sz | op | [param\0]* |
      case ListParams:
      {
        if (in_sz < 2 + 1 + 2)
        {
          sendError(psock, "Parse error");
          break;
        }

        ObjnetDevice* dev = getDevice(buf.data() + 3);
        if (!dev) { sendError(psock, "No device"); break; }

        char* pdata = buf.data() + 3;
        size_t offset = 0;

        for (int i = 0; i < dev->objectCount(); ++i)
        {
          ObjectInfo* obj = dev->objectInfo(i);
          if (!obj) continue;

          auto param_q = obj->name();
          std::string param = param_q.toStdString();
          memcpy(pdata + offset, param.c_str(), param.size() + 1);

          offset += param.size() + 1;
        }

        size_t sz = 3 + offset + 1;

        *reinterpret_cast<uint16_t*>(buf.data()) = sz;
        buf[2] = ListParams;
        buf[sz - 1] = 0;

        writeAll(psock, buf.data(), sz);
        break;
      }

      // <-- | sz | op | devlen | dev + \0 | int | [param + \0]* | \0 |
      // --> | sz | op |
      case SetAGRequest:
      {
        
        ObjnetDevice* dev = getDevice(buf.data() + 4);
        if (!dev) { sendError(psock, "No device"); break; }

        uint8_t devlen = *reinterpret_cast<uint8_t*>(buf.data() + 3);

        uint16_t interval =
          *reinterpret_cast<uint16_t*>(buf.data() + 4 + devlen);

        std::vector<QString> params;
        char* pdata = buf.data() + 4 + devlen + 2;

        if (*pdata == 0)
        {
          logger_->log("Disabling AGR");
          dev->clearAutoGroupRequest();
        }
        else 
        {
          bool found = true;

          for(; found && *pdata != 0; pdata += strlen(pdata) + 1)
          {
            ObjectInfo* info = getObject(dev, pdata);
            if (!info) found = false;
  
            params.emplace_back(pdata);
          }

          if (!found) { sendError(psock, "No object"); break; }

          for (const auto& param: params)
            logger_->log(param.toStdString());

          dev->autoGroupRequest(interval, params);

          logger_->log("here");
        }

        *reinterpret_cast<uint16_t*>(buf.data()) = 3;
        buf[2] = SetAGRequest;

        writeAll(psock, buf.data(), 3);
        break;
      }
      
      // <-- | sz | op |
      // --> | sz | op | nsamples |
      // <-- | sz | op | sample | (nsamples times)
      case GetSamples:
      {
        analMutex_.lock();
        size_t nSamples = samples_.size();
        analMutex_.unlock();

        logger_->log(std::string("samples ") + std::to_string(nSamples));
        
        if (nSamples >= 1 << 16)
        {
          sendError(psock, "Too much samples :(");
          Lock _ (analMutex_);
          samples_.clear();
        }

        *reinterpret_cast<uint16_t*>(buf.data()) = 5;
        buf[2] = GetSamples;
        *reinterpret_cast<uint16_t*>(buf.data() + 3) = nSamples;

        writeAll(psock, buf.data(), 5);

        for (size_t i = 0; i < nSamples; ++i)
        {
          QByteArray arr;
          QDataStream ds (&arr, QIODevice::WriteOnly);
          QVariant v {samples_[i]};

          ds << v;

          uint16_t sz = arr.size() + 3;
          *reinterpret_cast<uint16_t*>(buf.data()) = sz;
          buf[2] = GetSamples;

          memcpy(buf.data() + 3, arr.data(), arr.size());

          writeAll(psock, buf.data(), sz);
        }

        Lock _ (analMutex_);
        samples_.clear();
        break;
      }

      default:
        sendError(psock, "Unknown operation");
        break;
      }
      uint16_t msgSz = *reinterpret_cast<uint16_t*>(buf.data());
      logger_->log(std::string(" --> ") + buf2str(buf.data(), msgSz));
    }
  }
}

}
