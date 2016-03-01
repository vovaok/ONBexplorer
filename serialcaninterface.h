#ifndef SerialCanInterface_H
#define SerialCanInterface_H

#include "objnetInterface.h"
#include "serialcan.h"
#include <QQueue>

namespace Objnet
{

class SerialCanInterface : public ObjnetInterface
{
    Q_OBJECT

private:
    SerialCan *mCan;
    QQueue<CommonMessage> mRxQueue;
    typedef struct {unsigned long id, mask;} Filter;
    QVector<Filter> mFilters;

protected:

public:
    explicit SerialCanInterface(SerialCan *can);

    bool write(CommonMessage &msg);
    bool read(CommonMessage &msg);
    void flush();

    int availableWriteCount();

    int addFilter(unsigned long id, unsigned long mask=0xFFFFFFFF);
    void removeFilter(int number);
    
signals:

private slots:
    void receiveHandler(ulong id, QByteArray &data);
//    void transmitHandler();
    
public slots:
    
};

}

#endif // SerialCanInterface_H
