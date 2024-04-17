#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>

#if defined(ONB_SERIAL)
#include "serialonbinterface.h"
#include "serialportwidget.h"
#endif
#if defined(ONB_VIRTUAL)
#include "objnetvirtualinterface.h"
#include "objnetvirtualserver.h"
#endif
#if defined(ONB_USBHID)
#include "usbonbinterface.h"
#endif
#if defined(ONB_UDP)
#include "udponbinterface.h"
#endif

#include "objnetmaster.h"
#include "objnetnode.h"

#include "objtable.h"
#include "plotwidget.h"
#include "upgradewidget.h"
#include "objlogger.h"

#include "analyzerwidget.h"

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

protected:
    void resizeEvent(QResizeEvent *e);

private:
    Ui::MainWindow *ui;
    QPushButton *btnUpgrade;
    QPushButton *mLogEnableBtn, *btnResetStat;
    QTextEdit *editLog;
    QCheckBox *chkSvcOnly, *chkSuppressPolling;
    QLabel *status, *status2, *status3;
    QTreeWidget *mTree;
    QMap<unsigned short, QTreeWidgetItem*> mItems;
    QGroupBox *mInfoBox;
    QMap<QString, QLineEdit*> mEdits;
    QStackedWidget *mNodeWidget;

    QGroupBox *mGraphBox;
    QGroupBox *mLogBox;
    QGridLayout *mainLayout;

    UpgradeWidget *upg;
    QString firmwareDir;

    PlotWidget *mGraph;

    QComboBox *mPorts;

    int sent, received;

#if defined(ONB_SERIAL)
//    DonglePort *uart;
    SerialPortWidget *uartWidget;
#endif

    QVector<ObjnetMaster *> masters;
    ObjnetMaster *serialMaster = nullptr;
    ObjnetMaster *usbMaster = nullptr;
    ObjnetMaster *oviMaster = nullptr;
    ObjnetMaster *udpMaster = nullptr;
    ObjnetDevice *device = nullptr;

    QElapsedTimer mEtimer;

#if defined(ONB_VIRTUAL)
    ObjnetVirtualServer *onbvs;
    ObjnetVirtualInterface *onbvi;
    QPushButton *mOviServerBtn,
#endif

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
    void changeLayout(Qt::Orientation orient);

    void onMessage(ulong id, QByteArray &data);
    void onMessageSent(ulong id, QByteArray &data);

    void logMessage(QString message);
    void logMessage(QString netname, const CommonMessage &msg);

    void resetStat();

    void onTimer();
    void onConnTimer();

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
