#ifndef UPGRADEWIDGET_H
#define UPGRADEWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include "objnetmaster.h"

using namespace Objnet;

#pragma pack(push,1)
typedef struct
{
    char const pre[12];
    unsigned long cid;
    unsigned short ver;
    unsigned short pageSize;
    unsigned long length;
    unsigned long checksum;
    char timestamp[25];
} __appinfo_t__;
#pragma pack(pop)

class UpgradeWidget : public QWidget
{
    Q_OBJECT

private:
    ObjnetMaster *master;
    QProgressBar *pb;
    QTextEdit *log;
    QTimer *timer;
    QPushButton *scanBtn;
    QPushButton *startBtn;

    QByteArray bin;
    enum {sIdle, sStarted, sWork, sTransferPage, sEndPage, sFinish, sError} state;
    int mDevCount, mCurDevCount;
    int sz, cnt;
    int pagesz;
    int curbytes;
    unsigned long mClass;
    bool pageDone, pageTransferred, pageRepeat;

    void setPage(int page);

protected:
    void closeEvent(QCloseEvent *e) override;

public:
    UpgradeWidget(ObjnetMaster *onbMaster, QWidget *parent = 0);

    void load(QByteArray firmware);
    static bool checkClass(const QByteArray &firmware, unsigned long cid);
    void logAppend(QString s) {log->append(s);}

private slots:
    void onTimer();
    void scan();
    void start();

    void onGlobalMessage(unsigned char aid);
};

#endif // UPGRADEWIDGET_H
