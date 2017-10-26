#include "upgradewidget.h"

UpgradeWidget::UpgradeWidget(ObjnetMaster *onbMaster, QWidget *parent) : QWidget(parent, Qt::Tool),
    master(onbMaster), state(sIdle), mDevCount(0), mCurDevCount(0), sz(0), cnt(0), pageDone(false), pageTransferred(false), pageRepeat(false)
{
    setWindowTitle("Upgrade");
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(onTimer()));

    pb = new QProgressBar();
    pb->setTextVisible(true);

    log = new QTextEdit();
    log->setReadOnly(true);

    scanBtn = new QPushButton("rescan");
    connect(scanBtn, SIGNAL(clicked(bool)), SLOT(scan()));
    scanBtn->setEnabled(false);

    startBtn = new QPushButton("start");
    connect(startBtn, SIGNAL(clicked(bool)), SLOT(start()));
    startBtn->setEnabled(false);

    QVBoxLayout *vlay = new QVBoxLayout;
    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(scanBtn);
    hlay->addWidget(startBtn);
    vlay->addLayout(hlay);
    vlay->addWidget(pb);
    vlay->addWidget(log);
    setLayout(vlay);

    connect(master, SIGNAL(globalServiceMessage(unsigned char)), SLOT(onGlobalMessage(unsigned char)));

    show();
}

void UpgradeWidget::closeEvent(QCloseEvent *e)
{
    timer->stop();

    master->sendServiceRequest(aidUpgradeEnd, true);

    Q_UNUSED(e);
    deleteLater();
}

void UpgradeWidget::load(QByteArray firmware)
{
    // 0x6000F05F 0x60084912

    state = sIdle;
    bin = firmware;
    sz = bin.size();
    while (sz & 0x3)
    {
        bin.append('\0');
        sz = bin.size();
    }

    mDevCount = mCurDevCount = 0;
    cnt = 0;
    pageDone = false;
    pageTransferred = false;
    pageRepeat = false;
    pb->setRange(0, sz);
    log->append("firmware size = "+QString::number(sz)+" bytes");

    int idx = firmware.indexOf("__APPINFO__");
    if (idx < 0)
    {
        log->append("no app info supplied. abort");
        return;
    }

    __appinfo_t__ *info = reinterpret_cast<__appinfo_t__*>(bin.data()+idx);
    log->append(QString().sprintf("class id = 0x%08X", (unsigned int)info->cid));
    log->append(QString().sprintf("version %d.%d, %s", info->ver >> 8, info->ver & 0xFF, info->timestamp));
    log->append(QString().sprintf("burn count = %d", info->burncount));
    mClass = info->cid;
    info->length = sz;
    info->checksum = 0;
    unsigned long cs = 0;
    unsigned long *data = reinterpret_cast<unsigned long*>(bin.data());
    for (int i=0; i<sz/4; i++)
        cs -= data[i];
    info->checksum = cs;
    log->append(QString().sprintf("checksum = 0x%08X", (unsigned int)info->checksum));


    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&mClass), 4);
    master->sendServiceRequest(aidUpgradeStart, true, ba);
    log->append(QString().sprintf("start upgrade devices with class = 0x%08X\n", (unsigned int)mClass));
    timer->start(1000);
}

bool UpgradeWidget::checkClass(const QByteArray &firmware, unsigned long cid)
{
    int idx = firmware.indexOf("__APPINFO__");
    if (idx < 0)
        return false;
    const __appinfo_t__ *info = reinterpret_cast<const __appinfo_t__*>(firmware.data()+idx);
    return (info->cid == cid);
}

void UpgradeWidget::onTimer()
{
    timer->setInterval(16);

    if (state == sIdle)
    {
        scanBtn->setEnabled(true);

        if (!mDevCount)
        {
            timer->stop();
            log->append("no devices to upgrade :c");
            return;
        }

        log->append("found "+QString::number(mDevCount)+" device(s)\n");
        startBtn->setEnabled(true);
        timer->stop();
        return;
    }
    else if (state == sStarted)
    {
        if (mCurDevCount >= mDevCount)
        {
            log->append("all of devices are ready");
            state = sWork;
        }
    }
    else if (state == sWork)
    {
        for (int i=0; i<32; i++)
        {
            if (pageRepeat && cnt)
                cnt -= (1<<11);

            QByteArray ba = bin.mid(cnt, 8);
            int page = cnt >> 11;
            int seq = (cnt >> 3) & 0xFF;

            if ((!seq && !pageDone) || pageRepeat)
            {
                pageTransferred = true;
                mCurDevCount = 0;
                QByteArray ba2;
                ba2.append(reinterpret_cast<const char*>(&page), 4);
                log->append("page " + QString::number(page) + " of " + QString::number(sz >> 11)+" ...");
                master->sendServiceRequest(aidUpgradeSetPage, true, ba2);
                timer->stop();
                return;
            }

            pageDone = false;
            pageRepeat = false;

            //                log->moveCursor(QTextCursor::End);
            //                log->insertPlainText(".");
            //                log->moveCursor(QTextCursor::End);

            master->sendUpgrageData(seq, ba);
            cnt += ba.size();
            pb->setValue(cnt);

            if (seq == 0xFF)
                timer->setInterval(100);

            if (cnt >= sz)
            {
                timer->setInterval(100);
                state = sFinish;
                break;
            }
        }
    }
    else if (state == sFinish)
    {
        log->append("upgrade finished");
        master->sendServiceRequest(aidUpgradeEnd, true);
        timer->stop();
        scanBtn->setEnabled(true);
        startBtn->setEnabled(false);
        return;
    }
}

void UpgradeWidget::scan()
{
    state = sIdle;
    mDevCount = 0;
    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&mClass), 4);
    master->sendServiceRequest(aidUpgradeStart, true, ba);
    log->append(QString().sprintf("start upgrade devices with class = 0x%08X\n", (unsigned int)mClass));
    timer->start(1000);
    scanBtn->setEnabled(false);
}

void UpgradeWidget::start()
{
    mCurDevCount = 0;
    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&sz), 4);
    master->sendServiceRequest(aidUpgradeConfirm, true, ba);
    state = sStarted;
    scanBtn->setEnabled(false);
    startBtn->setEnabled(false);
    timer->start(100);
}

void UpgradeWidget::onGlobalMessage(unsigned char aid)
{
    aid &= 0x3F;
    if (aid == aidUpgradePageDone)
    {
        mCurDevCount++;
        if (mCurDevCount >= mDevCount)
        {
            log->moveCursor(QTextCursor::End);
            log->insertPlainText(" success");
            log->moveCursor(QTextCursor::End);
            pageDone = true;
            pageTransferred = false;
            timer->start(30);
        }
    }
    else if (aid == aidUpgradeRepeat && (pageTransferred || !cnt))
    {
        log->moveCursor(QTextCursor::End);
        log->insertPlainText(" FAIL! repeat page");
        log->moveCursor(QTextCursor::End);
        pageRepeat = true;
        pageDone = true;
        pageTransferred = false;
        timer->start(30);
    }
    else if (aid == aidUpgradeAccepted)
    {
        mDevCount++;
        log->moveCursor(QTextCursor::End);
        log->insertPlainText("+1 ");
        log->moveCursor(QTextCursor::End);
    }
    else if (aid == aidUpgradeReady)
    {
        mCurDevCount++;
        log->moveCursor(QTextCursor::End);
        log->insertPlainText("+1 ");
        log->moveCursor(QTextCursor::End);
    }
}