#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sent(0),
    received(0),
    curdev(0)
{

//    qDebug() << "-----------------";
//    qDebug() << typeid(void).name(); // v
//    qDebug() << typeid(bool).name(); // b
//    qDebug() << typeid(int).name(); // i
//    qDebug() << typeid(unsigned int).name(); // j
//    qDebug() << typeid(long long).name(); // x
//    qDebug() << typeid(unsigned long long).name(); // y
//    qDebug() << typeid(double).name(); // d
//    qDebug() << typeid(long).name(); // l
//    qDebug() << typeid(short).name(); // s
//    qDebug() << typeid(char).name(); // c
//    qDebug() << typeid(unsigned long).name(); // m
//    qDebug() << typeid(unsigned short).name(); // t
//    qDebug() << typeid(unsigned char).name(); // h
//    qDebug() << typeid(float).name(); // f
//    qDebug() << typeid(signed char).name(); // a
//    qDebug() << typeid(string).name(); // Ss
//    qDebug() << "-----------------";


    ui->setupUi(this);
//    resize(800, 500);

    uart = new QSerialPort();
    uart->setBaudRate(1000000);
    uart->setParity(QSerialPort::EvenParity);
    uart->setStopBits(QSerialPort::OneStop);
    uart->setFlowControl(QSerialPort::NoFlowControl);

    onbvs = new ObjnetVirtualServer(this);
    connect(onbvs, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));


    can = new SerialCan(uart, SerialCan::protoCommon);
    //can->setBaudrate(1000000);
    //can->setFixedWidth(260);
    connect(can, SIGNAL(onMessage(ulong,QByteArray&)), this, SLOT(onMessage(ulong,QByteArray&)));
    connect(can, SIGNAL(onMessageSent(ulong,QByteArray&)), this, SLOT(onMessageSent(ulong,QByteArray&)));
    connect(can, SIGNAL(connected()), this, SLOT(onBoardConnect()));
    connect(can, SIGNAL(disconnected()), this, SLOT(onBoardDisconnect()));
    //ui->mainToolBar->addWidget(can);

//    master = new ObjnetMaster(new SerialCanInterface(can));

//    UsbHidOnbInterface *usbonb = new UsbHidOnbInterface(new UsbHidThread(0x0bad, 0xcafe, this));
//    connect(usbonb, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
//    master = new ObjnetMaster(usbonb);

    onb << new ObjnetVirtualInterface("main");
    master = new ObjnetMaster(onb.last());
    master->setName("main");

    connect(master, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(master, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(master, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(master, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(master, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));

    onb.last()->setActive(true);

    onb << new ObjnetVirtualInterface("main");
    ObjnetNode *n1 = new ObjnetNode(onb.last());
    n1->setBusAddress(0xF);
    n1->setName("vasya");
    n1->setFullName("Vasiliy Alibabaevich");

    ObjnetNode *n2;
    for (int i=0; i<10; i++)
    {
        onb << new ObjnetVirtualInterface("main");
        n2 = new ObjnetNode(onb.last());
        n2->setBusAddress(i+1);
        n2->setName("drive");
        n2->setFullName("Motor controller");
    }

    onb << new ObjnetVirtualInterface("main");
    n2 = new ObjnetNode(onb.last());
    n2->setBusAddress(0xE);
    n2->setName("arm");

    onb << new ObjnetVirtualInterface("arm");
    ObjnetMaster *masterArm = new ObjnetMaster(onb.last());
    masterArm->setName("arm");
    masterArm->connect(n2);
    ObjnetNode *n3;
    onb << new ObjnetVirtualInterface("arm");
    n3 = new ObjnetNode(onb.last());
    n3->setBusAddress(4);
    n3->setName("shoulder");
    onb << new ObjnetVirtualInterface("arm");
    n3 = new ObjnetNode(onb.last());
    n3->setBusAddress(5);
    n3->setName("elbow");
    onb << new ObjnetVirtualInterface("arm");
    n3 = new ObjnetNode(onb.last());
    n3->setBusAddress(6);
    n3->setName("wrist");

    onb << new ObjnetVirtualInterface("hand");
    ObjnetMaster *masterHand = new ObjnetMaster(onb.last());
    masterHand->connect(n3);
    masterHand->setName("hand");
    ObjnetNode *n4;
    onb << new ObjnetVirtualInterface("hand");
    n4 = new ObjnetNode(onb.last());
    n4->setBusAddress(5);
    n4->setName("fingers");
    onb << new ObjnetVirtualInterface("hand");
    n4 = new ObjnetNode(onb.last());
    n4->setBusAddress(10);
    n4->setName("sensors");


    QVBoxLayout *netlay = new QVBoxLayout;
    QMap<QString, QWidget*> netw;
    foreach (ObjnetVirtualInterface *ovi, onb)
    {
        QString key = ovi->netname();
        if (!netw.contains(key))
        {
            QGroupBox *gb = new QGroupBox(key);
            QHBoxLayout *gblay = new QHBoxLayout;
            gb->setLayout(gblay);
            netw[key] = gb;
            netlay->addWidget(gb);
        }
        QCheckBox *chk = new QCheckBox;
        connect(chk, SIGNAL(toggled(bool)), ovi, SLOT(setActive(bool)));
        netw[key]->layout()->addWidget(chk);
    }

    mPorts = new QComboBox();
    mPorts->setFixedWidth(100);
    ui->mainToolBar->addWidget(mPorts);
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    mPorts->addItem("None");
    foreach (QSerialPortInfo port, ports)
    {
        QString s;
        if (port.isBusy())
            s += "busy ";
        if (port.isNull())
            s += "null ";
        if (!port.isValid())
            s += "invalid ";
        QString description = port.description();
        qDebug() << port.portName() << ":" << s << "//" << description;

        mPorts->addItem(port.portName());
    }
    connect(mPorts, SIGNAL(activated(QString)), SLOT(onPortChanged(QString)));

    btn = new QPushButton("Send");
    btn->setFixedWidth(80);
    connect(btn, SIGNAL(clicked()), this, SLOT(onBtn()));
    btn2 = new QPushButton("Send 30");
    btn2->setFixedWidth(80);
    connect(btn2, SIGNAL(clicked()), this, SLOT(onBtn2()));

    btnResetStat = new QPushButton("Reset stat");
    btnResetStat->setFixedWidth(80);
    connect(btnResetStat, SIGNAL(clicked()), this, SLOT(resetStat()));
    ui->mainToolBar->addWidget(btnResetStat);

    btnProto1 = new QPushButton("Common");
    btnProto1->setFixedWidth(100);
    connect(btnProto1, SIGNAL(clicked()), this, SLOT(onBtnProto()));
    btnProto2 = new QPushButton("Extended");
    btnProto2->setFixedWidth(100);
    connect(btnProto2, SIGNAL(clicked()), this, SLOT(onBtnProto()));

    editId = new QLineEdit("01234567");
    editId->setFixedWidth(80);
    editData = new QLineEdit("1 2 3 4 5");
    editData->setMinimumWidth(100);

    editIdIn = new QLineEdit();
    editIdIn->setFixedWidth(80);
    editIdIn->setReadOnly(true);
    editDataIn = new QLineEdit();
    editDataIn->setMinimumWidth(100);
    editDataIn->setReadOnly(true);

    editLog = new QTextEdit();
    editLog->setMinimumWidth(200);
//    editLog->setReadOnly(true);

    mTree = new QTreeWidget();
    mTree->setMinimumWidth(250);
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

    mInfoBox = new QGroupBox("Device info");
    QFormLayout *gblay = new QFormLayout;
    QStringList editnames;
    editnames << "Net address" << "Class ID" << "Name" << "Full name" << "Serial" << "Version" << "Build date" << "CPU info" << "Burn count";
    foreach (QString s, editnames)
    {
        QLineEdit *ed = new QLineEdit;
        ed->setReadOnly(true);
        ed->setMinimumWidth(200);
        mEdits[s] = ed;
        gblay->addRow(s, ed);
    }
    mInfoBox->setLayout(gblay);

    mObjTable = new QTableWidget(1, 2);
    mObjTable->setColumnWidth(1, 150);
    connect(mObjTable, SIGNAL(cellChanged(int,int)), SLOT(onCellChanged(int,int)));

    // ONLY IF ONBVS USED!!
    QStringList strings;
    strings << "<root>" << "0" << "0" << "FFFFFFFF";
    QTreeWidgetItem *item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);

    status = new QLabel("Disconnected");
    ui->statusBar->addWidget(status);
    status2 = new QLabel();
    ui->statusBar->addWidget(status2);

    QGridLayout *glay = new QGridLayout;
    QGroupBox *protoBox = new QGroupBox("Protocol");
    QHBoxLayout *h0 = new QHBoxLayout();
    h0->addWidget(btnProto1);
    h0->addWidget(btnProto2);
    protoBox->setLayout(h0);
    QGroupBox *outBox = new QGroupBox("Out message");
    QHBoxLayout *h1 = new QHBoxLayout();
    h1->addWidget(new QLabel("Id"));
    h1->addWidget(editId);
    h1->addWidget(new QLabel("Data"));
    h1->addWidget(editData);
    h1->addWidget(btn);
    h1->addWidget(btn2);
    outBox->setLayout(h1);
    QGroupBox *inBox = new QGroupBox("In message");
    QHBoxLayout *h2 = new QHBoxLayout();
    h2->addWidget(new QLabel("Id"));
    h2->addWidget(editIdIn);
    h2->addWidget(new QLabel("Data"));
    h2->addWidget(editDataIn);
    inBox->setLayout(h2);
    glay->addLayout(netlay, 0, 0, 2, 1);
    glay->addWidget(mTree, 2, 0);
    //glay->addWidget(protoBox, 0, 1);
    glay->addWidget(outBox, 0, 1);
    glay->addWidget(inBox, 1, 1);
    glay->addWidget(editLog, 2, 1);
    glay->addWidget(mObjTable, 0, 2, 2, 1);
    glay->addWidget(mInfoBox, 2, 2);
    ui->centralWidget->setLayout(glay);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);

    mEtimer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
//---------------------------------------------------------------------------

void MainWindow::onBtn()
{
    bool ok;
    ulong id = editId->text().toInt(&ok, 16);
    if (!ok)
        return;
    QString strdata = editData->text().trimmed();
    QByteArray data;
    if (!strdata.isEmpty())
    {
        QStringList strbytes = strdata.split(' ');
        foreach (QString strbyte, strbytes)
        {
            unsigned char byte = strbyte.toInt(&ok, 16);
            if (!ok)
                return;
            data.append(byte);
        }
    }
    curdev = (id >> 24) & 0xF;
    //can->sendMessage(id, data);
    CommonMessage msg;
    msg.setId(id);
    msg.setData(data);
    master->objnetInterface()->write(msg);
}

void MainWindow::onBtn2()
{
    for (int i=0; i<30; i++)
        onBtn();
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
    editIdIn->setText(strid);
    editDataIn->setText(strdata);
    logMessage(id, data);
}

void MainWindow::onMessageSent(ulong id, QByteArray &data)
{
    if (!can->isActive())
        return;
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
    //editId->setText(strid);
    //editData->setText(strdata);
    logMessage(id, data, 1);
}
//---------------------------------------------------------------------------

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
    QString text;
    text.sprintf("[%d] ", mEtimer.elapsed());
    text += netname + ": ";
    QString sid = QString().sprintf("%08X", (unsigned int)msg.rawId());
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
        text += ba2str(msg.data());
        if (msg.localId().mac != 0)
            text = "<font color=red>" + text + "</font>";
        else
            text = "<font color=blue>" + text + "</font>";
    }
    editLog->append(text);
//    qDebug() << text;
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

void MainWindow::onBtnProto()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn == btnProto1)
    {
        can->changeProtocolVersion(SerialCan::protoCommon);
    }
    else if (btn == btnProto2)
    {
        can->changeProtocolVersion(SerialCan::protoExtended);
    }
}

void MainWindow::onItemClick(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    mObjTable->clear();
    mObjTable->setRowCount(0);
    QVariant v = item->data(0, Qt::UserRole);
    ObjnetDevice *dev = reinterpret_cast<ObjnetDevice*>(v.toInt());
    if (dev)
    {
        curdev = dev->netAddress();

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

        int cnt = dev->objectCount();
        mObjTable->setColumnCount(4);
        mObjTable->setRowCount(cnt);
        mObjTable->blockSignals(true);
        for (int i=0; i<cnt; i++)
        {
            ObjectInfo *info = dev->objectInfo(i);
            QString name = info->name();
            mObjTable->setItem(i, 1, new QTableWidgetItem("n/a"));
            QString wt = QMetaType::typeName(info->wType());
            if (wt == "QString")
                wt = "string";
            else if (wt == "QByteArray")
                wt = "common";
            QString rt = QMetaType::typeName(info->rType());
            if (rt == "QString")
                rt = "string";
            else if (rt == "QByteArray")
                rt = "common";
            unsigned char fla = info->flags();
            QString typnam;


            if (fla & ObjectInfo::Function)
            {
                typnam = wt + "(" + rt + ")";
                mObjTable->setItem(i, 0, new QTableWidgetItem(name));
                QPushButton *btn = new QPushButton(name);
                mObjTable->setCellWidget(i, 0, btn);
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

                mObjTable->setItem(i, 0, new QTableWidgetItem(name));
                dev->requestObject(name);
            }

            mObjTable->setItem(i, 2, new QTableWidgetItem(typnam));
            QString flags = "-fdhsrwv";
            for (int j=0; j<8; j++)
                if (!(fla & (1<<j)))
                    flags[7-j] = '-';
            mObjTable->setItem(i, 3, new QTableWidgetItem(flags));
        }
        mObjTable->blockSignals(false);
    }
    else
    {
        curdev = 0;
        mInfoBox->setTitle("Device info");
        foreach (QLineEdit *ed, mEdits)
            ed->clear();
    }
}

void MainWindow::onCellChanged(int row, int col)
{
    if (col != 1)
        return;
    if (master->devices().count(curdev))
    {
        ObjnetDevice *dev = master->devices().at(curdev);
        ObjectInfo *info = dev->objectInfo(row);
        QVariant val = mObjTable->item(row, col)->text();
        if (info->rType() == ObjectInfo::Common)
            val = QByteArray::fromHex(val.toByteArray());
        else
            val.convert(info->rType());
        info->fromVariant(val);
        if (info->flags() & ObjectInfo::Function)
            return;
        dev->sendObject(info->name());
    }
}

void MainWindow::onObjectReceive(QString name, QVariant value)
{
    //qDebug() << "[MainWindow] object received:" << name << "=" << value;
    for (int i=0; i<mObjTable->rowCount(); i++)
    {
        QString nm = mObjTable->item(i, 0)->text();
        if (nm == name)
        {
            QString val;
#undef ByteArray
            if (!value.isValid())
                continue;
            if (value.type() == QVariant::ByteArray)
                val = value.toByteArray().toHex();
            else// if (value.type() == QVariant::String)
            {
                if (static_cast<QMetaType::Type>(value.type()) == QMetaType::UChar)
                    value = value.toInt();
                val = value.toString();
            }
            mObjTable->blockSignals(true);
            mObjTable->item(i, 1)->setText(val);
            mObjTable->blockSignals(false);
        }
    }
}
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
    master->reset();
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
//    master->task();


    status->setText(QString::number(mAdcValue));
    status2->setText(strtest);

    if (master->devices().count(curdev))
    {
        ObjnetDevice *dev = master->devices().at(curdev);
        for (int i=0; i<dev->objectCount(); i++)
        {
            ObjectInfo *info = dev->objectInfo(i);
            if (info->flags() & ObjectInfo::Volatile)
                dev->requestObject(info->name());
        }
        //dev->sendObject("testVar");
        setWindowTitle(dev->fullName());
    }
}
//---------------------------------------------------------------------------

void MainWindow::onDevAdded(unsigned char netAddress, const QByteArray &locData)
{
    editLog->append("device added: netAddress="+QString().sprintf("0x%02X", netAddress)+"; loc = "+ba2str(locData));

    QTreeWidgetItem *parent = mTree->topLevelItem(0);

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
                    mItems.remove(netaddr);
                    parent->setText(2, QString::number(netAddress));
                    mItems[netAddress] = parent;
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
            connect(dev, SIGNAL(objectReceived(QString,QVariant)), SLOT(onObjectReceive(QString,QVariant)));
            if (dev->bindVariable("adc", mAdcValue))
                qDebug() << "variable 'adc' binded";
            else
                qDebug() << "type mismatch while binding variable 'adc'";
            dev->bindVariable("testString", strtest);
            dev->bindVariable("testVar", testVar);
            //dev->bindMethod("var2", this, &MainWindow::onBindTest);

            int ptr = reinterpret_cast<int>(dev);
            item->setData(0, Qt::UserRole, ptr);
            parent->addChild(item);
            mItems[netAddress] = item;
            item->setExpanded(true);
            item->setDisabled(true);
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
    editLog->append("device connected: netAddress="+QString().sprintf("0x%02X", netAddress));
    QTreeWidgetItem *item = mItems.value(netAddress, 0L);
    if (item)
    {
        item->setDisabled(false);
        const ObjnetDevice *dev2 = master->devices().at(netAddress);
        if (!dev2->isValid())
        {
            master->requestClassId(netAddress);
            master->requestName(netAddress);
        }
    }
}

void MainWindow::onDevDisconnected(unsigned char netAddress)
{
    editLog->append("device disconnected: netAddress="+QString().sprintf("0x%02X", netAddress));
    QTreeWidgetItem *item = mItems.value(netAddress, 0L);
    if (item)
    {
        item->setDisabled(true);
    }
}

void MainWindow::onDevRemoved(unsigned char netAddress)
{
    editLog->append("device removed: netAddress="+QString().sprintf("0x%02X", netAddress));

    QTreeWidgetItem *item = mItems.value(netAddress, 0L);
    if (item)
    {
        item->parent()->removeChild(item);
    }
    mItems.remove(netAddress);
}
//---------------------------------------------------------------------------

void MainWindow::onServiceMessageAccepted(unsigned char netAddress, SvcOID oid, const QByteArray &data)
{
    QTreeWidgetItem *item = mItems.value(netAddress, 0L);
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

          default:;
        }
    }
}
//---------------------------------------------------------------------------

void MainWindow::onPortChanged(QString portname)
{
    if (portname == "None")
    {
        can->setActive(false);
    }
    else
    {
        uart->setPortName(portname);
        can->setActive(true);
    }
}

void MainWindow::upgrade()
{
    master->sendGlobalMessage(aidUpgradeStart);
    master->sendGlobalMessage(aidUpgradeConfirm);
    master->sendGlobalMessage(aidUpgradeData);

    master->sendGlobalMessage(aidUpgradeEnd);
}

void MainWindow::onBindTest(int var)
{
    qDebug() << "binded method called (!!) with var = " << var;
}
//---------------------------------------------------------------------------
