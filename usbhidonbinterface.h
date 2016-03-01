#ifndef USBHIDONBINTERFACE_H
#define USBHIDONBINTERFACE_H

#include "objnetInterface.h"
#include "usbhidthread.h"
#include <QElapsedTimer>

namespace Objnet
{

class UsbHidOnbInterface : public ObjnetInterface
{
    Q_OBJECT

private:
    UsbHidThread *usb;
    unsigned char mReadSeqNo, mSeqNo;
    bool mFirstTime;

public:
    explicit UsbHidOnbInterface(UsbHidThread *usbhid);
    virtual ~UsbHidOnbInterface();

    bool write(CommonMessage &msg);
    bool read(CommonMessage &msg);
    void flush();

    int availableWriteCount();

    int addFilter(unsigned long id, unsigned long mask=0xFFFFFFFF);
    void removeFilter(int number);

signals:
    void message(QString, CommonMessage&);

private slots:
    void onReportReceive(const QByteArray &ba);
};

}

#endif // USBHIDONBINTERFACE_H
