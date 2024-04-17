#include "objtable.h"
#undef ByteArray

ObjTable::ObjTable(ObjnetDevice *dev, QWidget *parent) :
    QTreeView(parent),
    m_device(dev),
    m_flag(false)
{
    setMinimumWidth(412);
    setDragEnabled(true);
    setModel(&m_model);
    setAnimated(true);
//    setSelectionBehavior();
    setSelectionMode(QTreeView::ExtendedSelection);
    setEditTriggers(QTreeView::DoubleClicked | QTreeView::AnyKeyPressed);

    connect(this, &ObjTable::clicked, this, &ObjTable::onClick);

    connect(dev, &ObjnetDevice::ready, this, &ObjTable::updateTable);
    connect(dev, &ObjnetDevice::objectReceived, this, &ObjTable::updateObject);
    connect(dev, &ObjnetDevice::timedObjectReceived, this, &ObjTable::updateTimedObject);

    if (m_device->isReady())
        updateTable();
}

//void ObjTable::setDevice(ObjnetDevice *dev)
//{
//    m_device = dev;

//    updateTable();
//}

void ObjTable::updateTable()
{
    disconnect(&m_model, &QStandardItemModel::itemChanged, this, &ObjTable::itemChanged);
    m_model.clear();

    if (m_device)
    {
        int cnt = m_device->objectCount();
//        m_model.setColumnCount(4);
//        m_model.setRowCount(cnt);
        QStringList objtablecolumns;
        objtablecolumns << "name" << "value" << "type" << "flags";
        m_model.setHorizontalHeaderLabels(objtablecolumns);

        QStandardItem *root = m_model.invisibleRootItem();

        for (int i=0; i<cnt; i++)
        {
            ObjectInfo *info = m_device->objectInfo(i);
            if (info)
            {
                root->appendRow(createRow(info));
                if (info->isWritable() && !info->isInvokable()) // read-write naoborot
                    m_device->requestObject(info->name());
            }
            else
                root->appendRow(new QStandardItem("ERROR"));
        }

        for (int i=0; i<cnt; i++)
        {
            ObjectInfo *info = m_device->objectInfo(i);
            QString name = info->name();
            if (info->flags() & ObjectInfo::Function)
            {
                QPushButton *btn = new QPushButton(name);
                setIndexWidget(m_model.index(i, 0), btn);
                if (info->flags() & ObjectInfo::Read) // na samom dele Write, no tut naoborot)
                    connect(btn, &QPushButton::clicked, [this, name](){if (m_device) m_device->sendObject(name);});
                else
                    connect(btn, &QPushButton::clicked, [this, name](){if (m_device) m_device->requestObject(name);});
            }
        }

        setColumnWidth(0, 120);
        setColumnWidth(1, 150);
        setColumnWidth(2, 80);
        setColumnWidth(3, 50);

//        setContextMenuPolicy(Qt::CustomContextMenu);
//        connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onObjectMenu(QPoint)));
    }
    connect(&m_model, &QStandardItemModel::itemChanged, this, &ObjTable::itemChanged);
}

QList<QStandardItem *> ObjTable::createRow(ObjectInfo *info)
{
    QList<QStandardItem *> items;
    if (!info)
        return items;

    QString name = info->name();

    QStandardItem *nameItem = new QStandardItem(name);
    nameItem->setData(info->id(), Qt::UserRole);
    nameItem->setEditable(false);

    QString wt = QMetaType::typeName(info->wType());
    if (wt == "QString")
        wt = "string";
    else if (wt == "QByteArray" && info->description().writeSize)
        wt = "common";

    QString rt = QMetaType::typeName(info->rType());
    if (rt == "QString")
        rt = "string";
    else if (rt == "QByteArray" && info->description().readSize)
        rt = "common";

    unsigned char flags = info->flags();
    QString typeName;

    if (info->isCompound())
        typeName = "struct";
    else if (flags & ObjectInfo::Function)
        typeName = wt + "(" + rt + ")";
    else if (flags & ObjectInfo::Dual)
        typeName = "r:"+wt+"/w:"+rt;
    else if (flags & ObjectInfo::Read)
        typeName = rt;
    else if (flags & ObjectInfo::Write)
        typeName = wt;

    QString flagString = "-fdhsrwv";
    for (int j=0; j<8; j++)
        if (!(flags & (1<<j)))
            flagString[7-j] = '-';

    if (info->isArray())
    {
        int cnt = info->isReadable()? info->rCount(): info->wCount();
        for (int i=0; i<cnt; i++)
        {
            QString key = QString("[%2]").arg(i);
            QList<QStandardItem *> items;
            items << new QStandardItem(key);
            items << new QStandardItem("n/a");
            items << new QStandardItem(typeName);
//            items << new QStandardItem(flagString);
            nameItem->appendRow(items);
        }
        typeName += QString("[%1]").arg(cnt);
    }
    else if (info->isCompound())
    {
        for (int i=0; i<info->subobjectCount(); i++)
            nameItem->appendRow(createRow(&info->subobject(i)));
    }

    QStandardItem *valueItem = new QStandardItem("n/a");
    valueItem->setData(info->id(), Qt::UserRole);
    if (info->isCompound() || info->isArray() || !info->isReadable())
    {
        valueItem->setEditable(false);
    }

    QStandardItem *typeItem = new QStandardItem(typeName);
    typeItem->setEditable(false);
    QStandardItem *flagItem = new QStandardItem(flagString);
    flagItem->setEditable(false);

    items << nameItem;
    items << valueItem;
    items << typeItem;
    items << flagItem;
    return items;
}

void ObjTable::updateItem(QStandardItem *parentItem, QString name, QVariant value)
{
    for (int i=0; i<parentItem->rowCount(); i++)
    {
        QStandardItem *item = parentItem->child(i);
        if (item->text() == name)
        {
            QStandardItem *valueItem = m_model.itemFromIndex(item->index().siblingAtColumn(1));
            m_flag = true;
            valueItem->setText(valueToString(value));
            m_flag = false;
            if (item->hasChildren())
            {
                if (value.type() == QMetaType::QVariantList)
                {
                    QVariantList vlist = value.toList();
                    for (int i=0; i<vlist.size(); i++)
                    {
                        QString key = QString("[%2]").arg(i);
                        updateItem(item, key, vlist[i]);
                    }
                }
                else if (value.type() == QMetaType::QVariantMap)
                {
                    QVariantMap vmap = value.toMap();
                    for (QString key: vmap.keys())
                        updateItem(item, key, vmap[key]);
                }
            }
            break;
        }
    }
}

void ObjTable::updateObject(QString name, QVariant value)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    if (dev == m_device)
        updateItem(m_model.invisibleRootItem(), name, value);
}

void ObjTable::updateTimedObject(QString name, uint32_t timestamp, QVariant value)
{
//    if (timestamp)
    updateObject(name, value);
}

QVariant ObjTable::readItem(ObjectInfo *info, QStandardItem *item)
{
    if (info->isArray())
    {
        QVariantList vlist;
        int cnt = info->isReadable()? info->rCount(): info->wCount();
        for (int i=0; i<cnt; i++)
            vlist << valueFromString(item->child(i, 1)->text(), info->rType());
        return vlist;
    }
    else if (info->isCompound())
    {
        QVariantMap vmap;
        for (int i=0; i<info->subobjectCount(); i++)
        {
            ObjectInfo *sub = &info->subobject(i);
            vmap[item->child(i)->text()] = readItem(sub, item->child(i));
        }
        return vmap;
    }

    QString text = m_model.itemFromIndex(item->index().siblingAtColumn(1))->text();
    return valueFromString(text, info->rType());
}

void ObjTable::itemChanged(QStandardItem *item)
{
    if (m_flag)
        return;

    while (item->index().siblingAtColumn(0).parent().isValid())
        item = m_model.itemFromIndex(item->index().siblingAtColumn(0).parent());

    if (m_device)
    {
        uint8_t oid = item->data(Qt::UserRole).toInt();
        ObjectInfo *info = m_device->objectInfo(oid);
        QVariant val = readItem(info, item);
        info->fromVariant(val);

        if (item->column() != 0)
            item = m_model.itemFromIndex(item->index().siblingAtColumn(0));
        updateItem(m_model.invisibleRootItem(), item->text(), val);

        if (!(info->flags() & ObjectInfo::Function))
            m_device->sendObject(info->name());
    }
}

QString ObjTable::valueToString(QVariant value)
{
    QString val;
    if (!value.isValid())
        return "<invalid>";
    if (value.type() == QVariant::ByteArray)
        val = value.toByteArray().toHex();
    else if (value.type() == QMetaType::Float)
    {
        val = QString::number(value.toFloat());
    }
    else// if (value.type() == QVariant::String)
    {
        QMetaType::Type mt = static_cast<QMetaType::Type>(value.type());
        if (mt == QMetaType::SChar)
        {
            value = value.toInt();
            val = value.toString();
        }
//        else if (mt == QMetaType::UChar)
//        {
//            value = value.toInt();
//            val = value.toString();
//        }
        else if (mt == QMetaType::UChar)
        {
            val = QString().sprintf("0x%02X", value.toInt());
        }
        else if (mt == QMetaType::UShort)
        {
            val = QString().sprintf("0x%04X", value.toInt());
        }
        else if (mt == QMetaType::ULong)
        {
            val = QString().sprintf("0x%08X", value.toInt());
        }
        else if (mt == QMetaType::QVector3D)
        {
            QVector3D v = value.value<QVector3D>();
            val = QString().sprintf("(x=%.3f, y=%.3f, z=%.3f)", v.x(), v.y(), v.z());
        }
        else if (mt == QMetaType::QQuaternion)
        {
            QQuaternion q = value.value<QQuaternion>();
            val = QString().sprintf("(w=%.3f, x=%.3f, y=%.3f, z=%.3f)", q.scalar(), q.x(), q.y(), q.z());
        }
        else if (mt == QMetaType::QStringList)
        {
            val = QString("[%1]").arg(value.toStringList().join(", "));
        }
        else if (mt == QMetaType::QVariantList)
        {
            QStringList items;
            QVariantList vlist = value.toList();
            for (QVariant v: vlist)
                items << valueToString(v);
            val = QString("[%1]").arg(items.join(", "));
        }
        else if (mt == QMetaType::QVariantMap)
        {
            QStringList items;
            QVariantMap vmap = value.toMap();
            for (QString key: vmap.keys())
                items << QString("%1: %2").arg(key).arg(valueToString(vmap[key]));
            val = QString("{%1}").arg(items.join(", "));
        }
        else
            val = value.toString();
    }
    return val;
}

QVariant ObjTable::valueFromString(QString s, ObjectInfo::Type t)
{
    QVariant val(s);
    if (t == ObjectInfo::Common)
    {
        return QByteArray::fromHex(val.toByteArray());
    }
    else if (t == ObjectInfo::Char)
    {
        if (s.length() >= 1)
        {
            val = s.toLatin1().data()[0];
            val.convert(QMetaType::Char);
            return val;
        }
        return QVariant(); // invalid
    }
    else
    {
        if (val.toString().startsWith("0x"))
            val = val.toString().mid(2).toUInt(nullptr, 16);
        val.convert(t);
    }
    return val;
}

void ObjTable::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    if (!m_device)
        return;

    QList<ObjectInfo*> selectedObjects;
    for (QModelIndex &index: selectedIndexes())
    {
        if (index.column() > 0)
            continue;
        int idx = m_model.data(index, Qt::UserRole).toInt();
        selectedObjects << m_device->objectInfo(idx);
    }

//    selectRow(currentRow());
//    QTableWidgetItem *it = item(currentRow(), 0);
//    int idx = it->data(Qt::UserRole).toInt();

    QMimeData *mimeData = new QMimeData;

//    QString mimetype = "application/x-onb-object";
//    if (!m_device->objectInfo(idx)->isWritable()) // inverted interpretation of read/write again
//        mimetype = "application/x-onb-nonreadable";
//    else if (m_device->objectInfo(idx)->wType() == ObjectInfo::Void)
//        mimetype = "application/x-onb-object-void";
//    else if (m_device->objectInfo(idx)->wType() == ObjectInfo::Common)
//        mimetype = "application/x-onb-object-common";
//    else if (m_device->objectInfo(idx)->wType() == ObjectInfo::String)
//        mimetype = "application/x-onb-object-string";

//    QByteArray ba;
//    m_device->objectInfo(idx)->description().read(ba);
//
//    mimeData->setData("application/x-onb-object-desc", ba);
//    mimeData->setData("application/x-onb-object-name", it->text().toLocal8Bit());

//    int wcnt = m_device->objectInfo(idx)->wCount();
//    if (wcnt > 1)
//    {
//        QByteArray baSize(reinterpret_cast<const char*>(&wcnt), sizeof(int));
//        mimeData->setData("application/x-onb-object-array-size", baSize);
//    }

    QByteArray devptr(reinterpret_cast<const char*>(&m_device), sizeof(ObjnetDevice*));
    mimeData->setData("application/x-onb-device", devptr);
//    ObjectInfo *objinfo = m_device->objectInfo(idx);
    if (selectedObjects.count() == 1)
    {
        QByteArray objptr(reinterpret_cast<const char*>(&selectedObjects[0]), sizeof(ObjectInfo*));
        mimeData->setData("application/x-onb-object", objptr);
    }
    else
    {
        QByteArray data;
        uint32_t cnt = selectedObjects.size();
        QByteArray cntdata(reinterpret_cast<const char*>(&cnt), sizeof(uint32_t));
        data.append(cntdata);
        for (ObjectInfo *obj: selectedObjects)
        {
            QByteArray objptr(reinterpret_cast<const char*>(&obj), sizeof(ObjectInfo*));
            data.append(objptr);
        }
        mimeData->setData("application/x-onb-object-list", data);
    }

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
//    drag->setHotSpot();
//    drag->setPixmap(pixmap);
    drag->exec();
}

void ObjTable::onClick(const QModelIndex &index)
{
    if (index.column() > 0)
        return;
    QString name = m_model.itemFromIndex(index.siblingAtColumn(0))->text();
    m_device->requestObject(name);
}
