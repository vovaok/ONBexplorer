#include "objtable.h"

ObjTable::ObjTable(QWidget *parent) :
    QTableWidget(parent),
    mDevice(nullptr)
{
    setMinimumWidth(412);
    setDragEnabled(true);
    connect(this, SIGNAL(cellChanged(int,int)), SLOT(onCellChanged(int,int)));
    connect(this, SIGNAL(cellDoubleClicked(int,int)), SLOT(onCellDblClick(int,int)));
}

void ObjTable::setDevice(ObjnetDevice *dev)
{
    mDevice = dev;

    clear();
    setRowCount(0);

    if (mDevice)
    {
        int cnt = dev->objectCount();
        setColumnCount(4);
        setRowCount(cnt);
        setColumnWidth(0, 120);
        setColumnWidth(1, 120);
        setColumnWidth(2, 80);
        setColumnWidth(3, 50);
        QStringList objtablecolumns;
        objtablecolumns << "name" << "value" << "type" << "flags";
        setHorizontalHeaderLabels(objtablecolumns);

        blockSignals(true);
        for (int i=0; i<cnt; i++)
        {
            ObjectInfo *info = dev->objectInfo(i);
            if (!info)
                continue;

            setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i)));

            QString name = info->name();
            setItem(i, 1, new QTableWidgetItem("n/a"));
            QString wt = QMetaType::typeName(info->wType());
            if (wt == "QString")
                wt = "string";
            else if (wt == "QByteArray")
                wt = "common";
            if (info->wCount() > 1)
                wt += "[" + QString::number(info->wCount()) + "]";
            QString rt = QMetaType::typeName(info->rType());
            if (rt == "QString")
                rt = "string";
            else if (rt == "QByteArray")
                rt = "common";
            if (info->rCount() > 1)
                rt += "[" + QString::number(info->rCount()) + "]";
            unsigned char fla = info->flags();
            QString typnam;


            if (fla & ObjectInfo::Function)
            {
                typnam = wt + "(" + rt + ")";
                setItem(i, 0, new QTableWidgetItem(name));
                QPushButton *btn = new QPushButton(name);
                setCellWidget(i, 0, btn);
                if (fla & ObjectInfo::Read) // na samom dele Write, no tut naoborot)
                    connect(btn, &QPushButton::clicked, [dev, name](){dev->sendObject(name);});
                else
                    connect(btn, &QPushButton::clicked, [dev, name](){dev->requestObject(name);});
            }
            else
            {
                if (fla & ObjectInfo::Dual)
                    typnam = "r:"+wt+"/w:"+rt;
                else if (fla & ObjectInfo::Read)
                    typnam = rt;
                else if (fla & ObjectInfo::Write)
                    typnam = wt;

                setItem(i, 0, new QTableWidgetItem(name));
                dev->requestObject(name);
            }

            setItem(i, 2, new QTableWidgetItem(typnam));
            QString flags = "-fdhsrwv";
            for (int j=0; j<8; j++)
                if (!(fla & (1<<j)))
                    flags[7-j] = '-';
            setItem(i, 3, new QTableWidgetItem(flags));

            item(i, 0)->setData(Qt::UserRole, i);
        }
        blockSignals(false);

//        setContextMenuPolicy(Qt::CustomContextMenu);
//        connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onObjectMenu(QPoint)));
    }
}

void ObjTable::updateObject(QString name, QVariant value)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    if (dev != mDevice)
        return;

    for (int i=0; i<rowCount(); i++)
    {
        if (!item(i, 0))
            continue;
        QString nm = item(i, 0)->text();
        if (nm == name)
        {
            QVariantList list = value.toList();

            QString val;
            if (list.isEmpty())
                val = valueToString(value);
            else
            {
                for (int j=0; j<list.size(); j++)
                {
                    if (j)
                        val += "; ";
                    val += valueToString(list[j]);
                }
            }


            blockSignals(true);
            item(i, 1)->setText(val);
//            if (mLogs.contains(nm))
//                mLogs[nm]->append(val);
//            if (item(i, 0)->data(Qt::UserRole+4).toBool())
//            {
//                mGraph->addPoint(nm, value.toFloat());
//            }
            blockSignals(false);
            break;
        }
    }
}

QString ObjTable::valueToString(QVariant value)
{
    QString val;
#undef ByteArray
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
        if (static_cast<QMetaType::Type>(value.type()) == QMetaType::UChar)
            value = value.toInt();
        if (static_cast<QMetaType::Type>(value.type()) == QMetaType::SChar)
            value = value.toInt();
        val = value.toString();
    }
    return val;
}

void ObjTable::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    if (!mDevice)
        return;

    selectRow(currentRow());
    QTableWidgetItem *it = item(currentRow(), 0);
    int idx = it->data(Qt::UserRole).toInt();

    QString mimetype = "application/x-onb-object";
    if (!mDevice->objectInfo(idx)->isWritable()) // inverted interpretation of read/write again
        mimetype = "application/x-onb-nonreadable";
    else if (mDevice->objectInfo(idx)->wType() == ObjectInfo::Void)
        mimetype = "application/x-onb-object-void";
    else if (mDevice->objectInfo(idx)->wType() == ObjectInfo::Common)
        mimetype = "application/x-onb-object-common";
    else if (mDevice->objectInfo(idx)->wType() == ObjectInfo::String)
        mimetype = "application/x-onb-object-string";

    QByteArray ba = QByteArray::number((unsigned int)mDevice->serial(), 16) + "." + it->text().toLocal8Bit();
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(mimetype, ba);
    mimeData->setData("application/x-onb-object-name", it->text().toLocal8Bit());

    QByteArray devptr(reinterpret_cast<const char*>(&mDevice), 4);
    mimeData->setData("application/x-onb-device", devptr);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
//    drag->setHotSpot();
//    drag->setPixmap(pixmap);
    drag->exec();
}

void ObjTable::onCellChanged(int row, int col)
{
    if (col != 1)
        return;
    if (mDevice)
    {
        int idx = item(row, 0)->data(Qt::UserRole).toInt();
        ObjectInfo *info = mDevice->objectInfo(idx);
        QVariant val = item(row, col)->text();
        if (info->rType() == ObjectInfo::Common)
            val = QByteArray::fromHex(val.toByteArray());
        else
            val.convert(info->rType());
        info->fromVariant(val);
        if (info->flags() & ObjectInfo::Function)
            return;
        mDevice->sendObject(info->name());
    }
}

void ObjTable::onCellDblClick(int row, int col)
{

}