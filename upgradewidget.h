#ifndef UPGRADEWIDGET_H
#define UPGRADEWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include "onbupgrader.h"

using namespace Objnet;

class UpgradeWidget : public QWidget
{
    Q_OBJECT

private:
    OnbUpgrader *upg;
    QProgressBar *pb;
    QTextEdit *log;
    QPushButton *scanBtn;
    QPushButton *startBtn;
    QByteArray mBin;
    QVector<unsigned char> mAddresses;

protected:
    void closeEvent(QCloseEvent *e) override;

public:
    UpgradeWidget(ObjnetMaster *onbMaster, QWidget *parent = 0);

    void load(const QByteArray &firmware);
    static bool checkClass(const QByteArray &firmware, unsigned long cid);
    void logAppend(string s);

    void scan(unsigned char netaddr);

private slots:
    void scan();
    void start();
    void onFinish();
};

#endif // UPGRADEWIDGET_H
