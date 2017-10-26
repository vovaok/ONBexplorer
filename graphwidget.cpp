#include "graphwidget.h"

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent)
{
    setAcceptDrops(true);

    mScene = new QPanel3D(this);
    mScene->setMinimumSize(320, 240);
    mScene->setDisabled(true);
    mGraph = new Graph2D(mScene->root());
    setupScene();

    mClearBtn = new QPushButton("Clear");
//    mClearBtn->setFixedWidth(100);
    connect(mClearBtn, &QPushButton::clicked, [=](){mGraph->clear(); mTimer.restart();});

    mNamesLay = new QFormLayout;
//    mNamesLay->addRow("name", new QLabel("period"));

    QSpinBox *pointLimitSpin = new QSpinBox();
    pointLimitSpin->setRange(0, 10000);
    pointLimitSpin->setSingleStep(100);
    connect(pointLimitSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val){mGraph->setPointLimit(val);});

    QGridLayout *lay = new QGridLayout;
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(mNamesLay, 0, 0);
    lay->addWidget(mScene, 0, 1, 2, 1);
    QGridLayout *vlay = new QGridLayout;
    vlay->addWidget(new QLabel("Point limit:"), 0, 0);
    vlay->addWidget(pointLimitSpin, 0, 1);
    vlay->addWidget(mClearBtn, 1, 0, 1, 2);
    lay->addLayout(vlay, 1, 0);
    lay->setColumnStretch(1, 1);
    lay->setRowStretch(0, 1);
    setLayout(lay);

    QTimer *drawTimer = new QTimer(this);
    connect(drawTimer, SIGNAL(timeout()), mScene, SLOT(updateGL()));
    drawTimer->start(16);
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
    if (event->mimeData()->hasFormat("application/x-onb-device"))
    {
//        QByteArray ba = event->mimeData()->data("application/x-onb-object");
//        if (mGraphNames.contains(ba))
//            return;
//        mGraphNames[ba] = true;

        QByteArray devptr = event->mimeData()->data("application/x-onb-device");
        ObjnetDevice *dev = *reinterpret_cast<ObjnetDevice**>(devptr.data());
        int ser = dev->serial();
        if (!mDevices.contains(ser))
        {
            mDevices[ser] = dev->name() + "[" + QString::number(dev->busAddress()) + "]";
            QLabel *devname = new QLabel(mDevices[ser]);
            devname->setProperty("serial", (unsigned int)ser);
            mNamesLay->addRow(devname, new QLabel(QString().sprintf("(%08X)", (unsigned int)ser)));
        }

        QByteArray objname = event->mimeData()->data("application/x-onb-object-name");
        if (!mVarNames[ser].contains(objname))
        {
            mVarNames[ser] << objname;
            addObjname(ser, objname);
            event->acceptProposedAction();
            return;
        }
    }

    event->ignore();
}

void GraphWidget::addObjname(unsigned long serial, QString objname)
{
    QSpinBox *spin = new QSpinBox();
    spin->setRange(5, 1000);
    spin->setValue(100);
    spin->setFixedWidth(60);
    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int val){emit periodChanged(serial, objname, val);});

    QPushButton *removeBtn = new QPushButton("×");
    removeBtn->setFixedSize(16, 16);

    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(spin);
    hlay->addWidget(removeBtn);

    int row = getRow(serial, objname);
    QLabel *lbl = new QLabel("    "+objname);
    lbl->setProperty("objname", objname);
    mNamesLay->insertRow(row, lbl, hlay);

    connect(removeBtn, &QPushButton::clicked, [=](){removeObjname(serial, objname);});
}

void GraphWidget::removeObjname(unsigned long serial, QString objname)
{
    int row = getRow(serial, objname);
    QLayoutItem *item = mNamesLay->itemAt(row, QFormLayout::LabelRole);
    mNamesLay->removeRow(mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!
    mVarNames[serial].removeOne(objname);

    removeGraph(mDevices[serial]+"."+objname);

    if (mVarNames[serial].isEmpty())
    {
        mVarNames.remove(serial);
        mDevices.remove(serial);
        QLayoutItem *item = mNamesLay->itemAt(row-1, QFormLayout::LabelRole);
        mNamesLay->removeRow(mNamesLay->indexOf(item->widget())); // FUCKING HACK!!!! QFormLayout removes row by its item_index instead of row number!!
    }
}

int GraphWidget::getRow(unsigned long serial, QString objname)
{
    bool devfound = false;
    int row;
    for (row=0; row<mNamesLay->rowCount(); row++)
    {
        QLabel *lbl = qobject_cast<QLabel*>(mNamesLay->itemAt(row, QFormLayout::LabelRole)->widget());
        if (lbl)
        {
            if (lbl->property("serial").toInt() == serial)
            {
                if (!devfound)
                    devfound = true;
                else
                    return row;
            }
            else if (lbl->property("objname") == objname)
            {
                return row;
            }
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
