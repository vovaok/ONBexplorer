#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    upg(0L),
    sent(0),
    received(0),
    device(0L),
    lastDeviceId(0)
{

    std::string str = "preved";


    ui->setupUi(this);
    setWindowTitle("ONB Explorer");
//    resize(800, 500);

    uartWidget = new SerialPortWidget();
    uartWidget->autoConnect("ONB");
    uartWidget->disableAutoRead();
    uartWidget->setBaudrate(1000000); // means nothing for VCP
    ui->mainToolBar->addWidget(uartWidget);

//    uart = new QSerialPort();
//    uart->setBaudRate(1000000);
//    uart->setParity(QSerialPort::EvenParity);
//    uart->setStopBits(QSerialPort::OneStop);
//    uart->setFlowControl(QSerialPort::NoFlowControl);

    onbvs = new ObjnetVirtualServer(this);
    connect(onbvs, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
    connect(onbvs, SIGNAL(message(QString)), SLOT(logMessage(QString)));


//    can = new SerialCan(uart, SerialCan::protoCommon);
//    //can->setBaudrate(1000000);
//    //can->setFixedWidth(260);
//    connect(can, SIGNAL(onMessage(ulong,QByteArray&)), this, SLOT(onMessage(ulong,QByteArray&)));
//    connect(can, SIGNAL(onMessageSent(ulong,QByteArray&)), this, SLOT(onMessageSent(ulong,QByteArray&)));
//    connect(can, SIGNAL(connected()), this, SLOT(onBoardConnect()));
//    connect(can, SIGNAL(disconnected()), this, SLOT(onBoardDisconnect()));
//    //ui->mainToolBar->addWidget(can);

    SerialOnbInterface *serialOnb = new SerialOnbInterface(uartWidget->device());
    connect(serialOnb, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
    serialMaster = new ObjnetMaster(serialOnb);

    connect(serialMaster, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(serialMaster, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(serialMaster, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(serialMaster, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(serialMaster, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));
    connect(serialMaster, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

    UsbHidOnbInterface *usbonb = new UsbHidOnbInterface(new UsbOnbThread(this));
    connect(usbonb, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
    usbMaster = new ObjnetMaster(usbonb);

    connect(usbMaster, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(usbMaster, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(usbMaster, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(usbMaster, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(usbMaster, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));
    connect(usbMaster, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

    onbvi = new ObjnetVirtualInterface("main", "192.168.1.1");
//    onbvi = new ObjnetVirtualInterface("main", "127.0.0.1");
    oviMaster = new ObjnetMaster(onbvi);
    oviMaster->setName("main");
//    onbvi->setActive(true);

    connect(oviMaster, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(oviMaster, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(oviMaster, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(oviMaster, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(oviMaster, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));
    connect(oviMaster, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));



    mOviServerBtn = new QPushButton("ONB server");
    mOviServerBtn->setCheckable(true);
    connect(mOviServerBtn, SIGNAL(clicked(bool)), onbvs, SLOT(setEnabled(bool)));
    ui->mainToolBar->addWidget(mOviServerBtn);

    mLogEnableBtn = new QPushButton("Enable log");
    mLogEnableBtn->setCheckable(true);
    ui->mainToolBar->addWidget(mLogEnableBtn);

    btnResetStat = new QPushButton("Reset stat");
    btnResetStat->setFixedWidth(80);
    connect(btnResetStat, SIGNAL(clicked()), this, SLOT(resetStat()));
    ui->mainToolBar->addWidget(btnResetStat);

    editLog = new QTextEdit();
    editLog->setMinimumWidth(200);
//    editLog->setFontFamily("Courier New");
//    editLog->setReadOnly(true);

    chkSvcOnly = new QCheckBox("Svc msg only");
    chkSuppressPolling = new QCheckBox("Suppress poll msg");

    chkSvcOnly->setChecked(true);
    chkSuppressPolling->setChecked(true);

    QGroupBox *logBox = new QGroupBox("Log");
    QVBoxLayout *logvlay = new QVBoxLayout;
    logBox->setLayout(logvlay);
    QHBoxLayout *loghlay = new QHBoxLayout;
    logvlay->addLayout(loghlay);
    logvlay->addWidget(editLog);
    loghlay->addWidget(chkSvcOnly);
    loghlay->addWidget(chkSuppressPolling);
    loghlay->addStretch(1);


    mTree = new QTreeWidget();
    mTree->setMinimumWidth(250);
    mTree->setMinimumHeight(100);
    mTree->setAnimated(true);
    mTree->setColumnCount(4);
    QStringList labels;
    labels << "name" << "mac" << "addr" << "class";
    mTree->setHeaderLabels(labels);
    mTree->setColumnWidth(0, 100);
    mTree->setColumnWidth(1, 40);
    mTree->setColumnWidth(2, 40);
    mTree->setColumnWidth(3, 70);
    mTree->setIndentation(10);
    connect(mTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(onItemClick(QTreeWidgetItem*,int)));
    mTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mTree, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onDeviceMenu(QPoint)));

    mInfoBox = new QGroupBox("Device info");
    QFormLayout *gblay = new QFormLayout;
    QStringList editnames;
    editnames << "Net address" << "Class ID" << "Name" << "Full name" << "Serial" << "Version" << "Build date" << "CPU info" << "Burn count" << "Bus type" << "Bus address";
    foreach (QString s, editnames)
    {
        QLineEdit *ed = new QLineEdit;
        if (s == "Name")
        {
            connect(ed, &QLineEdit::editingFinished, [=]()
            {
                if (device && (ed->text().trimmed() != device->name().trimmed()))
                    device->changeName(ed->text());
            });
        }
        else if (s == "Full name")
        {
            connect(ed, &QLineEdit::editingFinished, [=]()
            {
                if (device && (ed->text().trimmed() != device->fullName().trimmed()))
                    device->changeFullName(ed->text());
            });
        }
        else if (s == "Bus address")
        {
            connect(ed, &QLineEdit::editingFinished, [=]()
            {
                if (device)
                {
                    int mac = ed->text().toInt();
                    if (mac <= 0 || mac > 15)
                        ed->setText(QString::number(device->localBusAddress()));
                    else if (device->localBusAddress() != mac)
                        device->changeBusAddress(mac);
                }
            });
        }
        else
        {
            ed->setReadOnly(true);
        }
        ed->setMinimumWidth(210);
        mEdits[s] = ed;
        gblay->addRow(s, ed);
    }
    mInfoBox->setLayout(gblay);

    mObjTable = new ObjTable();

    QStringList strings;
    strings << "<Usb>" << "0" << "0" << "FFFFFFFF";
    QTreeWidgetItem *item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);
    strings[0] = "<WiFi>";
    item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);
    strings[0] = "<Serial>";
    item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);

    status = new QLabel("");
    ui->statusBar->addWidget(status);
    status2 = new QLabel();
    ui->statusBar->addWidget(status2);
    status3 = new QLabel();
    ui->statusBar->addWidget(status3);

    mGraph = new GraphWidget(this);
    connect(mGraph, SIGNAL(periodChanged(unsigned long,QString,int)), SLOT(setAutoRequestPeriod(unsigned long,QString,int)));
    QGroupBox *graphBox = new QGroupBox("Graph");
    graphBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    QVBoxLayout *graphLay = new QVBoxLayout;
    graphLay->addWidget(mGraph);
    graphBox->setLayout(graphLay);


    QGridLayout *glay = new QGridLayout;
    glay->setColumnStretch(1, 1);
    glay->addWidget(mTree, 0, 0);
    glay->addWidget(mInfoBox, 1, 0);
    glay->addWidget(logBox, 2, 0, 1, 2);
    glay->addWidget(graphBox, 0, 1, 2, 1);
    glay->addWidget(mObjTable, 0, 2, 3, 1);
    ui->centralWidget->setLayout(glay);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(100);

    mEtimer.start();


    mOviServerBtn->setChecked(true);
    onbvs->setEnabled(true);

    QLineEdit *cedit = new QLineEdit();
    QPushButton *ubtn = new QPushButton("Upgrade");
    connect(ubtn, &QPushButton::clicked, [=](bool)
    {
        unsigned long cid = cedit->text().toInt(nullptr, 16);
        this->upgrade(usbMaster, cid);
    });
    ui->mainToolBar->addWidget(cedit);
    ui->mainToolBar->addWidget(ubtn);

    mLogEnableBtn->setChecked(true);

//    QPushButton *b = new QPushButton("upgrade");
//    connect(b, SIGNAL(clicked(bool)), SLOT(upgrade()));
//    ui->mainToolBar->addWidget(b);
}

MainWindow::~MainWindow()
{
    delete ui;
}
//---------------------------------------------------------------------------

void MainWindow::onMessage(ulong id, QByteArray &data)
{
    QString strid;
    strid.sprintf("%08X", (unsigned int)id);
    QString strdata;
    for (int i=0; i<data.size(); i++)
    {
        QString s;
        unsigned char byte = data[i];
        s.sprintf("%02X ", byte);
        strdata += s;
    }
    logMessage(id, data);
}

void MainWindow::onMessageSent(ulong id, QByteArray &data)
{
//    if (!can->isActive())
//        return;
//    QString strid;
//    strid.sprintf("%08X", (unsigned int)id);
//    QString strdata;
//    for (int i=0; i<data.size(); i++)
//    {
//        QString s;
//        unsigned char byte = data[i];
//        s.sprintf("%02X ", byte);
//        strdata += s;
//    }
//    //editId->setText(strid);
//    //editData->setText(strdata);
//    logMessage(id, data, 1);
}
//---------------------------------------------------------------------------

void MainWindow::logMessage(QString message)
{
    editLog->append("<code>" + message + "</code>");
}

void MainWindow::logMessage(ulong id, QByteArray &data, bool dir)
{
    QString strid;
    strid.sprintf("%08X", (unsigned int)id);
    QString strdata;
    for (int i=0; i<data.size(); i++)
    {
        QString s;
        unsigned char byte = data[i];
        s.sprintf("%02X ", byte);
        strdata += s;
    }
    QString strdir = dir? " &lt;&lt; ": " >> ";
    QString text = strid + strdir + strdata;
    if (dir)
        text = "<font color=blue>" + text + "</font>";
    editLog->append(text);

    if (dir)
        sent++;
    else
        received++;
    QString srtext;
    srtext.sprintf("Sent=%d, received=%d", sent, received);
    status2->setText(srtext);
}

void MainWindow::logMessage(QString netname, CommonMessage &msg)
{
    if (!mLogEnableBtn->isChecked())
        return;

    bool isSvc = msg.rawId() & 0x00800000;
    if (chkSvcOnly->isChecked() && !isSvc)
        return;

    if (chkSuppressPolling->isChecked() && (msg.rawId() == 0x00800000 || msg.localId().oid == svcEcho))
        return;

    QString text;
    text.sprintf("[%d] ", static_cast<int>(mEtimer.elapsed()));
    text += netname + ": ";
    QString sid = QString().sprintf("%08X", (unsigned int)msg.rawId());
    QString sdata = ba2str(msg.data());
    if (msg.isGlobal())
    {
        text += "<b>" + sid + "</b>";
        if (!msg.data().isEmpty())
            text += " : " + ba2str(msg.data());
        if (msg.localId().mac == 0)
            text = "<font color=red>" + text + "</font>";
        else
            text = "<font color=blue>" + text + "</font>";
    }
    else
    {
        text += sid;
        text += " >> ";
        text += sdata;
        if (msg.localId().mac != 0)
            text = "<font color=red>" + text + "</font>";
        else
            text = "<font color=blue>" + text + "</font>";
    }
    editLog->append(text);
}

QString MainWindow::ba2str(const QByteArray &ba)
{
    QString res;
    for (int i=0; i<ba.size(); i++)
    {
        QString s;
        unsigned char byte = ba[i];
        s.sprintf("%02X ", byte);
        res += s;
    }
    return res;
}
//---------------------------------------------------------------------------

int MainWindow::getRootId(ObjnetMaster *mas)
{
    if (mas == usbMaster)
        return 0;
    else if (mas == oviMaster)
        return 1;
    else if (mas == serialMaster)
        return 2;
    else
        return -1;
}

ObjnetMaster *MainWindow::getMasterOfItem(QTreeWidgetItem *item)
{
    while (item->parent())
        item = item->parent();
    int idx = mTree->indexOfTopLevelItem(item);
    switch (idx)
    {
        case 0: return usbMaster;
        case 1: return oviMaster;
        case 2: return serialMaster;
    }
    return nullptr;
}
//---------------------------------------------------------------------------

void MainWindow::onItemClick(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    ObjnetMaster *master = getMasterOfItem(item);

    QVariant v = item->data(0, Qt::UserRole);
    ObjnetDevice *dev = reinterpret_cast<ObjnetDevice*>(v.toInt());
    if (dev)
    {
        device = dev;
        lastDeviceId = device->serial();

        if (!dev->isInfoValid())
        {
            //qDebug() << "request" << dev->netAddress();
            master->requestDevInfo(dev->netAddress());
            master->requestObjInfo(dev->netAddress());
        }

        mInfoBox->setTitle("Device info: " + dev->name());
        mEdits["Net address"]->setText(QString::number(dev->netAddress()));
        mEdits["Class ID"]->setText(QString().sprintf("0x%08X", (unsigned int)dev->classId()));
        mEdits["Name"]->setText(dev->name());
        mEdits["Full name"]->setText(dev->fullName());
        mEdits["Serial"]->setText(QString().sprintf("0x%08X", (unsigned int)dev->serial()));
        mEdits["Version"]->setText(dev->versionString());
        mEdits["Build date"]->setText(dev->buildDate());
        mEdits["CPU info"]->setText(dev->cpuInfo());
        mEdits["Burn count"]->setText(QString().sprintf("%d", dev->burnCount()));
        mEdits["Bus type"]->setText(dev->busTypeName());
        mEdits["Bus address"]->setText(QString::number(dev->localBusAddress()));

        for (int i=0; i<dev->objectCount(); i++)
        {
            ObjectInfo *info = dev->objectInfo(i);
            if (!info)
                continue;
            if (info->isVolatile())
            {
                dev->autoRequest(info->name(), -1); // request autoRequest
            }
        }
    }
    else
    {
        device = nullptr;
        lastDeviceId = 0;
        mInfoBox->setTitle("Device info");
        foreach (QLineEdit *ed, mEdits)
            ed->clear();
    }

    mObjTable->setDevice(device);
}

//void MainWindow::onCellChanged(int row, int col)
//{
//    if (col != 1)
//        return;
//    if (device)
//    {
//        ObjectInfo *info = device->objectInfo(row);
//        QVariant val = mObjTable->item(row, col)->text();
//        if (info->rType() == ObjectInfo::Common)
//            val = QByteArray::fromHex(val.toByteArray());
//        else
//            val.convert(info->rType());
//        info->fromVariant(val);
//        if (info->flags() & ObjectInfo::Function)
//            return;
//        device->sendObject(info->name());
//    }
//}

//void MainWindow::onCellDblClick(int row, int col)
//{
//    Q_UNUSED(col);
//    QTableWidgetItem *item = mObjTable->item(row, 0);
//    if (mLogs.contains(item->text()))
//    {
//        mLogs[item->text()]->parentWidget()->show();
//        return;
//    }
////    if (item->data(Qt::UserRole+3).isValid())
////        return;

//    QWidget *dlg = new QWidget(this, Qt::Tool);
//    dlg->setWindowTitle(item->text());
//    QTextEdit *log = new QTextEdit(dlg);
//    QVBoxLayout *lay = new QVBoxLayout();
//    lay->addWidget(log);
//    dlg->setLayout(lay);
//    dlg->show();
//    //item->setData(Qt::UserRole + 3, QVariant().fromValue<QTextEdit*>(log));
//    mLogs[item->text()] = log;
//}

//void MainWindow::onObjectReceive(QString name, QVariant value)
//{
//    ObjnetDevice *dev = dynamic_cast<ObjnetDevice*>(sender());

//    if (name == "App::incrementTest")
//    {
//        static float oldt = -1;
//        static float tf = 0;
//        float t1 = mEtimer.nsecsElapsed() * 1e-6;
//        float dt = t1 - oldt;
//        if (dt > 1000)
//            dt = 5;
//        if (oldt < 0)
//        {
//            tf = dt;
//        }
//        else
//        {
//            tf = 0.999*tf + 0.001*dt;
//            static int oval = 0;
//            static int errs = 0;
//            QString str = value.toString();
//            int idx = str.indexOf(' ');
//            int val = str.left(idx).toInt();
//            if (oval != val)
//            {
//                errs++;
//                oval = val;
//            }
//            oval++;
//            status3->setText(QString().sprintf("dt= %.2f ms, errs=%d", tf, errs));
//        }
//        oldt = t1;
//    }

//}
//---------------------------------------------------------------------------

void MainWindow::onBoardConnect()
{
    status->setText("Connected");
    QStringList strings;
    strings << "<root>" << "0" << "0" << "FFFFFFFF";
    QTreeWidgetItem *item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);
}

void MainWindow::onBoardDisconnect()
{
    status->setText("Disconnected");
//    master->reset();
    mTree->clear();
    mItems.clear();
}
//---------------------------------------------------------------------------

void MainWindow::resetStat()
{
    sent = 0;
    received = 0;
    editLog->clear();
}

void MainWindow::onTimer()
{
    if (device)
    {
        if (device->busType() == BusSwonb)
        {
            QStringList names;
            for (int i=0; i<device->objectCount(); i++)
            {
                ObjectInfo *info = device->objectInfo(i);
                if (!info)
                    continue;
                if (info->flags() & ObjectInfo::Volatile)
                {
                    names << info->name();
                }
            }
//            device->groupedRequest(names.toVector().toStdVector());
            foreach (QString name, names)
                device->requestObject(name);
        }
        setWindowTitle(device->fullName());
     }


//         panel3d->updateGL();
//static float time =0;
//         mGraph->addPoint("angle1", time, tempEnc1[4]);
//          mGraph->addPoint("angle2", time, tempEnc1[5]);
//           mGraph->addPoint("angle3", time, tempEnc1[6]);
//            mGraph->addPoint("angle4", time, tempEnc1[7]);

//            time+=0.03;
}
//---------------------------------------------------------------------------

void MainWindow::onDevAdded(unsigned char netAddress, const QByteArray &locData)
{
    ObjnetMaster *master = qobject_cast<ObjnetMaster*>(sender());

    logMessage("device added: netAddress="+QString().sprintf("0x%02X", netAddress)+"; loc = "+ba2str(locData));

    int rootId = getRootId(master);
    if (rootId < 0)
        return;

    QTreeWidgetItem *parent = mTree->topLevelItem(rootId);

    for (int i=locData.size()-1; i>0; --i)
    {
        int mac = locData[0];
        int netaddr = locData[i];
        bool found = false;
        for (int j=0; j<parent->childCount(); j++)
        {
            if (parent->child(j)->text(2).toInt() == netaddr)
            {
                parent = parent->child(j);
                found = true;
                if (i == 1)
                {
                    int netaddr = parent->text(2).toInt();
                    mItems.remove(rootId+netaddr);
                    parent->setText(2, QString::number(netAddress));
                    mItems[rootId+netAddress] = parent;
                }
                break;
            }
        }
        if (!found)
        {
            QStringList strings;
            strings << ("node"+QString::number(netAddress)) << QString::number(mac);
            //if (i == 1)
                strings << QString::number(netAddress);
            QTreeWidgetItem *item = new QTreeWidgetItem(strings);

            ObjnetDevice *dev = master->device(netAddress);
            if (!dev)
            {
                qDebug() << "netAddress" << netAddress << "does not exist";
                return;
            }
//            connect(dev, SIGNAL(objectReceived(QString,QVariant)), SLOT(onObjectReceive(QString,QVariant)));
            connect(dev, SIGNAL(objectReceived(QString,QVariant)), mObjTable, SLOT(updateObject(QString,QVariant)));
            connect(dev, SIGNAL(objectReceived(QString,QVariant)), mGraph, SLOT(updateObject(QString,QVariant)));
            connect(dev, SIGNAL(timedObjectReceived(QString,unsigned long,QVariant)), mGraph, SLOT(updateTimedObject(QString,unsigned long,QVariant)));
            connect(dev, SIGNAL(autoRequestAccepted(QString,int)), SLOT(onAutoRequestAccepted(QString,int)));
            connect(dev, SIGNAL(autoRequestAccepted(QString,int)), mGraph, SLOT(onAutoRequestAccepted(QString,int)));
            connect(dev, SIGNAL(ready()), SLOT(onDevReady()));
            connect(dev, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

            int ptr = reinterpret_cast<int>(dev);
            item->setData(0, Qt::UserRole, ptr);
            item->setData(0, Qt::UserRole+1, rootId);
            parent->addChild(item);
            mItems[rootId+netAddress] = item;
            item->setExpanded(true);
            item->setDisabled(true);
        }
        else
        {
            prepareDevice(master->device(netAddress));
        }
    }

//    if (parent != mTree->topLevelItem(0))
//    {
//        master->requestClassId(netAddress);
//        master->requestName(netAddress);
//    }
}

void MainWindow::onDevConnected(unsigned char netAddress)
{
    ObjnetMaster *master = qobject_cast<ObjnetMaster*>(sender());
    int rootId = getRootId(master);
    if (rootId < 0)
        return;

    logMessage("device connected: netAddress="+QString().sprintf("0x%02X", netAddress));
    QTreeWidgetItem *item = mItems.value(rootId+netAddress, 0L);
    if (item)
    {
        item->setDisabled(false);
        const ObjnetDevice *dev2 = master->devices().at(netAddress);
        if (!dev2->isValid())
        {
            master->requestClassId(netAddress);
            master->requestName(netAddress);
            master->requestSerial(netAddress);
        }
    }
}

void MainWindow::onDevDisconnected(unsigned char netAddress)
{
    ObjnetMaster *master = qobject_cast<ObjnetMaster*>(sender());
    int rootId = getRootId(master);
    if (rootId < 0)
        return;

    logMessage("device disconnected: netAddress="+QString().sprintf("0x%02X", netAddress));
    QTreeWidgetItem *item = mItems.value(rootId+netAddress, 0L);
    if (item)
    {
        item->setDisabled(true);
    }
}

void MainWindow::onDevRemoved(unsigned char netAddress)
{
    ObjnetMaster *master = qobject_cast<ObjnetMaster*>(sender());
    int rootId = getRootId(master);
    if (rootId < 0)
        return;

    if (master->device(netAddress) == device)
        device = 0L;

    logMessage("device removed: netAddress="+QString().sprintf("0x%02X", netAddress));

    QTreeWidgetItem *item = mItems.value(rootId+netAddress, 0L);
    if (item)
    {
        item->parent()->removeChild(item);
    }
    mItems.remove(rootId+netAddress);
}
//---------------------------------------------------------------------------

void MainWindow::onServiceMessageAccepted(unsigned char netAddress, SvcOID oid, const QByteArray &data)
{
    ObjnetMaster *master = qobject_cast<ObjnetMaster*>(sender());
    int rootId = getRootId(master);
    if (rootId < 0)
        return;

    QTreeWidgetItem *item = mItems.value(rootId+netAddress, 0L);
    if (item)
    {
        switch (oid)
        {
          case svcClass:
          {
            unsigned long classId = *reinterpret_cast<const unsigned long*>(data.data());
            item->setText(3, QString().sprintf("%08X", (unsigned int)classId));
            break;
          }

          case svcName:
            item->setText(0, data);
            break;

          case svcSerial:
          {
            unsigned long serial = *reinterpret_cast<const unsigned long*>(data.data());
            if (serial == lastDeviceId)
            {
                mTree->setCurrentItem(item);
                onItemClick(item, 0);
            }
            break;
          }

          default:;
        }
    }
}

void MainWindow::onAutoRequestAccepted(QString objname, int periodMs)
{
    ObjnetDevice *dev = qobject_cast<ObjnetDevice*>(sender());
    if (dev)
    {
        if (!periodMs)
        {
            dev->autoRequest(objname, 100);
            logMessage("<i>auto request of \"" + dev->name() + "." + objname + "\"</i>");
        }
    }
}

void MainWindow::onGlobalMessage(unsigned char aid)
{
    ObjnetDevice *dev = qobject_cast<ObjnetDevice*>(sender());
    ObjnetMaster *mas = qobject_cast<ObjnetMaster*>(sender());
    QString said = QString::number((int)aid);
    QString text = "Global message: " + said;
    if (dev)
        text = "Global message from " + dev->name() + ": " + said;
    else if (mas == usbMaster)
        text = "Global message on <Usb>: " + said;
    else if (mas == oviMaster)
        text = "Global message on <WiFi>: " + said;
    else if (mas == serialMaster)
        text = "Global message on <Serial>: " + said;
    logMessage(text.toHtmlEscaped());
}
//---------------------------------------------------------------------------

void MainWindow::upgrade(ObjnetMaster *master, unsigned long classId)
{
    if (master)
    {
        QStringList result;
        QStringList diritfilt;
        diritfilt << "*.bin";
        QDirIterator dirit("D:/projects/iar", diritfilt, QDir::Files, QDirIterator::Subdirectories);
        for (int i=0; i<100 && dirit.hasNext(); i++)
        {
            QString f = dirit.next();
            result << f;
            qDebug() << f;
        }

        foreach (QString fname, result)
        {
            QFile f(fname);
            f.open(QIODevice::ReadOnly);
            QByteArray fw = f.readAll();
            f.close();
            if (UpgradeWidget::checkClass(fw, classId))
            {
                if (!upg)
                {
                    upg = new UpgradeWidget(master, this);
                    connect(upg, &UpgradeWidget::destroyed, [this](){upg = 0L;});
                }
                else
                {
                    upg->show();
                }
                upg->logAppend("firmware found: "+fname);
                upg->load(fw);
                break;
            }
        }
    }
}

void MainWindow::onBindTest(int var)
{
    qDebug() << "binded method called (!!) with var = " << var;
}

void MainWindow::onDeviceMenu(QPoint p)
{
    QTreeWidgetItem *item = mTree->itemAt(p);
    if (item)
    {
        int netaddr = item->text(2).toInt();
        unsigned long cid = QString("0x"+item->text(3)).toULong(0, 16);
//        if (cid == 0xFFFFFFFF)
//            return;

        ObjnetMaster *master = getMasterOfItem(item);

        QAction *act1 = new QAction("Upgrade "+item->text(0)+" ("+QString::number(netaddr)+")", this);
        connect(act1, &QAction::triggered, [=](){qDebug() << "upgrade device with address" << netaddr;});
        QAction *act2 = new QAction("Upgrade all of "+item->text(0)+" ("+item->text(3)+")", this);
        connect(act2, &QAction::triggered, [=](){this->upgrade(master, cid);});
        QMenu menu(this);
        //menu.addAction(act1);
        menu.addAction(act2);
        menu.setWindowModality(Qt::NonModal);
        menu.exec(mTree->mapToGlobal(p));
    }
}

void MainWindow::onObjectMenu(QPoint p)
{
    QTableWidgetItem *item = mObjTable->itemAt(p);
    if (item)
    {
        item = mObjTable->item(item->row(), 0);

        QAction *graphAct;
        if (!item->data(Qt::UserRole+4).toBool())
        {
            graphAct = new QAction("Plot graph", this);
            connect(graphAct, &QAction::triggered, [=](){item->setData(Qt::UserRole+4, true); mGraph->show();});
        }
        else
        {
            graphAct = new QAction("Remove graph", this);
            connect(graphAct, &QAction::triggered, [=](){item->setData(Qt::UserRole+4, false);});
        }
        QMenu menu(this);
        menu.addAction(graphAct);
        menu.setWindowModality(Qt::NonModal);
        menu.exec(mObjTable->mapToGlobal(p));
    }
}

void MainWindow::setAutoRequestPeriod(unsigned long serial, QString objname, int period_ms)
{
    ObjnetMaster *masters[3] = {usbMaster, oviMaster, serialMaster};
    for (ObjnetMaster *master: masters)
    {
        ObjnetDevice *dev = master->deviceBySerial(serial);
        if (dev)
        {
//            dev->autoRequest(objname, period_ms);
//            logMessage("<i>auto request of \"" + dev->name() + "." + objname + "\" with period, ms:"+QString::number(period_ms)+"</i>");
            dev->timedRequest(objname, period_ms);
            logMessage("<i>auto request of \"" + dev->name() + "." + objname + "\" with period, ms:"+QString::number(period_ms)+"</i>");
            return;
        }
    }
}
//---------------------------------------------------------------------------

void MainWindow::onDevReady()
{
    ObjnetDevice *dev = qobject_cast<ObjnetDevice*>(sender());
    logMessage("device ready: " + dev->name());

    prepareDevice(dev);

    // white: 255 128 40
}

void MainWindow::prepareDevice(ObjnetDevice *dev)
{
    QTreeWidgetItem *item = mTree->currentItem();
    if (item)
    {
        QVariant v = item->data(0, Qt::UserRole);
        ObjnetDevice *itemdev = reinterpret_cast<ObjnetDevice*>(v.toInt());
        if (itemdev == dev)
            onItemClick(item, 0);
    }

//    for (int i=0; i<dev->objectCount(); i++)
//    {
//        ObjectInfo *info = dev->objectInfo(i);
//        if (!info)
//            continue;
//        if (info->isVolatile())
//        {
//            dev->autoRequest(info->name(), 100);
//            logMessage("<i>auto request of \"" + info->name() + "\"</i>");
//        }
//    }
}
//---------------------------------------------------------------------------
