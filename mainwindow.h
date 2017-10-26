#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include "serialcaninterface.h"
#include "objnetmaster.h"
#include "objnetnode.h"
#include "qserialport.h"
#include "qserialportinfo.h"

#include "objnetvirtualinterface.h"
#include "objnetvirtualserver.h"
#include "usbhidonbinterface.h"

#include "objtable.h"
#include "graphwidget.h"
#include "upgradewidget.h"

namespace Ui {
class MainWindow;
}

using namespace Objnet;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QPushButton *btnUpgrade;
    QPushButton *mOviServerBtn, *mLogEnableBtn, *btnResetStat;
    QTextEdit *editLog;
    QLabel *status, *status2, *status3;
    QTreeWidget *mTree;
    QMap<unsigned short, QTreeWidgetItem*> mItems;
    QGroupBox *mInfoBox;
    QMap<QString, QLineEdit*> mEdits;
    ObjTable *mObjTable;

    UpgradeWidget *upg;

    GraphWidget *mGraph;

    QComboBox *mPorts;

    int sent, received;

    QSerialPort *uart;
    SerialCan *can;
    ObjnetMaster *canMaster, *usbMaster, *oviMaster;
    ObjnetDevice *device;

    QElapsedTimer mEtimer;

    ObjnetVirtualServer *onbvs;
    ObjnetVirtualInterface *onbvi;

    QMap<QString, QTextEdit*> mLogs;

    void logMessage(ulong id, QByteArray &data, bool dir=0);
    QString ba2str(const QByteArray &ba);
    int getRootId(ObjnetMaster *mas);
    ObjnetMaster *getMasterOfItem(QTreeWidgetItem *item);

    void prepareDevice(ObjnetDevice *dev);

private slots:
    void onItemClick(QTreeWidgetItem *item, int column);
//    void onCellChanged(int row, int col);
//    void onCellDblClick(int row, int col);
//    void onObjectReceive(QString name, QVariant value);

    void onMessage(ulong id, QByteArray &data);
    void onMessageSent(ulong id, QByteArray &data);

    void logMessage(QString message);
    void logMessage(QString netname, CommonMessage &msg);

    void onBoardConnect();
    void onBoardDisconnect();

    void resetStat();

    void onTimer();

    void onDevAdded(unsigned char netAddress, const QByteArray &locData);
    void onDevConnected(unsigned char netAddress);
    void onDevDisconnected(unsigned char netAddress);
    void onDevRemoved(unsigned char netAddress);
    void onServiceMessageAccepted(unsigned char netAddress, SvcOID oid, const QByteArray &data);
    void onGlobalMessage(unsigned char aid);

    void onDevReady();

    void onPortChanged(QString portname);

    void upgrade(unsigned long classId);

    void onBindTest(int var);

    void onDeviceMenu(QPoint p);
    void onObjectMenu(QPoint p);

    void setAutoRequestPeriod(unsigned long serial, QString objname, int period_ms);
};

#endif // MAINWINDOW_H
