#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QtWidgets>
#include "graphwidget.h"
#include "objnetdevice.h"

using namespace Objnet;

class PlotWidget : public QWidget
{
    Q_OBJECT

private:
    GraphWidget *mGraph;
    QElapsedTimer mTimer;
    QMap<unsigned long, QString> mDevices; // serial -> name[mac]
    QMap<unsigned long, QStringList> mVarNames;
    QMap<QString, QStringList> mDependencies;
    QFormLayout *mNamesLay;
    QComboBox *mTriggerSource;
    QLineEdit *mTriggerLevel;
    QSpinBox *mTriggerOffset;
    float mTriggerWindow;
    enum TriggerEdge {TriggerOff = 0, TriggerRising, TriggerFalling} mTriggerEdge;
    float mOldTrigValue;
    float mTrigCaptureTime;
    QVector<QColor> mColors;
    int mCurColor;
    QColor nextColor();

    int mTimestamp0;
    float mTime;

    void addPoint(QString name, float val);
    void addPoint(QString name, float time, float val);
    void removeGraph(QString name);

    void addObjname(unsigned long serial, QString objname, int childCount=0);
    void removeObjname(unsigned long serial, QString objname);
    int getRow(unsigned long serial, QString objname);

    void regDevice(ObjnetDevice *dev);
    bool regObject(ObjnetDevice *dev, ObjectInfo *obj);

private slots:
    void evalAutoTriggerLevel();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public:
    explicit PlotWidget(QWidget *parent = 0);

signals:
    void periodChanged(unsigned long serial, QString objname, int period_ms);

public slots:
    void updateObject(QString name, QVariant value);
    void updateObjectGroup(QVariantMap values);
    void updateTimedObject(QString name, uint32_t timestamp, QVariant value);
    void onAutoRequestAccepted(QString objname, int periodMs);
};

#endif // PLOTWIDGET_H
