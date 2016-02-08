#include "objnetvirtualinterface.h"

using namespace Objnet;

ObjnetVirtualInterface::ObjnetVirtualInterface(QString netname) :
    mNetname(netname),
    mActive(false)
{
    mMaxFrameSize = 8;
    mSocket = new QTcpSocket(this);
    connect(mSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
    connect(mSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
    connect(mSocket, SIGNAL(readyRead()), SLOT(onSocketRead()));
    mSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    //mSocket->connectToHost(QHostAddress::LocalHost, 23230);
}
//---------------------------------------------------------

bool ObjnetVirtualInterface::write(Objnet::CommonMessage &msg)
{
    QByteArray ba;
    unsigned long id = msg.rawId();
    ba.append(reinterpret_cast<const char*>(&id), 4);
    ba.append(msg.data());
    ba.prepend(ba.size());
    if (mSocket->isOpen())
        mSocket->write(ba);
    return true;
}

bool ObjnetVirtualInterface::read(Objnet::CommonMessage &msg)
{
    if (mRxQueue.isEmpty())
        return false;
    msg = mRxQueue.dequeue();
    return true;
}

void ObjnetVirtualInterface::flush()
{
    mSocket->flush();
}

int ObjnetVirtualInterface::addFilter(unsigned long id, unsigned long mask)
{
    Filter f;
    f.id = id;
    f.mask = mask;
    mFilters << f;
    return mFilters.size() - 1;
}

void ObjnetVirtualInterface::removeFilter(int number)
{
    if (number>=0 && number < mFilters.size())
        mFilters.remove(number);
}

void ObjnetVirtualInterface::setActive(bool enabled)
{
    if (mActive != enabled)
    {
        mActive = enabled;
        if (mActive)
            mSocket->connectToHost(QHostAddress::LocalHost, 23230);
        else
            mSocket->disconnectFromHost();
    }
}
//---------------------------------------------------------

void ObjnetVirtualInterface::onSocketConnected()
{
    QByteArray ba;
    unsigned long id = 0xFF000000;
    ba.append(reinterpret_cast<const char*>(&id), 4);
    ba.append(mNetname.toLatin1());
    ba.prepend(ba.size());
    mSocket->write(ba);
}

void ObjnetVirtualInterface::onSocketDisconnected()
{
    mRxQueue.clear();
}

void ObjnetVirtualInterface::onSocketRead()
{
    QByteArray ba = mSocket->read(1);
    while (!ba.isEmpty())
    {
        int sz = ba[0];
        ba = mSocket->read(sz);

        unsigned long id = *reinterpret_cast<unsigned long*>(ba.data());

        bool accept = false;
        if (!mFilters.size())
            accept = true;
        foreach (Filter f, mFilters)
        {
            if ((f.id & f.mask) == (id & f.mask))
            {
                accept = true;
                break;
            }
        }

        if (accept)
        {
            ba.remove(0, 4);
            CommonMessage msg;
            msg.setId(id);
            msg.setData(ba);
            mRxQueue << msg;
        }

        ba = mSocket->read(1);
    }
}
