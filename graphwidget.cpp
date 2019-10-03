#include "graphwidget.h"

GraphWidget::GraphWidget(QWidget *parent) :
    QWidget(parent),
    mCurColor(0),
    mTimestamp0(0),
    mTime(0)
{
    setAcceptDrops(true);

    mColors << Qt::red << Qt::blue << Qt::black << QColor("green") << QColor(0, 192, 0) << QColor(0, 224, 224) << Qt::magenta << QColor(192, 192, 0) << Qt::darkGray;

    mScene = new QPanel3D(this);
    mScene->setMinimumSize(320, 240);
    mScene->setDisabled(true);
    mGraph = new Graph2D(mScene->root());
    setupScene();

    mClearBtn = new QPushButton("Clear");
//    mClearBtn->setFixedWidth(100);
    connect(mClearBtn, &QPushButton::clicked, [=](){mGraph->clear(); mTimer.restart();});

    mNamesLay = new QFormLayout;

    QSpinBox *pointLimitSpin = new QSpinBox();
    pointLimitSpin->setRange(0, 100);
    pointLimitSpin->setSingleStep(1);
    connect(pointLimitSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val){mGraph->setDataWindowWidth(val);});

    QGridLayout *lay = new QGridLayout;
    setLayout(lay);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(mNamesLay, 0, 0);
    lay->addWidget(new QLabel(), 1, 0);
    lay->addWidget(mScene, 0, 1, 3, 1);
    QGridLayout *vlay = new QGridLayout;
    lay->addLayout(vlay, 2, 0);
    vlay->addWidget(new QLabel("Time window, s:"), 0, 0);
    vlay->addWidget(pointLimitSpin, 0, 1);
    vlay->addWidget(mClearBtn, 1, 0, 1, 2);
    lay->setColumnStretch(1, 1);
    lay->setRowStretch(1, 1);


    QTimer *drawTimer = new QTimer(this);
    connect(drawTimer, SIGNAL(timeout()), mScene, SLOT(updateGL()));
    drawTimer->start(16);
}

QColor GraphWidget::nextColor()
{
    if (mCurColor >= mColors.size())
    {
        int r = rand() & 0xFF;
        int g = rand() & 0xFF;
        int b = 255 - qMax(r, g);
        mColors << QColor(r, g, b);
    }
    return mColors[mCurColor++];
}

void GraphWidget::setupScene()
{
    mScene->camera()->setPosition(QVector3D(0, 0, 10));
    mScene->camera()->setDirection(QVector3D(0, 0, -1));
    mScene->camera()->setTopDir(QVector3D(0, 1, 0));
    mScene->camera()->setOrtho(true);
    mScene->camera()->setFixedViewportSize(QSizeF(110, 110));
    mScene->camera()->setFixedViewport(true);
    mScene->setBackColor(Qt::white);
    mScene->setLightingEnabled(false);

    mGraph->setSize(100, 100);
    mGraph->setPosition(-50, -50, 0);
    mGraph->setBounds(QRectF(0, 0, 1, 0));
}

void GraphWidget::dragEnterEvent(QDragEnterEvent *event)
{
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
    else
    {
        event->ignore();
    }
}

void GraphWidget::dragMoveEvent(QDragMoveEvent *event)
{
//    qDebug() << "drag move";
}

void GraphWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-onb-object"))
    {
        QByteArray objptr = event->mimeData()->data("application/x-onb-object");
        ObjectInfo *obj = *reinterpret_cast<ObjectInfo**>(objptr.data());
        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());

        int ser = dev->serial();
        if (!mDevices.contains(ser))
        {
            mDevices[ser] = dev->name();// + "[" + QString::number(dev->busAddress()) + "]";
            QString serstr = QString().sprintf("(%08X)", (unsigned int)ser);
            QLabel *devname = new QLabel(mDevices[ser] + " " + serstr);
            devname->setProperty("serial", (unsigned int)ser);
            int row = mNamesLay->rowCount();
            mNamesLay->addRow(devname);
//            mNamesLay->addWidget(devname, row, 0, 1, 4);
        }

        if (!mVarNames[ser].contains(obj->name()))
        {
            mVarNames[ser] << obj->name();
            addObjname(ser, obj->name(), obj->isArray()? obj->wCount(): 0);
            event->acceptProposedAction();
            return;
        }
    }

    event->ignore();
}

void GraphWidget::addObjname(unsigned long serial, QString objname, int childCount)
{
    int row = getRow(serial, objname);

    QCheckBox *chk = new QCheckBox(objname);
    chk->setChecked(true);
    if (!childCount)
        connect(chk, &QCheckBox::toggled, [=](bool checked){mGraph->setVisible(mDevices[serial] + "." + objname, checked); mGraph->setBounds();});
    else
        connect(chk, &QCheckBox::toggled, [=](bool checked){});
    chk->setProperty("objname", objname);
    chk->setProperty("children", childCount);

    QSpinBox *spin = new QSpinBox();
    spin->setRange(1, 1000);
    //spin->setValue(100);
    spin->setFixedWidth(60);
    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val){emit periodChanged(serial, objname, val);});

    QPushButton *removeBtn = new QPushButton("×");
    removeBtn->setFixedSize(16, 16);
    connect(removeBtn, &QPushButton::clicked, [=](){removeObjname(serial, objname); emit periodChanged(serial, objname, 100);});

//    mNamesLay->addWidget(chk, row, 0);
//    mNamesLay->addWidget(lbl, row, 1);
//    mNamesLay->addWidget(spin, row, 2);
//    mNamesLay->addWidget(removeBtn, row, 3);

    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(spin);
    hlay->addWidget(removeBtn);
    mNamesLay->insertRow(row, chk, hlay);

    if (!childCount)
    {
        QString graphname = mDevices[serial] + "." + objname;
        mGraph->addGraph(graphname, nextColor());
    }

    row++;
    for (int i=0; i<childCount; i++, row++)
    {
        QString itemname = objname + "[" + QString::number(i) + "]";

        chk = new QCheckBox(itemname);
        chk->setChecked(true);
        connect(chk, &QCheckBox::toggled, [=](bool checked){mGraph->setVisible(mDevices[serial] + "." + itemname, checked); mGraph->setBounds();});
        chk->setProperty("objname", itemname);

//        mNamesLay->addWidget(chk, row, 0);
//        mNamesLay->addWidget(lbl, row, 1);

        mNamesLay->insertRow(row, chk, new QLabel());

        QString graphname = mDevices[serial] + "." + itemname;
        mGraph->addGraph(graphname, nextColor());
    }

    mGraph->setBounds();

    emit periodChanged(serial, objname, -1);
}

void GraphWidget::removeObjname(unsigned long serial, QString objname)
{
    int row = getRow(serial, objname);
    QLayoutItem *item = mNamesLay->itemAt(row, QFormLayout::LabelRole);
    if (!item)
        item = mNamesLay->itemAt(row, QFormLayout::SpanningRole);
    if (!item)
    {
        qDebug() << "FAIL!!! to remove objname from row" << row;
        return;
    }

    int childCount = item->widget()->property("children").toInt();
    for (int i=0; i<childCount; i++)
    {
        QString itemname = objname + "[" + QString::number(i) + "]";
        removeObjname(serial, itemname);
    }

    mNamesLay->removeRow(mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!

    removeGraph(mDevices[serial]+"."+objname);

    if (mVarNames.contains(serial))
    {
        mVarNames[serial].removeOne(objname);

        if (mVarNames[serial].isEmpty())
        {
            mVarNames.remove(serial);
            mDevices.remove(serial);
            QLayoutItem *item = mNamesLay->itemAt(row-1, QFormLayout::SpanningRole);
            mNamesLay->removeRow(mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!
        }
    }
}

int GraphWidget::getRow(unsigned long serial, QString objname)
{
    bool devfound = false;
    int row;
    for (row=0; row<mNamesLay->rowCount(); row++)
    {
        QLayoutItem *spanItem = mNamesLay->itemAt(row, QFormLayout::SpanningRole);
        QLayoutItem *labelItem = mNamesLay->itemAt(row, QFormLayout::LabelRole);
        if (spanItem && spanItem->widget())
        {
            if (spanItem->widget()->property("serial").toInt() == serial)
                devfound = true;
            else if (devfound)
                return row;
        }
        else if (devfound && labelItem && labelItem->widget())
        {
            if (labelItem->widget()->property("objname") == objname)
                return row;
        }
    }
    return row;
}

void GraphWidget::addPoint(QString name, float val)
{
    if (!mTimer.isValid())
        mTimer.start();

    float time = mTimer.nsecsElapsed() * 1.0e-9f;
    mGraph->addPoint(name, time, val);

//    mScene->updateGL();
}

void GraphWidget::removeGraph(QString name)
{
    mGraph->clear(name);
    mGraph->setBounds();
}

void GraphWidget::updateObject(QString name, QVariant value)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    int ser = dev->serial();
    if (mDevices.contains(ser))
    {
        if (mVarNames[ser].contains(name))
        {
            QString graphname = mDevices[ser] + "." + name;
            QVariantList list = value.toList();
            if (list.isEmpty())
                addPoint(graphname, value.toFloat());
            else
            {
                for (int i=0; i<list.size(); i++)
                {
                    addPoint(graphname+"["+QString::number(i)+"]", list[i].toFloat());
                }
            }
        }
    }
}

void GraphWidget::updateTimedObject(QString name, uint32_t timestamp, QVariant value)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    int ser = dev->serial();
    if (mDevices.contains(ser))
    {
        if (mVarNames[ser].contains(name))
        {
            if (!mTimestamp0)
                mTimestamp0 = timestamp;
            mTime = (timestamp - mTimestamp0) * 0.001f;

            QString graphname = mDevices[ser] + "." + name;
            QVariantList list = value.toList();
            if (list.isEmpty())
                mGraph->addPoint(graphname, mTime, value.toFloat());
            else
            {
                for (int i=0; i<list.size(); i++)
                {
                    mGraph->addPoint(graphname+"["+QString::number(i)+"]", mTime, list[i].toFloat());
                }
            }
        }
    }
}

void GraphWidget::onAutoRequestAccepted(QString objname, int periodMs)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    int ser = dev->serial();
    if (mDevices.contains(ser))
    {
        if (mVarNames[ser].contains(objname))
        {
            int row = getRow(ser, objname);
            QLayoutItem *fieldItem = mNamesLay->itemAt(row, QFormLayout::FieldRole);
            if (fieldItem)
            {
                QSpinBox *spin = qobject_cast<QSpinBox*>(fieldItem->layout()->itemAt(0)->widget());
                spin->blockSignals(true);
                if (!periodMs)
                    periodMs = 100;
                spin->setValue(periodMs);
                spin->blockSignals(false);
            }
        }
    }
}
