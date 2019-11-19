#include "upgradewidget.h"

UpgradeWidget::UpgradeWidget(ObjnetMaster *onbMaster, QWidget *parent) : QWidget(parent, Qt::Tool)
{
    setWindowTitle("Upgrade");

    upg = new OnbUpgrader(onbMaster);
    upg->logEvent = EVENT(&UpgradeWidget::logAppend);
    connect(upg, SIGNAL(finished()), SLOT(onFinish()));

    pb = new QProgressBar();
    pb->setTextVisible(true);

    connect(upg, SIGNAL(progressChanged(int)), pb, SLOT(setValue(int)));

    log = new QTextEdit();
    log->setReadOnly(true);

    scanBtn = new QPushButton("rescan");
    connect(scanBtn, SIGNAL(clicked(bool)), SLOT(scan()));
    //scanBtn->setEnabled(false);

    startBtn = new QPushButton("start");
    connect(startBtn, SIGNAL(clicked(bool)), SLOT(start()));
    //startBtn->setEnabled(false);

    QVBoxLayout *vlay = new QVBoxLayout;
    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(scanBtn);
    hlay->addWidget(startBtn);
    vlay->addLayout(hlay);
    vlay->addWidget(pb);
    vlay->addWidget(log);
    setLayout(vlay);

    show();
}

void UpgradeWidget::closeEvent(QCloseEvent *e)
{
    upg->deleteLater();
    Q_UNUSED(e);
    deleteLater();
}

void UpgradeWidget::load(const QByteArray &firmware)
{
    mBin = firmware;
    upg->load(mBin.data(), mBin.size());
}

bool UpgradeWidget::checkClass(const QByteArray &firmware, unsigned long cid)
{
    return OnbUpgrader::checkClass(firmware.data(), firmware.size(), cid);
}

void UpgradeWidget::logAppend(string s)
{
    log->moveCursor(QTextCursor::End);
    log->insertPlainText(QString::fromStdString(s));
    log->moveCursor(QTextCursor::End);
}

void UpgradeWidget::scan(unsigned char netaddr)
{
    if (!mAddresses.contains(netaddr))
        mAddresses << netaddr;
    scan();
}

void UpgradeWidget::scan()
{
    if (mAddresses.size())
    {
        foreach (unsigned char addr, mAddresses)
            upg->scan(addr);
    }
    else
    {
        upg->scan();
    }
//    scanBtn->setEnabled(false);
}

void UpgradeWidget::start()
{
    upg->start();
    scanBtn->setEnabled(false);
    startBtn->setEnabled(false);
}

void UpgradeWidget::onFinish()
{
    scanBtn->setEnabled(true);
    startBtn->setEnabled(true);
}
