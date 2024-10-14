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
    ui->setupUi(this);
    setWindowTitle("ONB Explorer");
//    resize(800, 1000);

//    uart = new DonglePort(this);
//    uart->autoConnectTo("ONB");
//    uart->disableAutoRead();
//    uart->setBaudRate(1000000);

#if defined(ONB_SERIAL)
    uartWidget = new SerialPortWidget(new QSerialPort());
    uartWidget->autoConnect("ONB");
    uartWidget->disableAutoRead();
    uartWidget->setBaudrate(1000000);
    ui->mainToolBar->addWidget(uartWidget);

    SerialOnbInterface *serialOnb = new SerialOnbInterface(uartWidget->device(), false); // true = SWONB mode
//    SerialOnbInterface *serialOnb = new SerialOnbInterface(true); // true = SWONB mode
//    connect(uartWidget, &SerialPortWidget::portChanged, serialOnb, &SerialOnbInterface::setPort);
    connect(serialOnb, SIGNAL(message(QString,const CommonMessage&)), SLOT(logMessage(QString,const CommonMessage&)));
    serialMaster = new ObjnetMaster(serialOnb);
    masters << serialMaster;
#endif

#if defined(ONB_VIRTUAL)
    onbvs = new ObjnetVirtualServer(this);
    connect(onbvs, SIGNAL(message(QString,const CommonMessage&)), SLOT(logMessage(QString,const CommonMessage&)));
    connect(onbvs, SIGNAL(message(QString)), SLOT(logMessage(QString)));
    //    onbvi = new ObjnetVirtualInterface("main", "192.168.1.1");
    onbvi = new ObjnetVirtualInterface("main", "127.0.0.1");
    oviMaster = new ObjnetMaster(onbvi);
    oviMaster->setName("main");
    onbvi->setActive(true);
    masters << oviMaster;

    mOviServerBtn = new QPushButton("ONB server");
    mOviServerBtn->setCheckable(true);
    connect(mOviServerBtn, SIGNAL(clicked(bool)), onbvs, SLOT(setEnabled(bool)));
    ui->mainToolBar->addWidget(mOviServerBtn);
#endif

#if defined(ONB_USBHID)
    UsbHidOnbInterface *usbonb = new UsbHidOnbInterface(new UsbOnbThread(this));
    connect(usbonb, SIGNAL(message(QString,const CommonMessage&)), SLOT(logMessage(QString, const CommonMessage&)));
    usbMaster = new ObjnetMaster(usbonb);
    masters << usbMaster;
#endif

#if defined(ONB_UDP)
    UdpOnbInterface *udponb = new UdpOnbInterface(this);
    connect(udponb, SIGNAL(message(QString,const CommonMessage&)), SLOT(logMessage(QString,const CommonMessage&)));
    udpMaster = new ObjnetMaster(udponb);
    masters << udpMaster;
#endif

    for (ObjnetMaster *m: masters)
    {
        connect(m, &ObjnetMaster::devAdded, this, &MainWindow::onDevAdded);
        connect(m, &ObjnetMaster::devConnected, this, &MainWindow::onDevConnected);
        connect(m, &ObjnetMaster::devDisconnected, this, &MainWindow::onDevDisconnected);
        connect(m, &ObjnetMaster::devRemoved, this, &MainWindow::onDevRemoved);
        connect(m, &ObjnetMaster::serviceMessageAccepted, this, &MainWindow::onServiceMessageAccepted);
        connect(m, &ObjnetMaster::globalMessage, this, &MainWindow::onGlobalMessage);
    }

    mLogEnableBtn = new QPushButton("Enable log");
    mLogEnableBtn->setCheckable(true);
    ui->mainToolBar->addWidget(mLogEnableBtn);

    btnResetStat = new QPushButton("Reset stat");
    btnResetStat->setFixedWidth(80);
    connect(btnResetStat, SIGNAL(clicked()), this, SLOT(resetStat()));
    ui->mainToolBar->addWidget(btnResetStat);

    ObjLogger *paramLogWidget = new ObjLogger(this);

    QPushButton *btnParamLog = new QPushButton("Log params");
    ui->mainToolBar->addWidget(btnParamLog);
    connect(btnParamLog, &QPushButton::clicked, paramLogWidget, &QWidget::show);

    editLog = new QTextEdit();
    editLog->setMinimumWidth(200);
//    editLog->setFontFamily("Courier New");
//    editLog->setReadOnly(true);

    chkSvcOnly = new QCheckBox("Svc msg only");
    chkSuppressPolling = new QCheckBox("Suppress poll msg");

    chkSvcOnly->setChecked(true);
    chkSuppressPolling->setChecked(true);

    mLogBox = new QGroupBox("Log");
    QVBoxLayout *logvlay = new QVBoxLayout;
    mLogBox->setLayout(logvlay);
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
                        ed->setText(QString::number(device->busAddress()));
                    else if (device->busAddress() != mac)
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

    QStringList strings;
    strings << "<Interface>" << "0" << "0" << "FFFFFFFF";
    QTreeWidgetItem *item;
    if (serialMaster)
    {
        strings[0] = "<Serial>";
        item = new QTreeWidgetItem(strings);
        mTree->addTopLevelItem(item);
        item->setExpanded(true);
    }
    if (oviMaster)
    {
        strings[0] = "<WiFi>";
        item = new QTreeWidgetItem(strings);
        mTree->addTopLevelItem(item);
        item->setExpanded(true);
    }
    if (usbMaster)
    {
        strings[0] = "<Usb>";
        item = new QTreeWidgetItem(strings);
        mTree->addTopLevelItem(item);
        item->setExpanded(true);
    }
    if (udpMaster)
    {
        strings[0] = "<Udp>";
        item = new QTreeWidgetItem(strings);
        mTree->addTopLevelItem(item);
        item->setExpanded(true);
    }

    status = new QLabel("");
    ui->statusBar->addWidget(status);
    status2 = new QLabel();
    ui->statusBar->addWidget(status2);
    status3 = new QLabel();
    ui->statusBar->addWidget(status3);

    mGraph = new PlotWidget(this);
    connect(mGraph, SIGNAL(periodChanged(unsigned long,QString,int)), SLOT(setAutoRequestPeriod(unsigned long,QString,int)));
    mGraphBox = new QGroupBox("Graph");
    mGraphBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    QVBoxLayout *graphLay = new QVBoxLayout;
    graphLay->addWidget(mGraph);
    mGraphBox->setLayout(graphLay);

    mNodeWidget = new QStackedWidget;

    mainLayout = new QGridLayout;
    mainLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(mTree, 0, 0);
    mainLayout->addWidget(mInfoBox, 1, 0);
    mainLayout->addWidget(mLogBox, 2, 0, 1, 2);
    mainLayout->addWidget(mGraphBox, 0, 1, 2, 1);
    mainLayout->addWidget(mNodeWidget, 0, 2, 3, 1);
    ui->centralWidget->setLayout(mainLayout);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(100);

    mEtimer.start();

#if defined(ONB_VIRTUAL)
    mOviServerBtn->setChecked(true);
    onbvs->setEnabled(true);
#endif

    QSpinBox *aidSpin = new QSpinBox;
    aidSpin->setRange(0, 64);
    QPushButton *sendGlobalBtn = new QPushButton(">>");
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addWidget(new QLabel("Send global msg: AID="));
    ui->mainToolBar->addWidget(aidSpin);
    ui->mainToolBar->addWidget(sendGlobalBtn);
    ui->mainToolBar->addSeparator();
    connect(sendGlobalBtn, &QPushButton::clicked, [=]()
    {
        for (ObjnetMaster *master: masters)
        {
            master->sendGlobalMessage(aidSpin->value());
        }
    });

    QLineEdit *cedit = new QLineEdit();
    QPushButton *ubtn = new QPushButton("Upgrade");
    connect(ubtn, &QPushButton::clicked, [=](bool)
    {
        unsigned long cid = cedit->text().toInt(nullptr, 16);
        qDebug() << QString().sprintf("Try upgrade 0x%08X", cid);
        this->upgrade(usbMaster, cid);
    });
    ui->mainToolBar->addWidget(cedit);
    ui->mainToolBar->addWidget(ubtn);

    QSettings sets;
    firmwareDir = sets.value("firmwareFolder", "d:/projects/iar").toString();
    QPushButton *fwFolderBtn = new QPushButton("Firmware folder");
    connect(fwFolderBtn, &QPushButton::clicked, [=](){
        QString fwdir = QFileDialog::getExistingDirectory(this, "Specify firmware folder", firmwareDir);
        if (QDir(fwdir).exists())
        {
            firmwareDir = fwdir;
            QSettings sets;
            sets.setValue("firmwareFolder", firmwareDir);
        }
    });
    ui->mainToolBar->addWidget(fwFolderBtn);

    m_connTimer = new QTimer(this);
    m_connTimer->setSingleShot(true);
    m_connTimer->setInterval(250);
    connect(m_connTimer, &QTimer::timeout, this, &MainWindow::onConnTimer);

//    mLogEnableBtn->setChecked(true);

//    QPushButton *b = new QPushButton("upgrade");
//    connect(b, SIGNAL(clicked(bool)), SLOT(upgrade()));
//    ui->mainToolBar->addWidget(b);

    QSize ssz = screen()->availableSize();
    if (ssz.width() < ssz.height())
    {
        resize(800, 1000);
        changeLayout(Qt::Vertical);
        setWindowState(Qt::WindowMaximized);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    if (e->size().width() > e->size().height())
        changeLayout(Qt::Horizontal);
    else
        changeLayout(Qt::Vertical);
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

void MainWindow::logMessage(QString netname, const CommonMessage &msg)
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
    for (int i=0; i<masters.size(); i++)
        if (mas == masters[i])
            return i;
    return -1;
}

ObjnetMaster *MainWindow::getMasterOfItem(QTreeWidgetItem *item)
{
    while (item->parent())
        item = item->parent();
    int idx = mTree->indexOfTopLevelItem(item);
    if (idx >= 0 && idx < masters.size())
        return masters[idx];
    return nullptr;
}
//---------------------------------------------------------------------------

void MainWindow::onItemClick(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    ObjnetMaster *master = getMasterOfItem(item);

    ObjnetDevice *dev = item->data(0, Qt::UserRole).value<ObjnetDevice*>();
    if (dev)
    {
        device = dev;
        lastDeviceId = device->serial();

        if (!dev->isInfoValid())
        {
            qDebug() << "info not valid!";
            master->requestDevInfo(dev->netAddress());
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
        mEdits["Bus address"]->setText(QString::number(dev->busAddress()));
    }
    else
    {
        device = nullptr;
        lastDeviceId = 0;
        mInfoBox->setTitle("Device info");
        foreach (QLineEdit *ed, mEdits)
            ed->clear();
    }

    if (device)
    {
        if (device->isReady())
        {
            int idx = item->data(0, Qt::UserRole + 23).toInt();
            mNodeWidget->setCurrentIndex(idx);
        }
        else
        {
            m_connTimer->start(); // try again after a while...

            // MUST be requested ONLY if requestAllInfo fails!
//            for (int i=0; i<device->objectCount(); i++)
//                if (!device->objectInfo(i))
//                {
//                    qDebug() << "request object info" << i;
//                    device->requestObjectInfo(i);
//                }
        }
    }
}

void MainWindow::changeLayout(Qt::Orientation orient)
{
    if (orient == Qt::Horizontal)
    {
        mainLayout->addWidget(mTree, 0, 0);
        mainLayout->addWidget(mInfoBox, 1, 0);
        mainLayout->addWidget(mLogBox, 2, 0, 1, 2);
        mainLayout->addWidget(mGraphBox, 0, 1, 2, 1);
        mainLayout->addWidget(mNodeWidget, 0, 2, 3, 1);
    }
    else
    {
        // vertical layout
        mainLayout->addWidget(mTree, 0, 0);
        mainLayout->addWidget(mInfoBox, 1, 0);
        mainLayout->addWidget(mNodeWidget, 0, 1, 2, 1);
        mainLayout->addWidget(mGraphBox, 2, 0, 1, 2);
        mainLayout->addWidget(mLogBox, 3, 0, 1, 2);
        mainLayout->setRowStretch(0, 1);
        mainLayout->setRowStretch(2, 1);
        mainLayout->setRowStretch(3, 0);
    }
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
        QStringList names;
        for (int i=0; i<device->objectCount(); i++)
        {
            ObjectInfo *info = device->objectInfo(i);
            if (!info)
                continue;
            if (info->flags() & ObjectInfo::Volatile)
                names << info->name();
        }

        if (device->busType() == BusSwonb)
        {
            foreach (QString name, names)
                device->requestObject(name);
        }
        else if (device->busType() == BusRadio)
        {
            QStringList names;
            for (int i=0; i<device->objectCount(); i++)
            {
                ObjectInfo *info = device->objectInfo(i);
                if (!info)
                    continue;
                if (info->flags() & ObjectInfo::Volatile)
                    names << info->name();
            }
            device->groupedRequest(names.toVector().toStdVector());
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

void MainWindow::onConnTimer()
{
    if (device)
    {
        if (!device->isInfoValid())
        {
            device->requestMetaInfo();
            m_connTimer->start();
        }
        else if (!device->isReady())
        {
            for (int i=0; i<device->objectCount(); i++)
                if (!device->objectInfo(i) || !device->objectInfo(i)->isValid())
                    device->requestObjectInfo(i);
            m_connTimer->start();
        }
    }
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

            connect(dev, SIGNAL(objectReceived(QString,QVariant)), mGraph, SLOT(updateObject(QString,QVariant)));
            connect(dev, SIGNAL(objectGroupReceived(QVariantMap)), mGraph, SLOT(updateObjectGroup(QVariantMap)));
            connect(dev, SIGNAL(timedObjectReceived(QString,uint32_t,QVariant)), mGraph, SLOT(updateTimedObject(QString,uint32_t,QVariant)));
            connect(dev, SIGNAL(autoRequestAccepted(QString,int)), SLOT(onAutoRequestAccepted(QString,int)));
            connect(dev, SIGNAL(autoRequestAccepted(QString,int)), mGraph, SLOT(onAutoRequestAccepted(QString,int)));
            connect(dev, SIGNAL(ready()), SLOT(onDevReady()));
            connect(dev, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

            item->setData(0, Qt::UserRole, QVariant::fromValue<ObjnetDevice*>(dev));
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
        ObjnetDevice *dev = master->devices().at(netAddress);
        if (!dev->isValid())
        {
            master->requestClassId(netAddress);
            master->requestName(netAddress);
            master->requestSerial(netAddress);
        }

        if (!item->data(0, Qt::UserRole + 23).isValid())
        {
            QWidget *nodeWidget = nullptr;
            //! @todo automatic plugin loading
            if (dev->classId() == 0x01000024) // Analyzer
            {
                nodeWidget = new AnalyzerWidget(dev);
            }
            else
            {
                nodeWidget = new ObjTable(dev);
            }

            int idx = mNodeWidget->addWidget(nodeWidget);
            item->setData(0, Qt::UserRole + 23, idx);
        }
        else
        {
            int idx = item->data(0, Qt::UserRole + 23).toInt();
            mNodeWidget->widget(idx)->setEnabled(true);
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
        int idx = item->data(0, Qt::UserRole + 23).toInt();
        mNodeWidget->widget(idx)->setDisabled(true);
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
    else if (mas == udpMaster)
        text = "Global message on <Udp>: " + said;
    logMessage(text.toHtmlEscaped());
}
//---------------------------------------------------------------------------

void MainWindow::upgrade(ObjnetMaster *master, unsigned long classId, unsigned char netAddress)
{
    if (master)
    {
        QByteArray fw;
        QString filename;
        QString classString;
        classString.sprintf("0x%08X", classId);
        QSettings sets;
        sets.beginGroup("firmwareCache");

        if (sets.contains(classString))
        {
            filename = sets.value(classString).toString();
            if (!QFile(filename).exists())
                filename = "";
        }
        if (filename.isEmpty())
        {
            QStringList result;
            QStringList diritfilt;
            diritfilt << "*.bin";
            QDirIterator dirit(firmwareDir, diritfilt, QDir::Files, QDirIterator::Subdirectories);
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
                fw = f.readAll();
                f.close();
                if (UpgradeWidget::checkClass(fw, classId))
                {
                    filename = fname;
                    break;
                }
            }

            if (!filename.isEmpty())
                sets.setValue(classString, filename);
        }
        sets.endGroup();

        if (filename.isEmpty())
        {
            QByteArray ba;
            ba.append(reinterpret_cast<const char*>(&classId), 4);
            master->sendGlobalRequest(aidUpgradeStart, true, ba);
            return;
        }

        if (fw.isEmpty())
        {
            QFile f(filename);
            f.open(QIODevice::ReadOnly);
            fw = f.readAll();
            f.close();
        }

        if (!upg)
        {
            upg = new UpgradeWidget(master, this);
            connect(upg, &UpgradeWidget::destroyed, [this](){upg = 0L;});
        }
        else
        {
            upg->show();
        }
        upg->logAppend(QString("firmware found: "+filename).toStdString());
        upg->load(fw);
        upg->scan(netAddress);
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
        connect(act1, &QAction::triggered, [=](){this->upgrade(master, cid, netaddr);});
        QAction *act2 = new QAction("Upgrade all of "+item->text(0)+" ("+item->text(3)+")", this);
        connect(act2, &QAction::triggered, [=](){this->upgrade(master, cid);});
        QMenu menu(this);
        menu.addAction(act1);
        menu.addAction(act2);
        menu.setWindowModality(Qt::NonModal);
        menu.exec(mTree->mapToGlobal(p));
    }
}

void MainWindow::onObjectMenu(QPoint p)
{
//    QStandardItem *item = mObjTable->itemAt(p);
//    if (item)
//    {
//        item = mObjTable->item(item->row(), 0);

//        QAction *graphAct;
//        if (!item->data(Qt::UserRole+4).toBool())
//        {
//            graphAct = new QAction("Plot graph", this);
//            connect(graphAct, &QAction::triggered, [=](){item->setData(Qt::UserRole+4, true); mGraph->show();});
//        }
//        else
//        {
//            graphAct = new QAction("Remove graph", this);
//            connect(graphAct, &QAction::triggered, [=](){item->setData(Qt::UserRole+4, false);});
//        }
//        QMenu menu(this);
//        menu.addAction(graphAct);
//        menu.setWindowModality(Qt::NonModal);
//        menu.exec(mObjTable->mapToGlobal(p));
//    }
}

void MainWindow::setAutoRequestPeriod(unsigned long serial, QString objname, int period_ms)
{
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
        ObjnetDevice *itemdev = item->data(0, Qt::UserRole).value<ObjnetDevice*>();
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
