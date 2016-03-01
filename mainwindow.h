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
    QPushButton *btn, *btn2, *btnResetStat, *btnUpgrade;
    QPushButton *btnProto1, *btnProto2;
    QLineEdit *editId, *editData;
    QLineEdit *editIdIn, *editDataIn;
    QTextEdit *editLog;
    QLabel *status, *status2;
    QTreeWidget *mTree;
    QMap<unsigned char, QTreeWidgetItem*> mItems;
    QGroupBox *mInfoBox;//, *mObjBox;
    QMap<QString, QLineEdit*> mEdits;
    QTableWidget *mObjTable;

    QComboBox *mPorts;

    int sent, received;

    QSerialPort *uart;
    SerialCan *can;
    ObjnetMaster *master;
    unsigned char curdev;

    unsigned short mAdcValue;
    QString strtest;
    int testVar;

    QElapsedTimer mEtimer;

    ObjnetVirtualServer *onbvs;
    QVector<ObjnetVirtualInterface*> onb;

    void logMessage(ulong id, QByteArray &data, bool dir=0);
    QString ba2str(const QByteArray &ba);

private slots:
    void onBtn();
    void onBtn2();
    void onBtnProto();
    void onItemClick(QTreeWidgetItem *item, int column);
    void onCellChanged(int row, int col);
    void onObjectReceive(QString name, QVariant value);

    void onMessage(ulong id, QByteArray &data);
    void onMessageSent(ulong id, QByteArray &data);

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

    void onPortChanged(QString portname);

    void upgrade();

    void onBindTest(int var);
};

#endif // MAINWINDOW_H
