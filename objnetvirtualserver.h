#ifndef OBJNETVIRTUALSERVER_H
#define OBJNETVIRTUALSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include "objnetmsg.h"
#include "qelapsedtimer.h"

using namespace Objnet;

class OviCodec
{
private:
    QByteArray mBuffer;
    unsigned char cs, esc, cmd_acc;

public:
    OviCodec();
    QByteArray encode(const QByteArray &ba);
    QByteArray decode(QByteArray &ba);
};

class ObjnetVirtualServer : public QTcpServer
{
    Q_OBJECT

private:
    QMultiHash<QString, QTcpSocket*> mNets;
    OviCodec mCodec;

//    QElapsedTimer mTimer;

public:
    explicit ObjnetVirtualServer(QObject *parent = 0);

signals:
    void message(QString);
    void message(QString, CommonMessage&);

private slots:
    void clientConnected();
    void clientRead();
    void clientDisconnected();

public slots:
    void setEnabled(bool enabled);
};

#endif // OBJNETVIRTUALSERVER_H
