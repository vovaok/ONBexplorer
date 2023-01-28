#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include "serialonbinterface.h"
#include "objnetmaster.h"
#include "objnetnode.h"
#include "serialportwidget.h"

#include "objnetvirtualinterface.h"
#include "objnetvirtualserver.h"
#include "usbonbinterface.h"
#include "udponbinterface.h"

#include "objtable.h"
#include "plotwidget.h"
#include "upgradewidget.h"

namespace Ui {
class MainWindow;
}

using namespace Objnet;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QPushButton *btnUpgrade;
    QPushButton *mOviServerBtn, *mLogEnableBtn, *btnResetStat;
    QTextEdit *editLog;
    QCheckBox *chkSvcOnly, *chkSuppressPolling;
    QLabel *status, *status2, *status3;
    QTreeWidget *mTree;
    QMap<unsigned short, QTreeWidgetItem*> mItems;
    QGroupBox *mInfoBox;
    QMap<QString, QLineEdit*> mEdits;
    ObjTable *mObjTable;

    UpgradeWidget *upg;
    QString firmwareDir;

    PlotWidget *mGraph;

    QComboBox *mPorts;

    int sent, received;

//    DonglePort *uart;
    SerialPortWidget *uartWidget;

    QVector<ObjnetMaster *> masters;
    ObjnetMaster *serialMaster, *usbMaster, *oviMaster, *udpMaster;
    ObjnetDevice *device;

    QElapsedTimer mEtimer;

    ObjnetVirtualServer *onbvs;
    ObjnetVirtualInterface *onbvi;

    QMap<QString, QTextEdit*> mLogs;

    unsigned long lastDeviceId;

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
    void logMessage(QString netname, const CommonMessage &msg);

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

    void upgrade(ObjnetMaster *master, unsigned long classId, unsigned char netAddress=0);

    void onBindTest(int var);

    void onDeviceMenu(QPoint p);
    void onObjectMenu(QPoint p);

    void setAutoRequestPeriod(unsigned long serial, QString objname, int period_ms);
    void onAutoRequestAccepted(QString objname, int periodMs);
};

#endif // MAINWINDOW_H
