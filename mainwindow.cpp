#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sent(0),
    received(0)
{
    ui->setupUi(this);
    resize(32, 500);

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

    master = new ObjnetMaster(new SerialCanInterface(can));
//    onb << new ObjnetVirtualInterface("main");
//    master = new ObjnetMaster(onb.last());
    connect(master, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(master, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(master, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(master, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(master, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));

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
    ui->centralWidget->setLayout(glay);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);
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
    QString text = netname + ": ";
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

    if (master->devices().count(curdev))
    {
        const ObjnetDevice *dev = master->devices().at(curdev);
        setWindowTitle(QString::fromStdString(dev->fullName()));
    }
}
//---------------------------------------------------------------------------

void MainWindow::onDevAdded(unsigned char netAddress, const QByteArray &locData)
{
    editLog->append("device added: netAddress="+QString().sprintf("0x%02X", netAddress)+"; loc = "+ba2str(locData));

    QTreeWidgetItem *parent = mTree->topLevelItem(0);

    for (int i=locData.size()-1; i>=0; i--)
    {
        int mac = locData[0];
        int netaddr = locData[i];
        bool found = false;
        for (int j=0; j<parent->childCount(); j++)
        {
            if (parent->child(j)->text(1).toInt() == netaddr)
            {
                parent = parent->child(j);
                found = true;
                if (i == 0)
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
            if (i == 0)
                strings << QString::number(netAddress);
            QTreeWidgetItem *item = new QTreeWidgetItem(strings);
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
//---------------------------------------------------------------------------
