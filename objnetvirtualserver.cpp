#include "objnetvirtualserver.h"

ObjnetVirtualServer::ObjnetVirtualServer(QObject *parent) :
    QTcpServer(parent)
{
    connect(this, SIGNAL(newConnection()), SLOT(clientConnected()));
    mTimer.start();
    listen(QHostAddress::Any, 23230);
}

void ObjnetVirtualServer::clientConnected()
{
    QTcpSocket *socket;
    while ((socket = nextPendingConnection()))
    {
        connect(socket, SIGNAL(readyRead()), SLOT(clientRead()));
        connect(socket, SIGNAL(disconnected()), SLOT(clientDisconnected()));
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }
}


void ObjnetVirtualServer::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    foreach (QString key, mNets.keys())
        mNets.remove(key, socket);
}

void ObjnetVirtualServer::clientRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray ba = socket->read(1);
    while (!ba.isEmpty())
    {
        int sz = ba[0];
        ba = socket->read(sz);

        unsigned long id = *reinterpret_cast<unsigned long*>(ba.data());

        QString netname;
        if (id == 0xff000000)
        {
            ba.remove(0, 4);
            netname = ba;
            mNets.insert(netname, socket);
        }
        else
        {
            foreach (QString key, mNets.keys())
            {
                if (mNets.contains(key, socket))
                {
                    netname = key;
                    break;
                }
            }
            if (!netname.isEmpty())
            {
                CommonMessage msg;
                msg.setId(id);
                QByteArray data = ba;
                data.remove(0, 4);
                msg.setData(data);
                QString text = QString().sprintf("[%d]\t", (int)mTimer.elapsed()) + netname;
                emit message(text, msg);

                ba.prepend(ba.size());

                foreach (QTcpSocket* sock, mNets.values(netname))
                {
                    if (sock != socket)
                        sock->write(ba);
                }
            }
        }
        ba = socket->read(1);
    }
}
