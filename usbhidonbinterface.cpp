#include "usbhidonbinterface.h"

using namespace Objnet;

UsbHidOnbInterface::UsbHidOnbInterface(UsbHid *usbhid) :
    usb(usbhid),
    mSeqNo(0),
    mFirstTime(true)
{
    mMaxFrameSize = 8;

    usb->setCurrentReportId(0x23);
    usb->availableDevices();
    usb->setDevice();
    usb->open();
    mEtimer.start();
}

UsbHidOnbInterface::~UsbHidOnbInterface()
{
    usb->close();
}


bool Objnet::UsbHidOnbInterface::write(Objnet::CommonMessage &msg)
{
    if (!usb->isOpen())
        return false;
    unsigned long id = msg.rawId();
    unsigned char sz = msg.data().size();
    QByteArray ba;
    ba.resize(13);
    *reinterpret_cast<unsigned long*>(ba.data()) = id;
    ba[4] = sz;
    for (int i=0; i<sz; i++)
        ba[5+i] = msg.data()[i];
    usb->setFeature(0x01, ba);

    QString strdata;
    for (int i=0; i<msg.data().size(); i++)
    {
        QString s;
        unsigned char byte = msg.data()[i];
        s.sprintf("%02X ", byte);
        strdata += s;
    }
    qDebug() << "[" << mEtimer.elapsed() << "] >>" << QString().sprintf("(0x%08x)", id) << strdata;
    return true;
}

bool Objnet::UsbHidOnbInterface::read(Objnet::CommonMessage &msg)
{
    if (!usb->isOpen())
        return false;
    QByteArray ba1 = usb->read(1);
    if (!ba1.size())
        return false;
    if (mFirstTime)
    {
        mSeqNo = (unsigned char)ba1[0];
        mFirstTime = false;
    }
    if ((unsigned char)ba1[0] == mSeqNo)
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

        QString strdata;
        for (int i=0; i<msg.data().size(); i++)
        {
            QString s;
            unsigned char byte = msg.data()[i];
            s.sprintf("%02X ", byte);
            strdata += s;
        }
        qDebug() << "[" << mEtimer.elapsed() << "] <<" << QString().sprintf("(0x%08x)", id) << strdata;
    }
//    else
//        qDebug() << "ne success(";
    return success;
}

void Objnet::UsbHidOnbInterface::flush()
{
    qDebug() << "[UsbHidOnbInterface]: flush is not implemented";
}

int Objnet::UsbHidOnbInterface::addFilter(unsigned long id, unsigned long mask)
{
    qDebug() << "[UsbHidOnbInterface]: Filter is not implemented. id=" << id << "mask=" << mask;
    return 0;
}

void Objnet::UsbHidOnbInterface::removeFilter(int number)
{
    qDebug() << "[UsbHidOnbInterface]: Filter is not implemented. number=" << number;
}
