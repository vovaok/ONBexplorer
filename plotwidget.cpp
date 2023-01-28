#include "plotwidget.h"

PlotWidget::PlotWidget(QWidget *parent) :
    QWidget(parent),
    mCurColor(0),
    mTimestamp0(0),
    mTime(0)
{
    setAcceptDrops(true);

    mColors << Qt::red << Qt::blue << Qt::black << QColor("green") << QColor(0, 192, 0) << QColor(0, 224, 224) << Qt::magenta << QColor(192, 192, 0) << Qt::darkGray;

    mGraph = new GraphWidget();
    mGraph->setMinimumSize(400, 200);
    mGraph->setMaxPointCount(65536);

    QTimer *drawTimer = new QTimer(this);
    connect(drawTimer, &QTimer::timeout, [=](){mGraph->update();});
    drawTimer->start(16);

    QPushButton *clearBtn = new QPushButton("Clear");
//    clearBtn->setFixedWidth(100);
    connect(clearBtn, &QPushButton::clicked, [=]()
    {
        mGraph->clear();
        mTimer.restart();
        mTimestamp0 = 0;
        mTrigCaptureTime = 0;
    });

    QPushButton *pauseBtn = new QPushButton("||");
    pauseBtn->setCheckable(true);
    pauseBtn->setFixedWidth(24);
    connect(pauseBtn, &QPushButton::clicked, [=](bool checked)
    {
        if (checked)
        {
            drawTimer->stop();
            mGraph->setAutoBoundsEnabled(false);
        }
        else
        {
            drawTimer->start();
            mGraph->setAutoBoundsEnabled(true);
        }
    });

    mNamesLay = new QFormLayout;

    QDoubleSpinBox *pointLimitSpin = new QDoubleSpinBox();
    pointLimitSpin->setFixedWidth(60);
    pointLimitSpin->setRange(0, 60);
    pointLimitSpin->setSingleStep(0.1);
    connect(pointLimitSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [=](double val)
    {
        mGraph->setXwindow(val);
        mTriggerWindow = val;
//        mGraph->setDataWindowWidth(val);
    });

    mOldTrigValue = 0;
    mTrigCaptureTime = 0;
    mTriggerSource = new QComboBox();
    mTriggerLevel = new QLineEdit("0");
    mTriggerLevel->setFixedWidth(40);
    QPushButton *autoTrigLvlBtn = new QPushButton("AUTO");
    autoTrigLvlBtn->setFixedWidth(40);
    connect(autoTrigLvlBtn, &QPushButton::clicked, this, &PlotWidget::evalAutoTriggerLevel);
    mTriggerOffset = new QSpinBox();
    mTriggerOffset->setFixedWidth(60);
    mTriggerOffset->setRange(-10000, 10000);
    mTriggerEdge = TriggerOff;
    QList<QPushButton *> trigButtons;
    trigButtons << new QPushButton("OFF");
    trigButtons << new QPushButton("_/");
    trigButtons << new QPushButton("\\_");
    for (int i=0; i<trigButtons.size(); i++)
    {
        QPushButton *btn = trigButtons[i];
        btn->setFixedWidth(28);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        connect(btn, &QPushButton::toggled, [=](bool checked)
        {
            if (checked)
                mTriggerEdge = static_cast<TriggerEdge>(i);
        });
    }
    connect(trigButtons[TriggerOff], &QPushButton::toggled, [=](bool checked)
    {
        mGraph->setAutoBoundsEnabled(checked);
//        mTriggerSource->setDisabled(checked);
//        mTriggerLevel->setDisabled(checked);
    });
    trigButtons[TriggerOff]->setChecked(true);

    QGridLayout *lay = new QGridLayout;
    setLayout(lay);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(mNamesLay, 0, 0);
    lay->addWidget(new QLabel(), 1, 0);
    lay->addWidget(mGraph, 0, 1, 3, 1);
    QVBoxLayout *vlay = new QVBoxLayout;
    lay->addLayout(vlay, 2, 0);
    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(new QLabel("Time window, s:"));
    hlay->addWidget(pointLimitSpin);
    vlay->addLayout(hlay);
    hlay = new QHBoxLayout;
    hlay->addWidget(clearBtn);
    hlay->addWidget(pauseBtn);
    vlay->addLayout(hlay);
    hlay = new QHBoxLayout;
    hlay->addWidget(new QLabel("Trigger:"));
    hlay->addWidget(mTriggerSource);
    hlay->addWidget(trigButtons[TriggerRising]);
    hlay->addWidget(trigButtons[TriggerFalling]);
    hlay->addWidget(trigButtons[TriggerOff]);
    vlay->addLayout(hlay);
    hlay = new QHBoxLayout;
    hlay->addWidget(new QLabel("Level:"));
    hlay->addWidget(mTriggerLevel);
//    hlay->addWidget(autoTrigLvlBtn);
    hlay->addWidget(new QLabel("Offset:"));
    hlay->addWidget(mTriggerOffset);
    vlay->addLayout(hlay);
    lay->setColumnStretch(1, 1);
    lay->setRowStretch(1, 1);
}

QColor PlotWidget::nextColor()
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

void PlotWidget::dragEnterEvent(QDragEnterEvent *event)
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
    else if (event->mimeData()->hasFormat("application/x-onb-object-list"))
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void PlotWidget::dragMoveEvent(QDragMoveEvent *event)
{
//    qDebug() << "drag move";
}

void PlotWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-onb-object"))
    {
        QByteArray objptr = event->mimeData()->data("application/x-onb-object");
        ObjectInfo *obj = *reinterpret_cast<ObjectInfo**>(objptr.data());
        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());

        regDevice(dev);

        if (regObject(dev, obj))
        {
            event->acceptProposedAction();
            return;
        }
    }

    else if (event->mimeData()->hasFormat("application/x-onb-object-list"))
    {        
        QByteArray listptr = event->mimeData()->data("application/x-onb-object-list");
        uint32_t cnt = *reinterpret_cast<uint32_t*>(listptr.data());
        ObjectInfo **objlist = reinterpret_cast<ObjectInfo**>(listptr.data() + sizeof(uint32_t));
        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());

        int ser = dev->serial();
        regDevice(dev);

        QStringList names;
        for (int i=0; i<cnt; i++)
        {
            //regObject(dev, objlist[i]);
            names << objlist[i]->name();
        }

        qDebug() << names;

        QString objname = names.join("+");
        if (names.count() == 2)
            objname = names[1] + "(" + names[0] + ")";
        QString graphname = mDevices[ser] + "." + objname;
        mDependencies[graphname] = names;

        if (!mVarNames[ser].contains(objname))
        {
            mVarNames[ser] << objname;
            addObjname(ser, objname, 0);// obj->isArray()? obj->wCount(): 0);
        }

        QTimer *tim = new QTimer(this);
        connect(tim, &QTimer::timeout, [=]()
        {
            dev->groupedRequest(names.toVector().toStdVector());
        });
        tim->start(30);

        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void PlotWidget::regDevice(ObjnetDevice *dev)
{
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
}

bool PlotWidget::regObject(ObjnetDevice *dev, ObjectInfo *obj)
{
    int ser = dev->serial();
    if (!mVarNames[ser].contains(obj->name()))
    {
        mVarNames[ser] << obj->name();
        addObjname(ser, obj->name(), obj->isArray()? obj->wCount(): 0);
        return true;
    }
    return false;
}

void PlotWidget::evalAutoTriggerLevel()
{

}

void PlotWidget::addObjname(unsigned long serial, QString objname, int childCount)
{
    int row = getRow(serial, objname);

    QCheckBox *chk = new QCheckBox(objname);
    chk->setChecked(true);
    if (!childCount)
    {
        connect(chk, &QCheckBox::toggled, [=](bool checked)
        {
            mGraph->setGraphVisible(mDevices[serial] + "." + objname, checked);
//            mGraph->resetBounds();
        });
    }
    chk->setProperty("objname", objname);
    chk->setProperty("child_count", childCount);

    QSpinBox *spin = new QSpinBox();
    spin->setRange(1, 1000);
    //spin->setValue(100);
    spin->setFixedWidth(60);
    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val){emit periodChanged(serial, objname, val);});

    QPushButton *removeBtn = new QPushButton("×");
    removeBtn->setFixedSize(16, 16);
    connect(removeBtn, &QPushButton::clicked, [=](){removeObjname(serial, objname); emit periodChanged(serial, objname, 0);}); // disable auto-request

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
        mGraph->addGraph(graphname, nextColor(), 2.0f);
        mTriggerSource->addItem(graphname);
    }

    QVector<QCheckBox *> childChecks;
    row++;
    for (int i=0; i<childCount; i++, row++)
    {
        QString itemname = objname + "[" + QString::number(i) + "]";

        QCheckBox *chk1 = new QCheckBox(itemname);
        childChecks << chk1;
        chk1->setChecked(true);
        connect(chk1, &QCheckBox::toggled, [=](bool checked)
        {
            mGraph->setGraphVisible(mDevices[serial] + "." + itemname, checked);
//            mGraph->resetBounds();
        });
        connect(chk, &QCheckBox::toggled, chk1, &QCheckBox::setChecked);
        chk1->setProperty("objname", itemname);

//        mNamesLay->addWidget(chk, row, 0);
//        mNamesLay->addWidget(lbl, row, 1);

        mNamesLay->insertRow(row, chk1, new QLabel());

        QString graphname = mDevices[serial] + "." + itemname;
        mGraph->addGraph(graphname, nextColor(), 2.0f);
        mTriggerSource->addItem(graphname);
    }

    mGraph->resetBounds();

    emit periodChanged(serial, objname, -1);
}

void PlotWidget::removeObjname(unsigned long serial, QString objname)
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

    int childCount = item->widget()->property("child_count").toInt();
    for (int i=0; i<childCount; i++)
    {
        QString itemname = objname + "[" + QString::number(i) + "]";
        removeObjname(serial, itemname);
    }

    mNamesLay->removeRow(row);//mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!

    QString graphname = mDevices[serial] + "." + objname;
    removeGraph(graphname);
    int trig_idx = mTriggerSource->findText(graphname);
    if (trig_idx >= 0)
        mTriggerSource->removeItem(trig_idx);

    if (mVarNames.contains(serial))
    {
        mVarNames[serial].removeOne(objname);

        if (mVarNames[serial].isEmpty())
        {
            mVarNames.remove(serial);
            mDevices.remove(serial);
//            QLayoutItem *item = mNamesLay->itemAt(row-1, QFormLayout::SpanningRole);
            mNamesLay->removeRow(row-1);//mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!
        }
    }
}

int PlotWidget::getRow(unsigned long serial, QString objname)
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

void PlotWidget::addPoint(QString name, float val)
{
    if (!mTimer.isValid())
        mTimer.start();

    float time = mTimer.nsecsElapsed() * 1.0e-9f;
    addPoint(name, time, val);
}

void PlotWidget::addPoint(QString name, float time, float val)
{
    mGraph->addPoint(name, time, val);

    if (name == mTriggerSource->currentText())
    {
        float level = mTriggerLevel->text().toDouble();
        float offset = mTriggerOffset->value() * 0.001f;
        float win = mTriggerWindow? mTriggerWindow: 1;

        if (time > mTrigCaptureTime + offset + win/2)
        {
            if (mTriggerEdge == TriggerRising)
            {
//                qDebug() << mOldTrigValue << level << val;
                if (mOldTrigValue < level && val >= level)
                    mTrigCaptureTime = time;
            }
            if (mTriggerEdge == TriggerFalling)
            {
                if (mOldTrigValue > level && val <= level)
                    mTrigCaptureTime = time;
            }
        }
        if (mTriggerEdge == TriggerOff)
        {
            mTrigCaptureTime = 0;
        }
        mOldTrigValue = val;

        if (mTrigCaptureTime)
        {
            mGraph->setXmin(mTrigCaptureTime + offset - win / 2);
            mGraph->setXmax(mTrigCaptureTime + offset + win / 2);
        }
    }
}

void PlotWidget::removeGraph(QString name)
{
    mGraph->removeGraph(name);
    mGraph->resetBounds();
}

void PlotWidget::updateObject(QString name, QVariant value)
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

void PlotWidget::updateObjectGroup(QVariantMap values)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    int ser = dev->serial();
    if (mDevices.contains(ser))
    {
        QString depname = values.keys().join("+");
        if (values.count() == 2)
            depname = values.keys()[1] + "(" + values.keys()[0] + ")";
        QString graphname = mDevices[ser] + "." + depname;
        QString xName;
        QString yName;
        if (mDependencies.contains(graphname))
        {
            xName = mDependencies[graphname][0];
            yName = mDependencies[graphname][1];
        }
        else if (values.count() == 2)
        {
            depname = values.keys()[0] + "(" + values.keys()[1] + ")";
            graphname = mDevices[ser] + "." + depname;
            if (mDependencies.contains(graphname))
            {
                xName = mDependencies[graphname][0];
                yName = mDependencies[graphname][1];
            }
        }

        if (values.count() == 2 && !xName.isEmpty())
        {
            float x = values[xName].toFloat();
            float y = values[yName].toFloat();
//            if (xName == "angle")
//            {
//                float phi = x;
//                float rho = y;
//                x = rho * cosf(phi);
//                y = rho * sinf(phi);
//            }
            mGraph->addPoint(graphname, x, y);
        }
    }
}

void PlotWidget::updateTimedObject(QString name, uint32_t timestamp, QVariant value)
{
    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());
    int ser = dev->serial();
    if (mDevices.contains(ser))
    {
        if (mVarNames[ser].contains(name))
        {
            if (!mTimestamp0 || timestamp < mTimestamp0)
                mTimestamp0 = timestamp;
            mTime = (timestamp - mTimestamp0) * 0.001f;

            QString graphname = mDevices[ser] + "." + name;
            QVariantList list = value.toList();
            if (list.isEmpty())
                addPoint(graphname, mTime, value.toFloat());
            else
            {
                for (int i=0; i<list.size(); i++)
                {
                    addPoint(graphname+"["+QString::number(i)+"]", mTime, list[i].toFloat());
                }
            }
        }
    }
}

void PlotWidget::onAutoRequestAccepted(QString objname, int periodMs)
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
