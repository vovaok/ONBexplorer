#include "upgradewidget.h"

UpgradeWidget::UpgradeWidget(ObjnetMaster *onbMaster, QWidget *parent) : QWidget(parent, Qt::Tool),
    master(onbMaster), state(sIdle), mDevCount(0), mCurDevCount(0), sz(0), cnt(0), pagesz(0), pageDone(false), pageTransferred(false), pageRepeat(false)
{
    setWindowTitle("Upgrade");
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(onTimer()));

    mNetAddress = 0;

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

    master->sendGlobalRequest(aidUpgradeEnd, true);

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
    log->append(QString().sprintf("page size = %d", info->pageSize));
    mClass = info->cid;
    pagesz = info->pageSize;
    if (pagesz < 8)
        pagesz = 2048;
    info->length = sz;
    info->checksum = 0;
    unsigned long cs = 0;
    unsigned long *data = reinterpret_cast<unsigned long*>(bin.data());
    for (int i=0; i<sz/4; i++)
        cs -= data[i];
    info->checksum = cs;
    log->append(QString().sprintf("checksum = 0x%08X", (unsigned int)info->checksum));

    unsigned long aaa = 0;
    for (int i=0; i<sz/4; i++)
        aaa += data[i];

    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&mClass), 4);
    master->sendGlobalRequest(aidUpgradeStart, true, ba);
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
            mCurDevCount = 0;
            log->append("all of devices are ready");
            cnt = 0;
            pageDone = pageTransferred = pageRepeat = false;
            setPage(0);
            state = sWork;
        }
    }
    else if (state == sWork)
    {
        if (pagesz < 8) // backwards compatibility
            pagesz = 2048;

        if (pageTransferred)
        {
//            qDebug() << "page" << page << "done =" << pageDone;
            if (!pageDone && !pageRepeat)
            {
//                pageRepeat = true;
//                return;
                master->sendGlobalRequest(aidUpgradeProbe, true);
                log->append("no response, probe request...");
                timer->setInterval(200);
                //state = sError;
                return;
            }
            else
            {
                pageTransferred = false;
                if (pageRepeat && cnt)
                    cnt = ((cnt - 1) / pagesz) * pagesz;
                int page = cnt / pagesz;

                if (cnt < sz)
                    setPage(page);
                else
                    state = sFinish;
                return;
            }
        }

        pageDone = false;
        pageRepeat = false;
        curbytes = 0;

        state = sTransferPage;
        timer->setInterval(16);
    }
    else if (state == sTransferPage)
    {
        timer->setInterval(16);

        int seqcnt = pagesz >> 3;
        int maxseqs = 128 >> 3;
        log->moveCursor(QTextCursor::End);
        log->insertPlainText(".");
        log->moveCursor(QTextCursor::End);
        //log->append("send bytes from " + QString::number(curbytes));
        for (int i=0; (i<maxseqs) && (cnt < sz); i++)
        {
            QByteArray ba = bin.mid(cnt, 8);
            int basz = ba.size();
            int seq = (cnt >> 3) & (seqcnt - 1);
            master->sendUpgrageData(seq, ba);
            cnt += basz;
            curbytes += basz;
            pb->setValue(cnt);
        }

        if (curbytes >= pagesz || cnt >= sz) // page transferred
            state = sEndPage;
    }
    else if (state == sEndPage)
    {
        mCurDevCount = 0;
        timer->setInterval(200); // wait ACK
        pageTransferred = true;
//#warning FUCKING HACK
        if (pagesz == 2048)
        {
            master->sendGlobalRequest(aidUpgradeProbe, true);
            master->sendServiceRequest(mNetAddress+1, svcEcho, QByteArray()); // FUCKER HACK
        }
        state = sWork;
    }
    else if (state == sFinish)
    {
        log->append("upgrade finished");
        master->sendGlobalRequest(aidUpgradeEnd, true);
        timer->stop();
        scanBtn->setEnabled(true);
        startBtn->setEnabled(false);
    }
    else if (state == sError)
    {
        log->append("ERROR!!!");
        timer->stop();
        scanBtn->setEnabled(true);
        startBtn->setEnabled(false);
    }
}

void UpgradeWidget::scan()
{
    state = sIdle;
    mDevCount = 0;
    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&mClass), 4);
    master->sendGlobalRequest(aidUpgradeStart, true, ba);
    log->append(QString().sprintf("start upgrade devices with class = 0x%08X\n", (unsigned int)mClass));
    timer->start(1000);
    scanBtn->setEnabled(false);
}

void UpgradeWidget::start()
{
    mCurDevCount = 0;
    QByteArray ba;
    ba.append(reinterpret_cast<const char*>(&sz), 4);
    master->sendGlobalRequest(aidUpgradeConfirm, true, ba);
    state = sStarted;
    scanBtn->setEnabled(false);
    startBtn->setEnabled(false);
    timer->start(100);
}

void UpgradeWidget::setPage(int page)
{
    QByteArray ba2;
    ba2.append(reinterpret_cast<const char*>(&page), 4);
    log->append("page " + QString::number(page) + " of " + QString::number(sz / pagesz)+" ...");
    master->sendGlobalRequest(aidUpgradeSetPage, true, ba2);
}


void UpgradeWidget::onGlobalMessage(unsigned char aid)
{
    aid &= 0x3F;
    if (aid == aidUpgradePageDone)
    {
        //qDebug() << "page" << (cnt / pagesz) << "done";
        mCurDevCount++;
        log->moveCursor(QTextCursor::End);
        log->insertPlainText("+1 ");
        log->moveCursor(QTextCursor::End);
        if (mCurDevCount >= mDevCount)
        {
            log->moveCursor(QTextCursor::End);
            log->insertPlainText(" success");
            log->moveCursor(QTextCursor::End);
            pageDone = true;
            timer->start(16);
        }
    }
    else if (aid == aidUpgradeRepeat)// && (pageTransferred || !cnt))
    {
        //qDebug() << "page" << (cnt / pagesz) << "repeat";
        log->moveCursor(QTextCursor::End);
        log->insertPlainText(" FAIL! repeat page");
        log->moveCursor(QTextCursor::End);
        pageRepeat = true;
        timer->start(16);
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
    else
    {
        qDebug() << "x3 response";
    }
}
