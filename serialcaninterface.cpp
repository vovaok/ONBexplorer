#include "serialcaninterface.h"

using namespace Objnet;

SerialCanInterface::SerialCanInterface(SerialCan *can) :
    mCan(can)
{
    mMaxFrameSize = 8;
    connect(mCan, SIGNAL(onMessage(ulong,QByteArray&)), this, SLOT(receiveHandler(ulong,QByteArray&)));
    //connect(mCan, SIGNAL(onMessageSent(ulong,QByteArray&)), this, SLOT(transmitHandler()));
}

void SerialCanInterface::receiveHandler(ulong id, QByteArray &data)
{
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

    if (!accept)
        return;

    CommonMessage msg;
    msg.setId(id);
    msg.setData(data);
    mRxQueue << msg;
}

bool SerialCanInterface::write(CommonMessage &msg)
{
    mCan->sendMessage(msg.rawId(), msg.data());
    return true;
}

bool SerialCanInterface::read(CommonMessage &msg)
{
    if (mRxQueue.isEmpty())
        return false;
    msg = mRxQueue.dequeue();
    return true;
}

void SerialCanInterface::flush()
{
    mCan->clearBoardBuffer();
}

int SerialCanInterface::availableWriteCount()
{
    return 256;
}

int SerialCanInterface::addFilter(unsigned long id, unsigned long mask)
{
    Filter f;
    f.id = id;
    f.mask = mask;
    mFilters << f;
    return mFilters.size() - 1;
}

void SerialCanInterface::removeFilter(int number)
{
    if (number>=0 && number < mFilters.size())
        mFilters.remove(number);
}

//void SerialCanInterface::transmitHandler()
//{
//}
