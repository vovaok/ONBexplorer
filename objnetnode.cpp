#include "objnetNode.h"

using namespace Objnet;

ObjnetNode::ObjnetNode(ObjnetInterface *iface) :
    ObjnetCommonNode(iface),
    mNetState(netnStart),
    mNetTimeout(0),
    mClass(0x00000000),
    mName("<node>"),
    mFullName("<Generic objnet node>")
{
    #ifdef __ICCARM__
    mTimer.setTimeoutEvent(EVENT(&ObjnetMaster::onTimer));
    #else
    QObject::connect(&mTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
    #endif
    mTimer.start(200);

    ObjectInfo obj;
    obj.bindVariableRO("class", &mClass, sizeof(mClass));
    registerSvcObject(obj);
    obj.bindString("name", &mName);
    registerSvcObject(obj);
}
//---------------------------------------------------------------------------

void ObjnetNode::task()
{
    ObjnetCommonNode::task();

    switch (mNetState)
    {
      case netnStart:
        mNetAddress = 0xFF;
        mTimer.stop();
        break;

      case netnConnecting:
        if (!mTimer.isActive())
        {
            ByteArray ba;
            ba.append(mBusAddress);
            sendServiceMessage(svcHello, ba);
            mTimer.start();
        }
        break;

      case netnDisconnecting:
        mInterface->flush();
        mTimer.stop();
        break;

      case netnAccepted:
      {
        mTimer.stop();
        mNetState = netnReady;
        break;
      }

      case netnReady:
        if (!mTimer.isActive())
        {
            mTimer.start();
        }
        break;
    }
}
//---------------------------------------------------------------------------

void ObjnetNode::acceptServiceMessage(unsigned char sender, SvcOID oid, ByteArray *ba)
{
//    #ifndef __ICCARM__
//    qDebug() << "node" << QString::fromStdString(mName) << "accept" << oid;
//    #endif

//    CommonMessage msg;
    switch (oid)
    {
      case svcHello: // translate hello msg
      {
        ba->append(mBusAddress);
        sendServiceMessage(svcHello, *ba);
        break;
      }

      case svcConnected:
      case svcDisconnected:
      case svcKill:
        sendServiceMessage(oid, *ba);
        break;

      default:;
    }
}

void ObjnetNode::parseServiceMessage(CommonMessage &msg)
{
    if (msg.isGlobal())
    {        
        StdAID aid = (StdAID)msg.globalId().aid;

//        #ifndef __ICCARM__
//        qDebug() << "node" << QString::fromStdString(mName) << "global" << aid;
//        #endif

        switch (aid)
        {
          case aidPollNodes:
            if (isConnected())
            {
                sendServiceMessage(svcEcho);
                if (mNetState == netnReady)
                    mNetTimeout = 0;
            }
            else
            {
                mNetState = netnConnecting;
            }
            break;

          case aidConnReset:
//            mNetAddress = 0x00;
//            mNetState = netnConnecting;
            mNetState = netnStart;
            break;

          default:;
        }
        return;
    }

    SvcOID oid = (SvcOID)msg.localId().oid;
    unsigned char remoteAddr = msg.localId().sender;

//    #ifndef __ICCARM__
//    qDebug() << "node" << QString::fromStdString(mName) << "parse" << oid;
//    #endif

    switch (oid)
    {
      case svcHello:
        mNetState = netnConnecting;
        break;

      case svcWelcome:
      case svcWelcomeAgain:
        if (msg.data().size() == 1)
        {
            mNetAddress = msg.data()[0];
            mNetState = netnAccepted;
            int len = mName.length();
            if (len > 8)
                len = 8;
            // send different info
            sendServiceMessage(remoteAddr, svcClass, ByteArray(reinterpret_cast<const char*>(&mClass), sizeof(mClass)));
            sendServiceMessage(remoteAddr, svcName, ByteArray(mName.c_str(), len));
            sendServiceMessage(remoteAddr, svcEcho); // echo at the end of info
        }
        else if (mAdjacentNode) // remote addr
        {
            mAdjacentNode->acceptServiceMessage(remoteAddr, oid, &msg.data());
        }
        break;

        default:;
    }

    if (oid < mSvcObjects.size())
    {
        ObjectInfo &obj = mSvcObjects[oid];
        if (msg.data().size()) // write
        {
            obj.write(msg.data());
        }
        else
        {
            sendServiceMessage(remoteAddr, oid, obj.read());
        }
    }
}
//---------------------------------------------------------------------------

void ObjnetNode::parseMessage(CommonMessage &msg)
{
    if (msg.isGlobal())
    {
        return;
    }

    unsigned char oid = msg.localId().oid;
    unsigned char remoteAddr = msg.localId().sender;

    if (oid < mObjects.size())
    {
        ObjectInfo &obj = mObjects[oid];
        if (msg.data().size()) // write
        {
            obj.write(msg.data());
        }
        else
        {
            sendMessage(remoteAddr, oid, obj.read());
        }
    }
}
//---------------------------------------------------------

void Objnet::ObjnetNode::onTimer()
{
    if (mNetState == netnConnecting)
    {
        mNetState = netnDisconnecting;
    }

    mNetTimeout += mTimer.interval();
    if (mNetTimeout >= 1000)
    {
        mNetState = netnStart;
    }
}
