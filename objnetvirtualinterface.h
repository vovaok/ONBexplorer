#ifndef OBJNETVIRTUALINTERFACE_H
#define OBJNETVIRTUALINTERFACE_H

#include "objnetInterface.h"
#include <QTcpSocket>
#include <QHostAddress>

namespace Objnet
{

class ObjnetVirtualInterface : public ObjnetInterface
{
    Q_OBJECT

private:
    QTcpSocket *mSocket;
    QString mNetname;
    typedef struct {unsigned long id, mask;} Filter;
    QVector<Filter> mFilters;
    QQueue<CommonMessage> mRxQueue;
    bool mActive;

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketRead();

public:
    ObjnetVirtualInterface(QString netname);

    bool write(CommonMessage &msg);
    bool read(CommonMessage &msg);
    void flush();

    int addFilter(unsigned long id, unsigned long mask=0xFFFFFFFF);
    void removeFilter(int number);

    bool isActive() const {return mActive;}

    QString netname() const {return mNetname;}

public slots:
    void setActive(bool enabled);
};

}

#endif // OBJNETVIRTUALINTERFACE_H
