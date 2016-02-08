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
#warning name ne rabotaet, t.k. ptr is being changed
    obj.bindVariableRO("name", const_cast<char*>(mName.c_str()), mName.length());
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
        int len = mName.length();
        if (len > 8)
            len = 8;
        // send different info
        sendServiceMessage(svcClass, ByteArray(reinterpret_cast<const char*>(&mClass), sizeof(mClass)));
        sendServiceMessage(svcName, ByteArray(mName.c_str(), len));
        sendServiceMessage(svcEcho); // echo at the end of info
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
    CommonMessage msg;
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
            mNetAddress = 0x00;
            mNetState = netnConnecting;
            break;

          default:;
        }
        return;
    }

    SvcOID oid = (SvcOID)msg.localId().oid;
    unsigned char remoteAddr = msg.localId().sender;
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
//            if (mAdjacentNode)
//            {
//                mAdjacentNode->mNetAddress = mNetAddress;
//            }
        }
        else if (mAdjacentNode)// remote addr
        {
            mAdjacentNode->acceptServiceMessage(remoteAddr, oid, &msg.data());
        }
        break;

        default:;
    }

    if (oid < mSvcObjects.size())
    {
        ObjectInfo &obj = mSvcObjects[oid];
        if ((obj.mWriteSize > 0) && ((size_t)msg.data().size() == obj.mWriteSize))
        {
            for (size_t i=0; i<obj.mWriteSize; i++)
                reinterpret_cast<unsigned char*>(obj.mWritePtr)[i] = msg.data()[i];
        }
        if (obj.mReadSize > 0)
        {
            ByteArray ba(reinterpret_cast<const char*>(obj.mReadPtr), obj.mReadSize);
            sendServiceMessage(remoteAddr, oid, ba);
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

    SvcOID oid = (SvcOID)msg.localId().oid;
    unsigned char remoteAddr = msg.localId().sender;

    if (oid < mObjects.size())
    {
        ObjectInfo &obj = mObjects[oid];
        if ((obj.mWriteSize > 0) && ((size_t)msg.data().size() == obj.mWriteSize))
        {
            for (size_t i=0; i<obj.mWriteSize; i++)
                reinterpret_cast<unsigned char*>(obj.mWritePtr)[i] = msg.data()[i];
        }
        if (obj.mReadSize > 0)
        {
            ByteArray ba(reinterpret_cast<const char*>(obj.mReadPtr), obj.mReadSize);
            sendMessage(remoteAddr, oid, ba);
        }
    }
}

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
