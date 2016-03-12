#include "objnetvirtualserver.h"

OviCodec::OviCodec() :
    cs(0), esc(0), cmd_acc(0)
{
}

QByteArray OviCodec::encode(const QByteArray &ba)
{
    QByteArray out;
    char cs = 0;
    out.append('{');
    int sz = ba.size();
    for (char i=0; i<=sz; i++)
    {
        char b = i<sz? ba[i]: cs;
        cs -= b;
        switch (b)
        {
            case '{':
            case '}':
            case '\\':
                out.append('\\');
                b ^= 0x20;
        }
        out.append(b);
    }
    out.append('}');
    return out;
}

QByteArray OviCodec::decode(QByteArray &ba)
{
    for (int i=0; i<ba.size(); i++)
    {
        char byte = ba[i];
        switch (byte)
        {
          case '\\':
            esc = 1;
            break;

          case '{':
            mBuffer.clear();
            cs = 0;
            esc = 0;
            cmd_acc = 1;
            break;

          case '}':
            if (cmd_acc)
            {
                if (!cs)
                {
                    mBuffer.resize(mBuffer.size()-1); // remove checksum
                    ba.remove(0, i+1);
                    return mBuffer;
                }
                cmd_acc = 0;
            }
            break;

          default:
            if (!cmd_acc)
                break;
            if (esc)
                byte ^= 0x20;
            esc = 0;
            mBuffer.append(byte);
            cs += byte;
        }
    }
    return QByteArray();
}
//---------------------------------------------------------
//---------------------------------------------------------

ObjnetVirtualServer::ObjnetVirtualServer(QObject *parent) :
    QTcpServer(parent)
{
    connect(this, SIGNAL(newConnection()), SLOT(clientConnected()));
}

void ObjnetVirtualServer::clientConnected()
{
    QTcpSocket *socket;
    while ((socket = nextPendingConnection()))
    {
        connect(socket, SIGNAL(readyRead()), SLOT(clientRead()));
        connect(socket, SIGNAL(disconnected()), SLOT(clientDisconnected()));
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        emit message("[ONB server] client connected: " + socket->peerAddress().toString());
        if (socket->peerAddress() != QHostAddress("127.0.0.1"))
            mNets.insert("main", socket);
    }
}


void ObjnetVirtualServer::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    emit message("[ONB server] client disconnected: " + socket->peerAddress().toString());
    foreach (QString key, mNets.keys())
        mNets.remove(key, socket);
}

void ObjnetVirtualServer::setEnabled(bool enabled)
{
    if (enabled && !isListening())
    {
        listen(QHostAddress::Any, 51966);
        emit message("[ONB server] started");
    }
    else if (!enabled && isListening())
    {
        close();
        emit message("[ONB server] stopped");
    }
}

void ObjnetVirtualServer::clientRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray in = socket->readAll();
    if (!isListening())
        return;
//    qDebug() << "[" << mTimer.elapsed() << "] size =" << in.size();
    QByteArray ba;
    while (!(ba=mCodec.decode(in)).isEmpty())
    {
        unsigned long id = *reinterpret_cast<const unsigned long*>(ba.data());

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
                msg.setData(QByteArray(ba.data()+4, ba.size()-4));
                emit message(netname, msg);

                foreach (QTcpSocket* sock, mNets.values(netname))
                {
                    if (sock != socket)
                        sock->write(mCodec.encode(ba));
                }
            }
        }
    }
}
