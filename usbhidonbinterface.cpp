#include "usbhidonbinterface.h"

using namespace Objnet;

UsbHidOnbInterface::UsbHidOnbInterface(UsbHidThread *usbhid) :
    usb(usbhid),
    mReadSeqNo(0), mSeqNo(0),
    mFirstTime(true)
{
    mMaxFrameSize = 8;

    connect(usb, UsbHidThread::reportReceived, this, UsbHidOnbInterface::onReportReceive, Qt::QueuedConnection);
    usb->setPollingInterval(1);
    usb->setCurrentReportId(0x23, 2);
    usb->start(QThread::HighPriority);

//    usb->availableDevices();
//    usb->setDevice();
//    usb->open();
}

UsbHidOnbInterface::~UsbHidOnbInterface()
{    
//    usb->close();
//    usb->requestInterruption();
//    while (!usb->isFinished());
}
//---------------------------------------------------------

void UsbHidOnbInterface::onReportReceive(const QByteArray &ba)
{
    if (ba.size() >= 2)
    {
        unsigned char sz = (unsigned char)ba[0];
        mReadSeqNo = (unsigned char)ba[1];
        if (mFirstTime)
        {
            mSeqNo = mReadSeqNo - sz;
            mFirstTime = false;
        }
    }
}
//---------------------------------------------------------

bool UsbHidOnbInterface::write(Objnet::CommonMessage &msg)
{
    if (!usb->isReady()) //isOpen())
        return false;
    unsigned long id = msg.rawId();
    unsigned char sz = msg.data().size();
    QByteArray ba;
    ba.resize(13);
    *reinterpret_cast<unsigned long*>(ba.data()) = id;
    ba[4] = sz;
    for (int i=0; i<sz; i++)
        ba[5+i] = msg.data()[i];
    //qDebug() << "start set feature";
    usb->setFeature(0x01, ba);
    //qDebug() << "end";

    emit message("usbonb", msg); // for debug purposes
    return true;
}

bool UsbHidOnbInterface::read(Objnet::CommonMessage &msg)
{
    if (!usb->isReady()) // isOpen())
        return false;
//    QByteArray ba1 = usb->read(1);
//    if (!ba1.size())
//        return false;
//    mReadSeqNo = (unsigned char)ba1[0];

    if (mReadSeqNo == mSeqNo)
        return false;

    QByteArray ba;
    bool success = usb->getFeature(0x01, ba);
    if (success)
    {
        mSeqNo++;
        unsigned long id = *reinterpret_cast<unsigned long*>(ba.data());
        msg.setId(id);
        unsigned char sz = ba[4];
        msg.setData(QByteArray(ba.data() + 5, sz));

        emit message("usbonb", msg); // for debug purposes
    }
//    else
//        qDebug() << "ne success(";
    return success;
}

int UsbHidOnbInterface::availableWriteCount()
{
    return 256;
}

void UsbHidOnbInterface::flush()
{
    qDebug() << "[UsbHidOnbInterface]: flush is not implemented";
}

int UsbHidOnbInterface::addFilter(unsigned long id, unsigned long mask)
{
    qDebug() << "[UsbHidOnbInterface]: Filter is not implemented. id=" << id << "mask=" << mask;
    return 0;
}

void UsbHidOnbInterface::removeFilter(int number)
{
    qDebug() << "[UsbHidOnbInterface]: Filter is not implemented. number=" << number;
}
