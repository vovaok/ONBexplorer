#ifndef USBHIDONBINTERFACE_H
#define USBHIDONBINTERFACE_H

#include "objnetInterface.h"
#include "usbhidthread.h"
#include <QElapsedTimer>

namespace Objnet
{

class UsbHidOnbInterface : public ObjnetInterface
{
private:
    UsbHid *usb;
    unsigned char mSeqNo;
    bool mFirstTime;
    QElapsedTimer mEtimer;

public:
    explicit UsbHidOnbInterface(UsbHid *usbhid);
    virtual ~UsbHidOnbInterface();

    bool write(CommonMessage &msg);
    bool read(CommonMessage &msg);
    void flush();

    int addFilter(unsigned long id, unsigned long mask=0xFFFFFFFF);
    void removeFilter(int number);
};

}

#endif // USBHIDONBINTERFACE_H
