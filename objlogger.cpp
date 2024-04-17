#include "objlogger.h"

using namespace Objnet;

ObjLogger::ObjLogger(QWidget *parent)
    : QWidget(parent, Qt::Tool)
{
    setWindowTitle("Object logger");
    setAcceptDrops(true);

    m_enableChk = new QCheckBox("Enable logging");

    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(0, 1000);
    m_intervalSpin->setValue(100);

    m_list = new QListWidget();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ObjLogger::sendRequest);

    connect(m_enableChk, &QCheckBox::clicked, this, &ObjLogger::setLoggingEnabled);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->addWidget(m_enableChk);
    lay->addWidget(m_intervalSpin);
    lay->addWidget(m_list);
    setLayout(lay);
}

void ObjLogger::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_enableChk->isChecked())
    {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat("application/x-onb-object"))
    {
        QByteArray objptr = event->mimeData()->data("application/x-onb-object");
        ObjectInfo *obj = *reinterpret_cast<ObjectInfo**>(objptr.data());
        if (!obj->isWritable()) // inverted interpretation of read/write again
            event->ignore();
        if (obj->isStorable())
            event->ignore();
        else if (obj->wType() == ObjectInfo::Void)
            event->ignore();
        else if (obj->wType() == ObjectInfo::Common)
            event->ignore();
        else if (obj->wType() == ObjectInfo::String)
            event->ignore();
        else
            event->acceptProposedAction();
    }
    else if (event->mimeData()->hasFormat("application/x-onb-object-list"))
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void ObjLogger::dragMoveEvent(QDragMoveEvent *event)
{

}

void ObjLogger::dropEvent(QDropEvent *event)
{
    if (m_enableChk->isChecked())
    {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat("application/x-onb-object"))
    {
        QByteArray objptr = event->mimeData()->data("application/x-onb-object");
        ObjectInfo *obj = *reinterpret_cast<ObjectInfo**>(objptr.data());
        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());

        m_device = dev;

        m_list->addItem(obj->name());

        event->acceptProposedAction();
    }
    else if (event->mimeData()->hasFormat("application/x-onb-object-list"))
    {
        QByteArray listptr = event->mimeData()->data("application/x-onb-object-list");
        uint32_t cnt = *reinterpret_cast<uint32_t*>(listptr.data());
        ObjectInfo **objlist = reinterpret_cast<ObjectInfo**>(listptr.data() + sizeof(uint32_t));
        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());

        m_device = dev;

        QStringList names;
        for (int i=0; i<cnt; i++)
            names << objlist[i]->name();

        m_list->addItems(names);

        event->acceptProposedAction();
        return;
    }
    else
    {
        event->ignore();
    }
}

void ObjLogger::setLoggingEnabled(bool enabled)
{
    m_list->setDisabled(enabled);
    m_intervalSpin->setDisabled(enabled);

    if (enabled)
    {
        QDateTime date = QDateTime::currentDateTime();
        QString filename = date.toString("dd-MM-yyyy-hh-mm-ss") + ".txt";
        m_filename = "D:/work/sberbot/logs/" + filename;
        m_elapsed.start();
        m_timer->start(m_intervalSpin->value());
        if (m_device)
            connect(m_device, &ObjnetDevice::objectGroupReceived, this, &ObjLogger::updateObjectGroup);
    }
    else
    {
        if (m_device)
            disconnect(m_device, &ObjnetDevice::objectGroupReceived, this, &ObjLogger::updateObjectGroup);
        m_elapsed.invalidate();
        m_timer->stop();
    }
}

void ObjLogger::sendRequest()
{
    QStringList names;
    for (int i=0; i<m_list->count(); i++)
        names << m_list->item(i)->text();
    if (m_device)
        m_device->groupedRequest(names.toVector().toStdVector());
}

void ObjLogger::updateObjectGroup(QVariantMap values)
{
    if (m_filename.isEmpty())
        return;

    QByteArray time = QByteArray::number(m_elapsed.elapsed());

    QByteArrayList list;
    list << time;

    for (int i=0; i<m_list->count(); i++)
    {
        QString name = m_list->item(i)->text();
        QString value = values[name].toString();
        list << value.toLatin1();
    }

    QFile f(m_filename);
    f.open(QIODevice::Append | QIODevice::Text);
    f.write(list.join(';')+"\n");
    f.close();
}

