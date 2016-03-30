#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sent(0),
    received(0),
    device(0L)
{
    ui->setupUi(this);
//    resize(800, 500);

    for(int i =0;i<8;i++)
    {
        tempEnc1[i]=0;
        tempEnc2[i]=0;
        tempEnc3[i]=0;

    }

    uart = new QSerialPort();
    uart->setBaudRate(1000000);
    uart->setParity(QSerialPort::EvenParity);
    uart->setStopBits(QSerialPort::OneStop);
    uart->setFlowControl(QSerialPort::NoFlowControl);

    onbvs = new ObjnetVirtualServer(this);
    connect(onbvs, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
    connect(onbvs, SIGNAL(message(QString)), SLOT(logMessage(QString)));


    can = new SerialCan(uart, SerialCan::protoCommon);
    //can->setBaudrate(1000000);
    //can->setFixedWidth(260);
    connect(can, SIGNAL(onMessage(ulong,QByteArray&)), this, SLOT(onMessage(ulong,QByteArray&)));
    connect(can, SIGNAL(onMessageSent(ulong,QByteArray&)), this, SLOT(onMessageSent(ulong,QByteArray&)));
    connect(can, SIGNAL(connected()), this, SLOT(onBoardConnect()));
    connect(can, SIGNAL(disconnected()), this, SLOT(onBoardDisconnect()));
    //ui->mainToolBar->addWidget(can);

//    master = new ObjnetMaster(new SerialCanInterface(can));

    UsbHidOnbInterface *usbonb = new UsbHidOnbInterface(new UsbHidThread(0x0bad, 0xcafe, this));
    connect(usbonb, SIGNAL(message(QString,CommonMessage&)), SLOT(logMessage(QString,CommonMessage&)));
    usbMaster = new ObjnetMaster(usbonb);

    connect(usbMaster, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(usbMaster, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(usbMaster, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(usbMaster, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(usbMaster, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));
    connect(usbMaster, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

    onb << new ObjnetVirtualInterface("main");
    oviMaster = new ObjnetMaster(onb.last());
    oviMaster->setName("main");

    connect(oviMaster, SIGNAL(devAdded(unsigned char,QByteArray)), this, SLOT(onDevAdded(unsigned char,QByteArray)));
    connect(oviMaster, SIGNAL(devConnected(unsigned char)), this, SLOT(onDevConnected(unsigned char)));
    connect(oviMaster, SIGNAL(devDisconnected(unsigned char)), this, SLOT(onDevDisconnected(unsigned char)));
    connect(oviMaster, SIGNAL(devRemoved(unsigned char)), this, SLOT(onDevRemoved(unsigned char)));
    connect(oviMaster, SIGNAL(serviceMessageAccepted(unsigned char,SvcOID,QByteArray)), this, SLOT(onServiceMessageAccepted(unsigned char,SvcOID,QByteArray)));
    connect(oviMaster, SIGNAL(globalMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

    //onb.last()->setActive(true);

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
    QCheckBox *firstOviNode = 0L;
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
        if (!firstOviNode)
            firstOviNode = chk;
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

    mOviServerBtn = new QPushButton("ONB server");
    mOviServerBtn->setCheckable(true);
    connect(mOviServerBtn, SIGNAL(clicked(bool)), onbvs, SLOT(setEnabled(bool)));
    ui->mainToolBar->addWidget(mOviServerBtn);

    mLogEnableBtn = new QPushButton("Enable log");
    mLogEnableBtn->setCheckable(true);
    ui->mainToolBar->addWidget(mLogEnableBtn);

    btn = new QPushButton("Send");
    btn->setFixedWidth(80);
    connect(btn, SIGNAL(clicked()), this, SLOT(onBtn()));

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
//    editLog->setFontFamily("Courier New");
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

    mObjTable = new QTableWidget();//1, 2);
    mObjTable->setMinimumWidth(372);
    connect(mObjTable, SIGNAL(cellChanged(int,int)), SLOT(onCellChanged(int,int)));

    QStringList strings;
    strings << "<Usb>" << "0" << "0" << "FFFFFFFF";
    QTreeWidgetItem *item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);
    strings[0] = "<WiFi>";
    item = new QTreeWidgetItem(strings);
    mTree->addTopLevelItem(item);
    item->setExpanded(true);

    status = new QLabel("Disconnected");
    ui->statusBar->addWidget(status);
    status2 = new QLabel();
    ui->statusBar->addWidget(status2);
    status3 = new QLabel();
    ui->statusBar->addWidget(status3);

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
    outBox->setLayout(h1);
    QGroupBox *inBox = new QGroupBox("In message");
    QHBoxLayout *h2 = new QHBoxLayout();
    h2->addWidget(new QLabel("Id"));
    h2->addWidget(editIdIn);
    h2->addWidget(new QLabel("Data"));
    h2->addWidget(editDataIn);
    inBox->setLayout(h2);

    glay->addWidget(mTree, 0, 0, 2, 1);
    glay->addWidget(outBox, 0, 1);
    glay->addWidget(inBox, 1, 1);
    glay->addLayout(netlay, 0, 2, 2, 1);
    glay->addWidget(mInfoBox, 2, 0);
    glay->addWidget(editLog, 2, 1);
    glay->addWidget(mObjTable, 2, 2);

    ui->centralWidget->setLayout(glay);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(30);

    mEtimer.start();


//    QVBoxLayout *vl2 = new QVBoxLayout();
//    QWidget *widget =  new QWidget(this,Qt::Window|Qt::Tool);
//    widget->setMinimumSize(400, 300);
//    widget->show();


//    panel3d = new QPanel3D(widget);
//    panel3d->show();

//    mGraph = new Graph2D(panel3d->root());



//    panel3d->camera()->setPosition(QVector3D(0, 0, 10));
//    panel3d->camera()->setDirection(QVector3D(0, 0, -1));
//    panel3d->camera()->setTopDir(QVector3D(0, 1, 0));
//    panel3d->camera()->setOrtho(true);
//    panel3d->camera()->setFixedViewportSize(QSizeF(110, 110));
//    panel3d->camera()->setFixedViewport(true);
//    panel3d->setBackColor(Qt::white);
//    panel3d->setLightingEnabled(false);

//    mGraph->setSize(100, 100);
//    mGraph->setPosition(-50, -50, 0);
//    mGraph->setBounds(QRectF(0, 0, 0, 0));

//    mGraph->setLabelX("angle1");
//    mGraph->setLabelX("angle2");
//    mGraph->setLabelX("angle3");
//    mGraph->setLabelX("angle4");

//    vl2->addWidget(panel3d);
//    widget->setLayout(vl2);


    mOviServerBtn->setChecked(true);
    onbvs->setEnabled(true);
    firstOviNode->setChecked(true);
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
//    curdev = (id >> 24) & 0xF;
    //can->sendMessage(id, data);
    CommonMessage msg;
    msg.setId(id);
    msg.setData(data);
    if (master)
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
    editIdIn->setText(sid);
    editDataIn->setText(sdata);
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

int MainWindow::getRootId(ObjnetMaster *mas)
{
    if (mas == usbMaster)
        return 0;
    else if (mas == oviMaster)
        return 1;
    else
        return -1;
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
    //int rootId = item->data(0, Qt::UserRole+1).toInt();
    QVariant v = item->data(0, Qt::UserRole);
    ObjnetDevice *dev = reinterpret_cast<ObjnetDevice*>(v.toInt());
    if (dev)
    {
        device = dev;

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
        mObjTable->setColumnWidth(0, 100);
        mObjTable->setColumnWidth(1, 100);
        mObjTable->setColumnWidth(2, 80);
        mObjTable->setColumnWidth(3, 50);
        QStringList objtablecolumns;
        objtablecolumns << "name" << "value" << "type" << "flags";
        mObjTable->setHorizontalHeaderLabels(objtablecolumns);
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
        device = 0L;
        mInfoBox->setTitle("Device info");
        foreach (QLineEdit *ed, mEdits)
            ed->clear();
    }
}

void MainWindow::onCellChanged(int row, int col)
{
    if (col != 1)
        return;
    if (device)
    {
        ObjectInfo *info = device->objectInfo(row);
        QVariant val = mObjTable->item(row, col)->text();
        if (info->rType() == ObjectInfo::Common)
            val = QByteArray::fromHex(val.toByteArray());
        else
            val.convert(info->rType());
        info->fromVariant(val);
        if (info->flags() & ObjectInfo::Function)
            return;
        device->sendObject(info->name());
    }
}

void MainWindow::onObjectReceive(QString name, QVariant value)
{
    if (name == "App::incrementTest")
    {
        static float oldt = -1;
        static float tf = 0;
        float t1 = mEtimer.nsecsElapsed() * 1e-6;
        float dt = t1 - oldt;
        if (dt > 1000)
            dt = 5;
        if (oldt < 0)
        {
            tf = dt;
        }
        else
        {
            tf = 0.999*tf + 0.001*dt;
            static int oval = 0;
            static int errs = 0;
            QString str = value.toString();
            int idx = str.indexOf(' ');
            int val = str.left(idx).toInt();
            if (oval != val)
            {
                errs++;
                oval = val;
            }
            oval++;
            status3->setText(QString().sprintf("dt= %.2f ms, errs=%d", tf, errs));
        }
        oldt = t1;


    }

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
    status->setText(QString::number(mAdcValue));
    status2->setText(strtest);

    if (device)
    {
        for (int i=0; i<device->objectCount(); i++)
        {
            ObjectInfo *info = device->objectInfo(i);
//            if (info->flags() & ObjectInfo::Volatile)
//                device->requestObject(info->name());
        }
        //device->sendObject("testVar");
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
            connect(dev, SIGNAL(objectReceived(QString,QVariant)), SLOT(onObjectReceive(QString,QVariant)));
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

          default:;
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
    logMessage(text.toHtmlEscaped());
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
    if (master)
    {
        master->sendGlobalMessage(aidUpgradeStart);
        master->sendGlobalMessage(aidUpgradeConfirm);
        master->sendGlobalMessage(aidUpgradeData);

        master->sendGlobalMessage(aidUpgradeEnd);
    }
}

void MainWindow::onBindTest(int var)
{
    qDebug() << "binded method called (!!) with var = " << var;
}
//---------------------------------------------------------------------------

void MainWindow::onDevReady()
{
    ObjnetDevice *dev = qobject_cast<ObjnetDevice*>(sender());
    logMessage("device ready: " + dev->name());
    //dev->autoRequest("testString", 3);
    //dev->autoRequest("App::incrementTest", 5);

    for (int i=0; i<dev->objectCount(); i++)
    {
        ObjectInfo *info = dev->objectInfo(i);
        if (info->isVolatile())
            dev->autoRequest(info->name(), 30);
    }
}
//---------------------------------------------------------------------------
